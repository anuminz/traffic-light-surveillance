#pragma once
#include <cstdint>
#include <mutex>
#include <string>
#include "tlight_uapi.h"

namespace tls {

class Statistics {
public:
    struct Snapshot {
        uint64_t events = 0;
        uint64_t state_changes = 0;
        uint64_t cycles = 0;          // number of times RED was entered
        uint64_t violations = 0;
        uint64_t last_violation_ns = 0;
        uint32_t current_state = TL_RED;
    };

    void onEvent(const tl_event& e);
    Snapshot snapshot() const;
    std::string toText() const;

private:
    mutable std::mutex mutex_;
    Snapshot s_;
};

}  // namespace tls
