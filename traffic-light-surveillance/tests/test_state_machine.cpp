#include "test_framework.hpp"
#include "TrafficStateMachine.hpp"

using tls::TrafficStateMachine;

TEST(sm_initial_state_is_red_and_auto) {
    TrafficStateMachine sm;
    auto st = sm.status();
    CHECK_EQ(st.state, (uint32_t)TL_RED);
    CHECK_EQ(st.auto_mode, 1u);
    CHECK(sm.drainEvents().empty());
}

TEST(sm_cycles_red_green_yellow_red) {
    TrafficStateMachine sm;
    sm.advance(4000); CHECK_EQ(sm.status().state, (uint32_t)TL_GREEN);
    sm.advance(4000); CHECK_EQ(sm.status().state, (uint32_t)TL_YELLOW);
    sm.advance(1500); CHECK_EQ(sm.status().state, (uint32_t)TL_RED);
}

TEST(sm_no_change_before_phase_ends) {
    TrafficStateMachine sm;
    sm.advance(3999);
    CHECK_EQ(sm.status().state, (uint32_t)TL_RED);
    CHECK(sm.drainEvents().empty());
}

TEST(sm_large_step_emits_one_event_per_transition) {
    TrafficStateMachine sm;
    sm.advance(4000 + 4000 + 1500);
    auto ev = sm.drainEvents();
    CHECK_EQ(ev.size(), 3u);
    CHECK_EQ(ev[0].state, (uint32_t)TL_GREEN);
    CHECK_EQ(ev[1].state, (uint32_t)TL_YELLOW);
    CHECK_EQ(ev[2].state, (uint32_t)TL_RED);
}

TEST(sm_event_timestamps_are_exact_and_monotonic) {
    TrafficStateMachine sm(1000000000ULL);
    sm.advance(9500);
    auto ev = sm.drainEvents();
    CHECK_EQ(ev[0].ts_ns, 1000000000ULL + 4000ULL * 1000000ULL);
    CHECK_EQ(ev[1].ts_ns, 1000000000ULL + 8000ULL * 1000000ULL);
    CHECK_EQ(ev[2].ts_ns, 1000000000ULL + 9500ULL * 1000000ULL);
}

TEST(sm_vehicle_on_red_is_violation) {
    TrafficStateMachine sm;
    sm.vehicleArrived();
    auto st = sm.status();
    CHECK_EQ(st.violations, 1u);
    CHECK_EQ(st.vehicles, 1u);
    auto ev = sm.drainEvents();
    CHECK_EQ(ev.size(), 1u);
    CHECK_EQ(ev[0].type, (uint32_t)TL_EVT_VIOLATION);
}

TEST(sm_vehicle_on_green_or_yellow_is_legal) {
    TrafficStateMachine sm;
    sm.setState(TL_GREEN);  sm.vehicleArrived();
    sm.setState(TL_YELLOW); sm.vehicleArrived();
    CHECK_EQ(sm.status().violations, 0u);
    CHECK_EQ(sm.status().vehicles, 2u);
}

TEST(sm_manual_mode_freezes_state) {
    TrafficStateMachine sm;
    sm.setAuto(false);
    sm.advance(60000);
    CHECK_EQ(sm.status().state, (uint32_t)TL_RED);
    CHECK(sm.drainEvents().empty());
}

TEST(sm_reenabling_auto_restarts_phase_timer) {
    TrafficStateMachine sm;
    sm.setAuto(false);
    sm.advance(10000);
    sm.setAuto(true);
    sm.advance(3999);
    CHECK_EQ(sm.status().state, (uint32_t)TL_RED);
    sm.advance(1);
    CHECK_EQ(sm.status().state, (uint32_t)TL_GREEN);
}

TEST(sm_set_state_rejects_invalid) {
    TrafficStateMachine sm;
    CHECK(!sm.setState(3));
    CHECK(sm.setState(TL_YELLOW));
    CHECK_EQ(sm.status().state, (uint32_t)TL_YELLOW);
}

TEST(sm_config_validation) {
    TrafficStateMachine sm;
    CHECK(!sm.setConfig({50, 1000, 1000}));
    CHECK(!sm.setConfig({1000, 1000, 700000}));
    CHECK(sm.setConfig({200, 300, 400}));
    sm.advance(200);
    CHECK_EQ(sm.status().state, (uint32_t)TL_GREEN);
}

TEST(sm_reset_stats) {
    TrafficStateMachine sm;
    sm.vehicleArrived();
    sm.resetStats();
    CHECK_EQ(sm.status().violations, 0u);
    CHECK_EQ(sm.status().vehicles, 0u);
}
