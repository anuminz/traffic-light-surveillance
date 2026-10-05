# Traffic Light Surveillance

An individual project covering **Linux device drivers**, **system programming** and **C++**.

A kernel driver (`/dev/tlight`) controls a traffic light and detects red-light violations.
A C++17 daemon (`tlightd`) monitors the driver, logs every event to CSV and offers a control socket;
`tlightctl` is the command-line client.

```
 vehicle sensor (simulated) ─┐
                             ▼
  ┌─────────────┐  read/poll/ioctl  ┌──────────────────────────┐  UNIX socket  ┌───────────┐
  │ tlight.ko   │◄─────────────────►│ tlightd (C++17 daemon)   │◄─────────────►│ tlightctl │
  │ /dev/tlight │                   │ Monitor·Logger·Stats·Ctl │               └───────────┘
  └─────────────┘                   └────────────┬─────────────┘
   RED→GREEN→YELLOW cycle                        ▼
   violation detection                 tlight_events.csv
```

No hardware or kernel module? Run the daemon with `--sim` – a user-space simulator implements the same interface.

## Quick start

```bash
make                      # build tlightd + tlightctl
make test                 # 27 unit tests (no kernel needed)
make integration          # end-to-end test using the simulator

# --- simulator mode (any machine) ---
./build/tlightd --sim &
./build/tlightctl STATUS
./build/tlightctl AUTO 0
./build/tlightctl STATE R
./build/tlightctl VEHICLE        # vehicle on RED -> violation
./build/tlightctl STATS
kill %1

# --- real driver (needs kernel headers + root) ---
make driver
sudo insmod driver/tlight.ko red_ms=3000 green_ms=3000 yellow_ms=1000
echo v > /dev/tlight             # simulate a vehicle
./build/tlightd --device /dev/tlight --log /tmp/events.csv
sudo rmmod tlight
```

## Commands (tlightctl)

| Command | Meaning |
|---|---|
| `STATUS` | light state, mode, counters, monitor statistics |
| `STATE R\|G\|Y` | force the light to a colour |
| `AUTO 0\|1` | manual / automatic cycling |
| `VEHICLE` | simulate a vehicle crossing the stop line |
| `CONFIG r g y` | phase durations in ms (100 –600000) |
| `STATS`, `RESET`, `HELP` | statistics, clear counters, help |

Driver `write()` shortcuts: `r g y` set colour, `a` auto, `m` manual, `v` vehicle.

## Repository layout

```
include/tlight_uapi.h   shared kernel/user API (ioctl numbers, structs)
driver/                 Linux kernel module (tlight.c, Makefile)
app/include, app/src    C++ library: state machine, devices, logger, stats, monitor, server
app/tools               tlightd (daemon) and tlightctl (CLI)
tests/                  unit tests (own tiny framework, no dependencies)
scripts/                integration test
docs/                   one document per project stage + UML diagrams
.github/workflows       CI (build, unit + integration tests, module build)
```

## Documentation (stages)

| Stage | Document |
|---|---|
| 1 Introduction | [docs/01_project_introduction.md](docs/01_project_introduction.md) |
| 2 Requirements & plan (PRD) | [docs/02_requirements_prd.md](docs/02_requirements_prd.md) |
| 3 Design & architecture | [docs/03_system_design.md](docs/03_system_design.md), UML in [docs/uml/](docs/uml/) |
| 4 Prototype | [docs/04_prototype_progress.md](docs/04_prototype_progress.md) |
| 5 Testing & improvement | [docs/05_testing_report.md](docs/05_testing_report.md) |
| 6 Final report & presentation | [docs/06_final_report.md](docs/06_final_report.md) |
| Git workflow & commit plan | [docs/GIT_WORKFLOW.md](docs/GIT_WORKFLOW.md) |

## Author
<ANU MINZ> – <Roll no -2341013130>
