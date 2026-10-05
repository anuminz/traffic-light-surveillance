# Stage 1 – Project Introduction

## Project idea
**Traffic Light Surveillance**: a Linux kernel driver that controls a traffic light and detects
red-light violations, plus a C++ user-space system that monitors, logs and controls it.

## Problem to be solved
Traffic signals and the sensors that watch them are normally separate black boxes. Operators need
(1) a reliable signal controller, (2) automatic evidence when a vehicle crosses on red, and
(3) a log they can analyse later. This project builds that pipeline end to end on Linux.

## Objective
Build a layered system that demonstrates the three required areas:

| Area | Where it is demonstrated |
|---|---|
| Linux device driver | `driver/tlight.c`: misc char device, ioctl, read/write/poll, wait queues, delayed work, spinlocks, module parameters |
| System programming | `tlightd`/`tlightctl`: open/read/poll/ioctl, signals (`sigaction`), UNIX sockets, threads, daemonization, file I/O |
| C++ | C++17 RAII, interfaces/polymorphism, state machine, mutex-protected classes, STL, unit tests |

## Scope
**In scope:** one intersection approach (one light, one stop-line sensor); automatic RED→GREEN→YELLOW
cycle; configurable timings; manual override; simulated vehicle sensor; event queue; CSV logging;
statistics; control CLI; simulator fallback; tests; CI.

**Out of scope:** real GPIO/camera hardware, multi-junction coordination, number-plate recognition,
networking beyond a local socket, GUI.

## Expected outcome
- A loadable module `tlight.ko` exposing `/dev/tlight`.
- `tlightd` writing `timestamp,event,state,violations` CSV rows (see `docs/sample_events.csv`).
- `tlightctl` to inspect and control the system.
- Documented process: requirements → design → implementation → testing → delivery.

## Applications
Teaching embedded Linux; prototype for a smart-city violation recorder; base for GPIO/camera-backed
versions (replace the simulated sensor by an interrupt handler).

## Stage-1 deliverables / presentation checklist
- [x] Idea, problem, scope, outcome (this document)
- [x] Git repository created, first commit `docs: add project introduction`
- [ ] 5-minute presentation: problem → idea → block diagram (README) → plan for stage 2

## Roadmap to Stage 2
Convert the objectives into numbered functional/non-functional requirements and a timeline (PRD).
