#pragma once
// Pure, deterministic traffic-light logic (no I/O, no real clock).
// Mirrors the behaviour of the kernel driver so it can be unit-tested.
#include <cstdint>
#include <vector>
#include "tlight_uapi.h"

namespace tls {

constexpr uint32_t kMinPhaseMs = 100;
constexpr uint32_t kMaxPhaseMs = 600000;

class TrafficStateMachine {
public:
    explicit TrafficStateMachine(uint64_t base_ns = 0);

    bool setConfig(const tl_config& cfg);   // false if out of range
    void setAuto(bool on);
    bool setState(uint32_t state);          // false if invalid
    void advance(uint32_t ms);              // move simulated time forward
    void vehicleArrived();
    void resetStats();

    tl_status status() const;
    std::vector<tl_event> drainEvents();

private:
    uint32_t phaseMs(uint32_t s) const;
    void push(uint32_t type);

    tl_config cfg_{4000, 4000, 1500};
    uint32_t state_ = TL_RED;
    bool     auto_  = true;
    uint32_t elapsed_ms_ = 0;     // time spent in current state
    uint32_t violations_ = 0;
    uint32_t vehicles_   = 0;
    uint64_t base_ns_;
    uint64_t clock_ns_ = 0;
    std::vector<tl_event> events_;
};

}  // namespace tls
