# UML – State machine diagram

```mermaid
stateDiagram-v2
    [*] --> RED
    RED --> GREEN: auto && elapsed ≥ red_ms
    GREEN --> YELLOW: auto && elapsed ≥ green_ms
    YELLOW --> RED: auto && elapsed ≥ yellow_ms
    RED --> RED: vehicle / violations++, emit VIOLATION
    GREEN --> GREEN: vehicle / vehicles++
    YELLOW --> YELLOW: vehicle / vehicles++
    RED --> GREEN: SET_STATE(G)
    GREEN --> RED: SET_STATE(R)
    YELLOW --> GREEN: SET_STATE(G)
```

Every transition that changes the colour emits a `STATE_CHANGE` event. `SET_STATE` is allowed
from any state to any state (shown partially for readability); when auto mode is on it restarts the
phase timer.
