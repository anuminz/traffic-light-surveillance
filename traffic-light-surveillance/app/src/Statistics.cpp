#include "Statistics.hpp"
#include "EventLogger.hpp"

namespace tls {

void Statistics::onEvent(const tl_event& e) {
    std::lock_guard<std::mutex> lk(mutex_);
    ++s_.events;
    if (e.type == TL_EVT_STATE_CHANGE) {
        ++s_.state_changes;
        s_.current_state = e.state;
        if (e.state == TL_RED) ++s_.cycles;
    } else if (e.type == TL_EVT_VIOLATION) {
        ++s_.violations;
        s_.last_violation_ns = e.ts_ns;
    }
}

Statistics::Snapshot Statistics::snapshot() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return s_;
}

std::string Statistics::toText() const {
    const Snapshot s = snapshot();
    std::string out = "events=" + std::to_string(s.events) +
                      " state_changes=" + std::to_string(s.state_changes) +
                      " cycles=" + std::to_string(s.cycles) +
                      " violations=" + std::to_string(s.violations) +
                      " current=" + stateName(s.current_state);
    if (s.violations > 0) out += " last_violation=" + formatTimestamp(s.last_violation_ns);
    return out;
}

}  // namespace tls
