# Stage 2 – Project Requirements Document (PRD) & Development Plan

**Project:** Traffic Light Surveillance  **Version:** 1.0  **Author:** `<Your Name>`

## 1. Purpose and scope
See Stage 1. One intersection approach, simulated vehicle sensor, Linux x86-64/ARM, kernel ≥ 5.x.

## 2. Functional requirements

| ID | Requirement | Verified by |
|---|---|---|
| FR-1 | The driver shall expose a character device `/dev/tlight`. | integration (hw) |
| FR-2 | In automatic mode the light shall cycle RED → GREEN → YELLOW → RED using configurable durations. | `sm_cycles_*`, `sm_large_step_*`, integration |
| FR-3 | The driver shall start in RED with defaults 4000/4000/1500 ms (module parameters can override). | `sm_initial_state_*` |
| FR-4 | A vehicle detected while RED shall count as a violation and generate a `VIOLATION` event; on GREEN/YELLOW it shall not. | `sm_vehicle_on_*`, integration |
| FR-5 | Every colour change shall generate a `STATE_CHANGE` event with timestamp. | `sm_event_timestamps_*` |
| FR-6 | Events shall be readable with blocking `read()` and `poll()`; non-blocking mode returns `-EAGAIN`. | integration (hw) |
| FR-7 | The driver shall provide ioctls: get status, set config, set auto, set state, simulate vehicle, reset stats. | `cmd_*`, integration (hw) |
| FR-8 | Invalid inputs (state > YELLOW, durations outside 100–600000 ms) shall be rejected with `-EINVAL`. | `sm_set_state_rejects_invalid`, `sm_config_validation` |
| FR-9 | The daemon shall log all events to a CSV file with header `timestamp,event,state,violations`. | `fmt_*`, integration |
| FR-10 | The daemon shall maintain statistics (events, state changes, cycles, violations, last violation). | `stats_counts_events` |
| FR-11 | The daemon shall accept control commands over a UNIX socket (STATUS, STATE, AUTO, VEHICLE, CONFIG, STATS, RESET, HELP). | `cmd_*`, integration |
| FR-12 | The daemon shall shut down cleanly on SIGINT/SIGTERM (close device, remove socket, write summary). | integration |
| FR-13 | The system shall run without the kernel module using `--sim`. | integration |
| FR-14 | `tlightctl` shall return exit code 0 on `OK` replies and non-zero otherwise. | integration |

## 3. Non-functional requirements

| ID | Requirement | How checked |
|---|---|---|
| NFR-1 | **Reliability:** the driver shall not lose the *newest* events; on ring overflow (64) the oldest are dropped. | code review / stress test |
| NFR-2 | **Concurrency safety:** all shared driver state protected by a spinlock; no sleeping under the lock. | review, `CONFIG_PROVE_LOCKING` run |
| NFR-3 | **Latency:** event reaches the daemon log < 100 ms after it occurs. | integration timing |
| NFR-4 | **Portability:** user space builds with g++ ≥ 9 (C++17), no external libraries. | CI |
| NFR-5 | **Maintainability:** one shared API header, interface-based design, warnings-clean (`-Wall -Wextra -Wpedantic`). | CI |
| NFR-6 | **Testability:** core logic testable without kernel or root. | `make test` |
| NFR-7 | **Security (demo level):** control socket is local only; device mode 0666 is demo-only and documented. | review |
| NFR-8 | **Resource safety:** no leaks of file descriptors; RAII for the device handle. | valgrind / review |

## 4. Modules, features, deliverables

| Module | Features |
|---|---|
| `driver/tlight.c` | cycle timer, event ring, ioctl/read/write/poll, module params |
| `TrafficStateMachine` | deterministic mirror of driver logic |
| `IDevice`, `TLightDevice`, `SimDevice` | hardware abstraction (real + simulated) |
| `Monitor`, `EventLogger`, `Statistics` | event pipeline |
| `ControlServer`, `tlightctl` | remote control |
| `tests/`, `scripts/` | unit + integration tests |

Deliverables: source code, Git repo with stage tags, PRD, design + UML, test report, final report, CSV sample.

## 5. Assumptions and constraints
Vehicle sensor simulated; one reader of the event queue is expected; Linux only; root required to
load the module.

## 6. Risks
| Risk | Mitigation |
|---|---|
| Kernel module crashes a VM | develop in a disposable VM; snapshot before `insmod` |
| Kernel headers missing | `--sim` mode + CI builds user space regardless |
| Timing flakiness in tests | state machine uses a *simulated* clock in unit tests |

## 7. Development plan and timeline (6 stages)

| Stage | Focus | Output | Git tag |
|---|---|---|---|
| 1 | Introduction | `01_project_introduction.md` | `stage-1` |
| 2 | Requirements, plan | this PRD | `stage-2` |
| 3 | Design, UML, env, Git | design doc, UML, Makefiles, skeleton | `stage-3` |
| 4 | Prototype | driver + state machine + sim + daemon skeleton | `stage-4` |
| 5 | Test & improve | full test suite, bug fixes, CI | `stage-5` |
| 6 | Final delivery | final report, demo, release | `v1.0` |

## 8. Roadmap to Stage 3
Define architecture, data structures, UML; set up Git branching and tooling.
