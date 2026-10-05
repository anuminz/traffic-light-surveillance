#!/usr/bin/env bash
# End-to-end test: tlightd + tlightctl (+ optionally the real kernel driver).
#   ./scripts/integration_test.sh         -> uses the user-space simulator
#   sudo insmod driver/tlight.ko; ./scripts/integration_test.sh --hw
set -u
cd "$(dirname "$0")/.."
BIN=build
MODE=sim; [[ "${1:-}" == "--hw" ]] && MODE=hw
SOCK=$(mktemp -u /tmp/tlightd.XXXXXX.sock)
LOG=$(mktemp /tmp/tlight.XXXXXX.csv)
PASS=0; FAIL=0

check() {   # check "description" command...
    local desc=$1; shift
    if "$@" >/dev/null 2>&1; then echo "[ PASS ] $desc"; PASS=$((PASS+1))
    else echo "[ FAIL ] $desc"; FAIL=$((FAIL+1)); fi
}
ctl() { "$BIN/tlightctl" -s "$SOCK" "$@"; }

if [[ $MODE == hw ]]; then
    [[ -c /dev/tlight ]] || { echo "/dev/tlight missing: sudo insmod driver/tlight.ko"; exit 1; }
    "$BIN/tlightd" --device /dev/tlight --socket "$SOCK" --log "$LOG" --quiet &
else
    "$BIN/tlightd" --sim --socket "$SOCK" --log "$LOG" --quiet &
fi
PID=$!
for _ in $(seq 1 30); do [[ -S $SOCK ]] && break; sleep 0.1; done

check "daemon created control socket"        test -S "$SOCK"
check "STATUS answers OK"                    ctl STATUS
ctl AUTO 0 >/dev/null
ctl STATE R >/dev/null
check "manual RED reported"                  bash -c "'$BIN/tlightctl' -s '$SOCK' STATUS | grep -q 'state=RED auto=0'"
ctl VEHICLE >/dev/null
check "vehicle on RED = 1 violation"         bash -c "'$BIN/tlightctl' -s '$SOCK' STATUS | grep -q 'violations=1 '"
ctl STATE G >/dev/null
ctl VEHICLE >/dev/null
check "vehicle on GREEN is not a violation"  bash -c "'$BIN/tlightctl' -s '$SOCK' STATUS | grep -q 'violations=1 '"
check "invalid state rejected"               bash -c "! '$BIN/tlightctl' -s '$SOCK' STATE PURPLE"
check "out-of-range config rejected"         bash -c "! '$BIN/tlightctl' -s '$SOCK' CONFIG 1 1 1"
check "valid config accepted"                ctl CONFIG 200 200 200
ctl STATE R >/dev/null
ctl AUTO 1 >/dev/null
sleep 1.6
CHANGES=$(grep -c STATE_CHANGE "$LOG")
check "auto cycling logged >= 5 state changes (got $CHANGES)" test "$CHANGES" -ge 5
check "violation event in CSV log"           grep -q ',VIOLATION,RED,1' "$LOG"
check "CSV header present"                   grep -q '^timestamp,event,state,violations' "$LOG"

kill -TERM "$PID"; wait "$PID"; RC=$?
check "daemon exits 0 on SIGTERM (rc=$RC)"   test "$RC" -eq 0
check "control socket removed on shutdown"   test ! -e "$SOCK"
check "shutdown summary written to log"      grep -q 'tlightd stopping' "$LOG"

echo; echo "Result: $PASS passed, $FAIL failed   (log: $LOG)"
[[ $FAIL -eq 0 ]]
