# Stage 4 – Initial Implementation & Prototype

## Implemented core modules
| Module | Status | Notes |
|---|---|---|
| `include/tlight_uapi.h` | done | shared structs + ioctl numbers |
| `TrafficStateMachine` | done | simulated clock, exact timestamps |
| `IDevice`, `SimDevice`, `TLightDevice` | done | real + simulated back-ends |
| `Monitor`, `EventLogger`, `Statistics` | done | CSV pipeline |
| `ControlServer`, `tlightd`, `tlightctl` | done | socket control, signals, `--daemon` |
| `driver/tlight.c` | done | misc device, timer, ring, ioctl/read/write/poll |

## Progressive integration order
1. State machine + unit tests → 2. SimDevice → 3. Monitor/Logger/Statistics → 4. ControlServer + tools →
5. Kernel driver; swap `SimDevice` for `TLightDevice` with **no change** to the services (interface payoff).

## Demonstration (simulator, real captured output)
```console
$ ./build/tlightd --sim --log demo.csv --quiet &
$ ./build/tlightctl CONFIG 300 300 200
OK config updated
$ ./build/tlightctl AUTO 0 ; ./build/tlightctl STATE R ; ./build/tlightctl VEHICLE
OK auto=0
OK state=RED
OK vehicle detected
$ ./build/tlightctl STATUS
OK state=YELLOW auto=1 vehicles=2 violations=1 red_ms=300 green_ms=300 yellow_ms=200 | monitor: events=8 ...
```
Resulting log: [`docs/sample_events.csv`](sample_events.csv) – shows `VIOLATION,RED,1` while the light was red.

### Demonstration with the real driver (do on your VM and paste your own output here)
```bash
make driver && sudo insmod driver/tlight.ko
dmesg | tail -3                       # "tlight: loaded, /dev/tlight ..."
echo v > /dev/tlight                  # vehicle on RED -> violation
./build/tlightd --device /dev/tlight --log /tmp/events.csv
```

## Progress log (fill in as you work)
| Date | Task | Issue encountered | Solution |
|---|---|---|---|
| `<date>` | Wrote state machine | Event timestamps wrong when one `advance()` crossed several phases | Advance clock phase-by-phase instead of once at the end |
| `<date>` | SimDevice | Tests were timing-dependent | Added non-realtime mode with manual `advance()` |
| `<date>` | ControlServer | Server thread never stopped | Used `poll()` with 200 ms timeout + atomic stop flag |
| `<date>` | Driver | Risk of deadlock in `set_auto` | Call `cancel_delayed_work_sync()` only after releasing the spinlock |

## Roadmap to Stage 5
Complete test coverage (unit/integration/system), run the driver tests on a VM, fix defects,
add CI, review performance and code quality.
