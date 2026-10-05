#include "TrafficStateMachine.hpp"

namespace tls {

namespace {
uint32_t nextState(uint32_t s) {
    switch (s) {
        case TL_RED:   return TL_GREEN;
        case TL_GREEN: return TL_YELLOW;
        default:       return TL_RED;
    }
}
bool validPhase(uint32_t ms) { return ms >= kMinPhaseMs && ms <= kMaxPhaseMs; }
}  // namespace

TrafficStateMachine::TrafficStateMachine(uint64_t base_ns) : base_ns_(base_ns) {}

uint32_t TrafficStateMachine::phaseMs(uint32_t s) const {
    switch (s) {
        case TL_RED:   return cfg_.red_ms;
        case TL_GREEN: return cfg_.green_ms;
        default:       return cfg_.yellow_ms;
    }
}

void TrafficStateMachine::push(uint32_t type) {
    tl_event e{};
    e.ts_ns      = base_ns_ + clock_ns_;
    e.type       = type;
    e.state      = state_;
    e.violations = violations_;
    events_.push_back(e);
}

bool TrafficStateMachine::setConfig(const tl_config& cfg) {
    if (!validPhase(cfg.red_ms) || !validPhase(cfg.green_ms) || !validPhase(cfg.yellow_ms))
        return false;
    cfg_ = cfg;
    return true;
}

void TrafficStateMachine::setAuto(bool on) {
    if (on != auto_) elapsed_ms_ = 0;   // restart the current phase timer
    auto_ = on;
}

bool TrafficStateMachine::setState(uint32_t state) {
    if (state > TL_YELLOW) return false;
    state_ = state;
    elapsed_ms_ = 0;
    push(TL_EVT_STATE_CHANGE);
    return true;
}

void TrafficStateMachine::advance(uint32_t ms) {
    uint32_t remaining = ms;
    while (remaining > 0) {
        if (!auto_) {
            clock_ns_  += static_cast<uint64_t>(remaining) * 1000000ULL;
            elapsed_ms_ += remaining;
            return;
        }
        const uint32_t dur  = phaseMs(state_);
        const uint32_t need = elapsed_ms_ >= dur ? 0 : dur - elapsed_ms_;
        if (remaining < need) {
            clock_ns_  += static_cast<uint64_t>(remaining) * 1000000ULL;
            elapsed_ms_ += remaining;
            return;
        }
        remaining -= need;
        clock_ns_ += static_cast<uint64_t>(need) * 1000000ULL;
        state_ = nextState(state_);
        elapsed_ms_ = 0;
        push(TL_EVT_STATE_CHANGE);
    }
}

void TrafficStateMachine::vehicleArrived() {
    ++vehicles_;
    if (state_ == TL_RED) {
        ++violations_;
        push(TL_EVT_VIOLATION);
    }
}

void TrafficStateMachine::resetStats() {
    violations_ = 0;
    vehicles_ = 0;
}

tl_status TrafficStateMachine::status() const {
    tl_status st{};
    st.state      = state_;
    st.auto_mode  = auto_ ? 1 : 0;
    st.violations = violations_;
    st.vehicles   = vehicles_;
    st.cfg        = cfg_;
    return st;
}

std::vector<tl_event> TrafficStateMachine::drainEvents() {
    std::vector<tl_event> out;
    out.swap(events_);
    return out;
}

}  // namespace tls
