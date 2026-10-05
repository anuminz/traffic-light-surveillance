// SPDX-License-Identifier: GPL-2.0
/*
 * tlight.c - Traffic light controller and red-light violation sensor.
 *
 * A misc character device (/dev/tlight) that
 *   - runs an automatic RED -> GREEN -> YELLOW -> RED cycle (delayed work),
 *   - simulates a road sensor: a vehicle detected while RED is a violation,
 *   - queues events in a ring buffer readable with read()/poll(),
 *   - is controlled through ioctl() or single-character write():
 *       r/g/y = set state, a = auto on, m = manual, v = vehicle detected.
 *
 * On real hardware the vehicle sensor would be a GPIO interrupt; here it is
 * simulated so the project runs on any Linux machine or VM.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/workqueue.h>
#include <linux/ktime.h>
#include <linux/slab.h>
#include "tlight_uapi.h"

#define DRV_NAME      "tlight"
#define TL_RING_SIZE  64              /* must be a power of two */
#define TL_MIN_MS     100
#define TL_MAX_MS     600000

static unsigned int red_ms = 4000;
module_param(red_ms, uint, 0444);
MODULE_PARM_DESC(red_ms, "Initial RED duration in ms (100..600000)");

static unsigned int green_ms = 4000;
module_param(green_ms, uint, 0444);
MODULE_PARM_DESC(green_ms, "Initial GREEN duration in ms (100..600000)");

static unsigned int yellow_ms = 1500;
module_param(yellow_ms, uint, 0444);
MODULE_PARM_DESC(yellow_ms, "Initial YELLOW duration in ms (100..600000)");

static bool auto_start = true;
module_param(auto_start, bool, 0444);
MODULE_PARM_DESC(auto_start, "Start in automatic cycling mode");

struct tl_dev {
	struct miscdevice    misc;
	spinlock_t           lock;       /* protects everything below */
	wait_queue_head_t    wq;
	struct delayed_work  work;
	struct tl_config     cfg;
	u32                  state;
	u32                  auto_mode;
	u32                  violations;
	u32                  vehicles;
	struct tl_event      ring[TL_RING_SIZE];
	unsigned int         head;       /* free-running write index */
	unsigned int         tail;       /* free-running read index  */
};

static struct tl_dev *tl;

static u32 tl_phase_ms(const struct tl_dev *d, u32 s)
{
	switch (s) {
	case TL_RED:   return d->cfg.red_ms;
	case TL_GREEN: return d->cfg.green_ms;
	default:       return d->cfg.yellow_ms;
	}
}

static u32 tl_next(u32 s)
{
	switch (s) {
	case TL_RED:   return TL_GREEN;
	case TL_GREEN: return TL_YELLOW;
	default:       return TL_RED;
	}
}

/* Caller must hold d->lock. Overwrites the oldest event when full. */
static void tl_push_locked(struct tl_dev *d, u32 type)
{
	struct tl_event *e = &d->ring[d->head & (TL_RING_SIZE - 1)];

	e->ts_ns      = ktime_get_real_ns();
	e->type       = type;
	e->state      = d->state;
	e->violations = d->violations;
	e->reserved   = 0;
	d->head++;
	if (d->head - d->tail > TL_RING_SIZE)
		d->tail = d->head - TL_RING_SIZE;
}

static void tl_work_fn(struct work_struct *w)
{
	struct tl_dev *d = container_of(to_delayed_work(w), struct tl_dev, work);
	unsigned long flags;

	spin_lock_irqsave(&d->lock, flags);
	if (d->auto_mode) {
		d->state = tl_next(d->state);
		tl_push_locked(d, TL_EVT_STATE_CHANGE);
		schedule_delayed_work(&d->work,
				      msecs_to_jiffies(tl_phase_ms(d, d->state)));
	}
	spin_unlock_irqrestore(&d->lock, flags);
	wake_up_interruptible(&d->wq);
}

static int tl_set_state(struct tl_dev *d, u32 s)
{
	unsigned long flags;
	bool rearm;
	u32 delay;

	if (s > TL_YELLOW)
		return -EINVAL;

	spin_lock_irqsave(&d->lock, flags);
	d->state = s;
	tl_push_locked(d, TL_EVT_STATE_CHANGE);
	rearm = d->auto_mode;
	delay = tl_phase_ms(d, s);
	spin_unlock_irqrestore(&d->lock, flags);

	if (rearm)
		mod_delayed_work(system_wq, &d->work, msecs_to_jiffies(delay));
	wake_up_interruptible(&d->wq);
	return 0;
}

static void tl_set_auto(struct tl_dev *d, bool on)
{
	unsigned long flags;
	bool changed;
	u32 delay;

	spin_lock_irqsave(&d->lock, flags);
	changed = (d->auto_mode != on);
	d->auto_mode = on;
	delay = tl_phase_ms(d, d->state);
	spin_unlock_irqrestore(&d->lock, flags);

	if (!changed)
		return;
	if (on)
		mod_delayed_work(system_wq, &d->work, msecs_to_jiffies(delay));
	else
		cancel_delayed_work_sync(&d->work);
}

static void tl_vehicle(struct tl_dev *d)
{
	unsigned long flags;
	bool violation = false;

	spin_lock_irqsave(&d->lock, flags);
	d->vehicles++;
	if (d->state == TL_RED) {
		d->violations++;
		tl_push_locked(d, TL_EVT_VIOLATION);
		violation = true;
	}
	spin_unlock_irqrestore(&d->lock, flags);

	if (violation)
		wake_up_interruptible(&d->wq);
}

/* ------------------------------------------------------------------ */
/* file operations                                                    */
/* ------------------------------------------------------------------ */

static ssize_t tl_read(struct file *f, char __user *buf, size_t count,
		       loff_t *ppos)
{
	struct tl_dev *d = tl;
	struct tl_event ev;
	size_t done = 0;
	unsigned long flags;
	int ret;

	if (count < sizeof(ev))
		return -EINVAL;

	while (done + sizeof(ev) <= count) {
		spin_lock_irqsave(&d->lock, flags);
		if (d->head == d->tail) {
			spin_unlock_irqrestore(&d->lock, flags);
			if (done)
				break;
			if (f->f_flags & O_NONBLOCK)
				return -EAGAIN;
			ret = wait_event_interruptible(d->wq, d->head != d->tail);
			if (ret)
				return ret;
			continue;
		}
		ev = d->ring[d->tail & (TL_RING_SIZE - 1)];
		d->tail++;
		spin_unlock_irqrestore(&d->lock, flags);

		if (copy_to_user(buf + done, &ev, sizeof(ev)))
			return done ? (ssize_t)done : -EFAULT;
		done += sizeof(ev);
	}
	return done;
}

static ssize_t tl_write(struct file *f, const char __user *buf, size_t count,
			loff_t *ppos)
{
	char c;
	int ret = 0;

	if (!count)
		return 0;
	if (get_user(c, buf))
		return -EFAULT;

	switch (c) {
	case 'r': ret = tl_set_state(tl, TL_RED);    break;
	case 'g': ret = tl_set_state(tl, TL_GREEN);  break;
	case 'y': ret = tl_set_state(tl, TL_YELLOW); break;
	case 'a': tl_set_auto(tl, true);             break;
	case 'm': tl_set_auto(tl, false);            break;
	case 'v': tl_vehicle(tl);                    break;
	default:  return -EINVAL;
	}
	return ret ? ret : (ssize_t)count;
}

static __poll_t tl_poll(struct file *f, poll_table *wait)
{
	struct tl_dev *d = tl;
	__poll_t mask = 0;

	poll_wait(f, &d->wq, wait);
	if (d->head != d->tail)
		mask |= EPOLLIN | EPOLLRDNORM;
	return mask;
}

static bool tl_cfg_valid(const struct tl_config *c)
{
	return c->red_ms    >= TL_MIN_MS && c->red_ms    <= TL_MAX_MS &&
	       c->green_ms  >= TL_MIN_MS && c->green_ms  <= TL_MAX_MS &&
	       c->yellow_ms >= TL_MIN_MS && c->yellow_ms <= TL_MAX_MS;
}

static long tl_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	struct tl_dev *d = tl;
	void __user *uarg = (void __user *)arg;
	unsigned long flags;

	switch (cmd) {
	case TL_IOC_GET_STATUS: {
		struct tl_status st;

		spin_lock_irqsave(&d->lock, flags);
		st.state      = d->state;
		st.auto_mode  = d->auto_mode;
		st.violations = d->violations;
		st.vehicles   = d->vehicles;
		st.cfg        = d->cfg;
		spin_unlock_irqrestore(&d->lock, flags);
		return copy_to_user(uarg, &st, sizeof(st)) ? -EFAULT : 0;
	}
	case TL_IOC_SET_CONFIG: {
		struct tl_config c;

		if (copy_from_user(&c, uarg, sizeof(c)))
			return -EFAULT;
		if (!tl_cfg_valid(&c))
			return -EINVAL;
		spin_lock_irqsave(&d->lock, flags);
		d->cfg = c;
		spin_unlock_irqrestore(&d->lock, flags);
		return 0;
	}
	case TL_IOC_SET_AUTO: {
		__u32 v;

		if (copy_from_user(&v, uarg, sizeof(v)))
			return -EFAULT;
		tl_set_auto(d, !!v);
		return 0;
	}
	case TL_IOC_SET_STATE: {
		__u32 v;

		if (copy_from_user(&v, uarg, sizeof(v)))
			return -EFAULT;
		return tl_set_state(d, v);
	}
	case TL_IOC_SIM_VEHICLE:
		tl_vehicle(d);
		return 0;
	case TL_IOC_RESET_STATS:
		spin_lock_irqsave(&d->lock, flags);
		d->violations = 0;
		d->vehicles = 0;
		spin_unlock_irqrestore(&d->lock, flags);
		return 0;
	default:
		return -ENOTTY;
	}
}

static const struct file_operations tl_fops = {
	.owner          = THIS_MODULE,
	.read           = tl_read,
	.write          = tl_write,
	.poll           = tl_poll,
	.unlocked_ioctl = tl_ioctl,
	.llseek         = no_llseek,
};

static int __init tl_init(void)
{
	struct tl_config c = { red_ms, green_ms, yellow_ms };
	int ret;

	if (!tl_cfg_valid(&c)) {
		pr_err(DRV_NAME ": phase durations must be %d..%d ms\n",
		       TL_MIN_MS, TL_MAX_MS);
		return -EINVAL;
	}

	tl = kzalloc(sizeof(*tl), GFP_KERNEL);
	if (!tl)
		return -ENOMEM;

	spin_lock_init(&tl->lock);
	init_waitqueue_head(&tl->wq);
	INIT_DELAYED_WORK(&tl->work, tl_work_fn);
	tl->cfg       = c;
	tl->state     = TL_RED;
	tl->auto_mode = auto_start;

	tl->misc.minor = MISC_DYNAMIC_MINOR;
	tl->misc.name  = DRV_NAME;
	tl->misc.fops  = &tl_fops;
	tl->misc.mode  = 0666;   /* demo only: world accessible */

	ret = misc_register(&tl->misc);
	if (ret) {
		kfree(tl);
		return ret;
	}

	if (tl->auto_mode)
		schedule_delayed_work(&tl->work, msecs_to_jiffies(tl->cfg.red_ms));

	pr_info(DRV_NAME ": loaded, /dev/%s (red=%u green=%u yellow=%u ms)\n",
		DRV_NAME, red_ms, green_ms, yellow_ms);
	return 0;
}

static void __exit tl_exit(void)
{
	cancel_delayed_work_sync(&tl->work);
	misc_deregister(&tl->misc);
	kfree(tl);
	pr_info(DRV_NAME ": unloaded\n");
}

module_init(tl_init);
module_exit(tl_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name <you@example.com>");
MODULE_DESCRIPTION("Traffic light controller and red-light violation sensor");
MODULE_VERSION("1.0");
