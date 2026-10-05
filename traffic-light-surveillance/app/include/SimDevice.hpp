#pragma once
// User-space simulator of the driver. Lets the whole system run (and be
// tested) on machines where the kernel module cannot be loaded.
#include <chrono>
#include <mutex>
#include "IDevice.hpp"
#include "TrafficStateMachine.hpp"

namespace tls {

class SimDevice : public IDevice {
public:
    // realtime=true : simulated time follows the wall clock.
    // realtime=false: time only moves when advance() is called (for tests).
    explicit SimDevice(bool realtime = true);

    int  readEvents(std::vector<tl_event>& out, int timeout_ms) override;
    bool getStatus(tl_status& st) override;
    bool setState(uint32_t state) override;
    bool setAuto(bool on) override;
    bool setConfig(const tl_config& cfg) override;
    bool simulateVehicle() override;
    bool resetStats() override;

    void advance(uint32_t ms);   // manual time step (non-realtime mode)

private:
    void syncLocked();           // realtime catch-up; caller holds mutex_

    std::mutex mutex_;
    TrafficStateMachine sm_;
    bool realtime_;
    std::chrono::steady_clock::time_point last_;
};

}  // namespace tls
