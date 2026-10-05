#pragma once
// Abstract interface to a traffic-light device. Implemented by the real
// kernel driver wrapper (TLightDevice) and a pure user-space simulator
// (SimDevice), so every other component can be tested without the kernel.
#include <cstdint>
#include <vector>
#include "tlight_uapi.h"

namespace tls {

class IDevice {
public:
    virtual ~IDevice() = default;

    // Wait up to timeout_ms for events and append them to `out`.
    // Returns number of events (0 on timeout) or -1 on error.
    virtual int  readEvents(std::vector<tl_event>& out, int timeout_ms) = 0;
    virtual bool getStatus(tl_status& st) = 0;
    virtual bool setState(uint32_t state) = 0;
    virtual bool setAuto(bool on) = 0;
    virtual bool setConfig(const tl_config& cfg) = 0;
    virtual bool simulateVehicle() = 0;
    virtual bool resetStats() = 0;
};

}  // namespace tls
