# Stage 5 – Testing, Integration & Improvement

## Test strategy
| Level | What | Tool | Needs kernel? |
|---|---|---|---|
| Unit | state machine, formatting, statistics, command parser, monitor | `make test` (27 tests) | no |
| Integration | daemon + socket + CLI + log + signals | `make integration` (14 checks, simulator) | no |
| System | real driver + daemon | `./scripts/integration_test.sh --hw` and manual checklist below | yes |
| Static/dynamic | warnings-clean build, valgrind, sparse/checkpatch for the module | optional | partly |

## Results obtained
* **Unit tests:** 27/27 passed (`make test`).
* **Integration (simulator):** 14/14 checks passed (`make integration`).
* **Build:** `-Wall -Wextra -Wpedantic`, zero warnings (g++, C++17).
* **Kernel module:** *not yet run in this repository's author environment — complete the table below on your VM.*

### Traceability (requirement → tests)
See the "Verified by" column of [02_requirements_prd.md](02_requirements_prd.md).

## System test checklist for the kernel driver (fill in)
Run on an Ubuntu VM with kernel headers: `make driver && sudo insmod driver/tlight.ko`.

| # | Test | Command | Expected | Result |
|---|---|---|---|---|
| K1 | Module loads | `dmesg \| tail` | `tlight: loaded` | ☐ |
| K2 | Node exists | `ls -l /dev/tlight` | `crw-rw-rw-` | ☐ |
| K3 | Auto cycle | `./build/tlightd --device /dev/tlight --log /tmp/e.csv` for 15 s | GREEN, YELLOW, RED rows at ≈4 s/4 s/1.5 s spacing | ☐ |
| K4 | Violation on red | `echo m > /dev/tlight; echo r > /dev/tlight; echo v > /dev/tlight` | `VIOLATION,RED,1` | ☐ |
| K5 | No violation on green | `echo g > /dev/tlight; echo v > /dev/tlight` | counter unchanged | ☐ |
| K6 | Bad write | `echo x > /dev/tlight` | `Invalid argument` | ☐ |
| K7 | Bad config | `./build/tlightctl CONFIG 1 1 1` | `ERR`, driver returns EINVAL | ☐ |
| K8 | Non-blocking read | small program with `O_NONBLOCK` and empty queue | `EAGAIN` | ☐ |
| K9 | Overflow | `for i in $(seq 200); do echo y > /dev/tlight; done` then read | 64 newest events, no crash | ☐ |
| K10 | Unload while idle | `sudo rmmod tlight` | clean unload, no oops in `dmesg` | ☐ |
| K11 | Unload while auto | load, `rmmod` immediately | no timer fires after unload | ☐ |
| K12 | Module params | `insmod … red_ms=50` | load fails with `Invalid argument` | ☐ |

## Defects found and fixed (examples recorded during development)
| ID | Defect | Fix | Test added |
|---|---|---|---|
| D1 | Timestamps wrong when one `advance()` spanned several phases | per-phase clock advance | `sm_event_timestamps_are_exact_and_monotonic` |
| D2 | Phase timer not restarted after leaving manual mode | `setAuto()` resets `elapsed_ms_` | `sm_reenabling_auto_restarts_phase_timer` |
| D3 | Control thread blocked shutdown | `poll()` with timeout | integration: exits 0 on SIGTERM |
| D4 | Config of 0 ms could busy-loop the cycle | min 100 ms validation in kernel and user space | `sm_config_validation` |

## Improvements to performance, reliability and code quality
* Ring buffer drops oldest events instead of blocking the timer.
* Whole events copied outside the spinlock; `read()` returns several events per syscall.
* `Monitor::run` backs off and aborts after repeated device errors.
* Interfaces + RAII; no raw `new/delete`; no external dependencies; CI on every push.

## Optional extra checks
```bash
valgrind --leak-check=full ./build/run_tests
make clean && make CXXFLAGS="-std=c++17 -fsanitize=address,undefined -g -Iinclude -Iapp/include -pthread" test
```

## Roadmap to Stage 6
Finish documentation, record demo, tag release, prepare final presentation.
