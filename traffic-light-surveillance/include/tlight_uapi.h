/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * tlight_uapi.h - User/kernel interface of the traffic light driver.
 * Shared by driver/tlight.c (kernel) and the C++ applications (user space).
 */
#ifndef TLIGHT_UAPI_H
#define TLIGHT_UAPI_H

#include <linux/types.h>
#include <linux/ioctl.h>

enum tl_state {
	TL_RED    = 0,
	TL_GREEN  = 1,
	TL_YELLOW = 2,
};

enum tl_event_type {
	TL_EVT_STATE_CHANGE = 1,  /* light changed colour            */
	TL_EVT_VIOLATION    = 2,  /* vehicle detected while RED      */
};

/* One record returned by read() on /dev/tlight (fixed size, 24 bytes). */
struct tl_event {
	__u64 ts_ns;       /* wall-clock timestamp in nanoseconds   */
	__u32 type;        /* enum tl_event_type                    */
	__u32 state;       /* light state when the event happened   */
	__u32 violations;  /* total violations so far               */
	__u32 reserved;
};

/* Phase durations in milliseconds (valid range 100 .. 600000). */
struct tl_config {
	__u32 red_ms;
	__u32 green_ms;
	__u32 yellow_ms;
};

struct tl_status {
	__u32 state;       /* enum tl_state                 */
	__u32 auto_mode;   /* 1 = automatic cycling         */
	__u32 violations;  /* red-light violations          */
	__u32 vehicles;    /* vehicles detected in total    */
	struct tl_config cfg;
};

#define TL_IOC_MAGIC       'T'
#define TL_IOC_GET_STATUS  _IOR(TL_IOC_MAGIC, 1, struct tl_status)
#define TL_IOC_SET_CONFIG  _IOW(TL_IOC_MAGIC, 2, struct tl_config)
#define TL_IOC_SET_AUTO    _IOW(TL_IOC_MAGIC, 3, __u32)
#define TL_IOC_SET_STATE   _IOW(TL_IOC_MAGIC, 4, __u32)
#define TL_IOC_SIM_VEHICLE _IO(TL_IOC_MAGIC, 5)
#define TL_IOC_RESET_STATS _IO(TL_IOC_MAGIC, 6)

#endif /* TLIGHT_UAPI_H */
