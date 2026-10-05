#include "test_framework.hpp"
#include "ControlServer.hpp"
#include "EventLogger.hpp"
#include "Monitor.hpp"
#include "SimDevice.hpp"
#include "Statistics.hpp"

using namespace tls;

static tl_event ev(uint64_t ts, uint32_t type, uint32_t state, uint32_t viol) {
    tl_event e{};
    e.ts_ns = ts; e.type = type; e.state = state; e.violations = viol;
    return e;
}

// ---- EventLogger formatting -------------------------------------------
TEST(fmt_timestamp_epoch) {
    CHECK_EQ(formatTimestamp(0), std::string("1970-01-01T00:00:00.000Z"));
}
TEST(fmt_timestamp_known_value) {
    CHECK_EQ(formatTimestamp(1700000000123000000ULL), std::string("2023-11-14T22:13:20.123Z"));
}
TEST(fmt_event_state_change) {
    CHECK_EQ(formatEvent(ev(0, TL_EVT_STATE_CHANGE, TL_GREEN, 0)),
             std::string("1970-01-01T00:00:00.000Z,STATE_CHANGE,GREEN,0"));
}
TEST(fmt_event_violation) {
    CHECK_EQ(formatEvent(ev(0, TL_EVT_VIOLATION, TL_RED, 3)),
             std::string("1970-01-01T00:00:00.000Z,VIOLATION,RED,3"));
}

// ---- Statistics --------------------------------------------------------
TEST(stats_counts_events) {
    Statistics s;
    s.onEvent(ev(1, TL_EVT_STATE_CHANGE, TL_GREEN, 0));
    s.onEvent(ev(2, TL_EVT_STATE_CHANGE, TL_YELLOW, 0));
    s.onEvent(ev(3, TL_EVT_STATE_CHANGE, TL_RED, 0));
    s.onEvent(ev(4, TL_EVT_VIOLATION, TL_RED, 1));
    auto snap = s.snapshot();
    CHECK_EQ(snap.events, 4u);
    CHECK_EQ(snap.state_changes, 3u);
    CHECK_EQ(snap.cycles, 1u);
    CHECK_EQ(snap.violations, 1u);
    CHECK_EQ(snap.last_violation_ns, 4u);
    CHECK_EQ(snap.current_state, (uint32_t)TL_RED);
}

// ---- Monitor + SimDevice ----------------------------------------------
TEST(monitor_forwards_events_to_statistics) {
    SimDevice dev(false);
    EventLogger logger("", false);
    Statistics stats;
    Monitor mon(dev, logger, stats);
    dev.advance(4000);
    dev.setState(TL_RED);
    dev.simulateVehicle();
    CHECK(mon.stepOnce(0) >= 3);
    CHECK_EQ(stats.snapshot().violations, 1u);
}

TEST(monitor_returns_zero_on_timeout) {
    SimDevice dev(false);
    EventLogger logger("", false);
    Statistics stats;
    Monitor mon(dev, logger, stats);
    CHECK_EQ(mon.stepOnce(0), 0);
}

// ---- Command interpreter ----------------------------------------------
static std::string cmd(SimDevice& d, Statistics& s, const char* line) {
    return ControlServer::handleCommand(d, s, line);
}
static bool starts(const std::string& s, const char* p) { return s.rfind(p, 0) == 0; }

TEST(cmd_status_ok) {
    SimDevice d(false); Statistics s;
    auto r = cmd(d, s, "STATUS");
    CHECK(starts(r, "OK state=RED"));
}
TEST(cmd_state_changes_light) {
    SimDevice d(false); Statistics s;
    CHECK(starts(cmd(d, s, "state g"), "OK"));
    tl_status st{}; d.getStatus(st);
    CHECK_EQ(st.state, (uint32_t)TL_GREEN);
}
TEST(cmd_state_rejects_garbage) {
    SimDevice d(false); Statistics s;
    CHECK(starts(cmd(d, s, "STATE PURPLE"), "ERR"));
    CHECK(starts(cmd(d, s, "STATE"), "ERR"));
}
TEST(cmd_vehicle_on_red_counts_violation) {
    SimDevice d(false); Statistics s;
    CHECK(starts(cmd(d, s, "VEHICLE"), "OK"));
    tl_status st{}; d.getStatus(st);
    CHECK_EQ(st.violations, 1u);
}
TEST(cmd_auto_toggle) {
    SimDevice d(false); Statistics s;
    CHECK(starts(cmd(d, s, "AUTO 0"), "OK auto=0"));
    tl_status st{}; d.getStatus(st);
    CHECK_EQ(st.auto_mode, 0u);
}
TEST(cmd_config_valid_and_invalid) {
    SimDevice d(false); Statistics s;
    CHECK(starts(cmd(d, s, "CONFIG 1000 2000 500"), "OK"));
    CHECK(starts(cmd(d, s, "CONFIG 1 2 3"), "ERR"));
    CHECK(starts(cmd(d, s, "CONFIG 1000"), "ERR"));
}
TEST(cmd_reset_clears_counters) {
    SimDevice d(false); Statistics s;
    cmd(d, s, "VEHICLE");
    cmd(d, s, "RESET");
    tl_status st{}; d.getStatus(st);
    CHECK_EQ(st.violations, 0u);
}
TEST(cmd_unknown_command) {
    SimDevice d(false); Statistics s;
    CHECK(starts(cmd(d, s, "FLY"), "ERR"));
    CHECK(starts(cmd(d, s, ""), "ERR"));
}
