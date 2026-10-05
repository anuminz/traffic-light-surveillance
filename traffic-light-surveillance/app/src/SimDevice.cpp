#include "SimDevice.hpp"
#include <algorithm>
#include <thread>

namespace tls {

using Clock = std::chrono::steady_clock;

namespace {
uint64_t wallNowNs() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
}
}  // namespace

SimDevice::SimDevice(bool realtime)
    : sm_(realtime ? wallNowNs() : 0), realtime_(realtime), last_(Clock::now()) {}

void SimDevice::syncLocked() {
    if (!realtime_) return;
    const auto now = Clock::now();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_).count();
    if (ms > 0) {
        sm_.advance(static_cast<uint32_t>(ms));
        last_ += std::chrono::milliseconds(ms);   // keep sub-ms remainder
    }
}

void SimDevice::advance(uint32_t ms) {
    std::lock_guard<std::mutex> lk(mutex_);
    sm_.advance(ms);
}

int SimDevice::readEvents(std::vector<tl_event>& out, int timeout_ms) {
    const auto deadline = Clock::now() + std::chrono::milliseconds(timeout_ms);
    for (;;) {
        {
            std::lock_guard<std::mutex> lk(mutex_);
            syncLocked();
            auto ev = sm_.drainEvents();
            if (!ev.empty()) {
                out.insert(out.end(), ev.begin(), ev.end());
                return static_cast<int>(ev.size());
            }
        }
        const auto now = Clock::now();
        if (now >= deadline) return 0;
        std::this_thread::sleep_for(std::min<Clock::duration>(
            std::chrono::milliseconds(10), deadline - now));
    }
}

bool SimDevice::getStatus(tl_status& st) {
    std::lock_guard<std::mutex> lk(mutex_);
    syncLocked();
    st = sm_.status();
    return true;
}

bool SimDevice::setState(uint32_t state) {
    std::lock_guard<std::mutex> lk(mutex_);
    syncLocked();
    return sm_.setState(state);
}

bool SimDevice::setAuto(bool on) {
    std::lock_guard<std::mutex> lk(mutex_);
    syncLocked();
    sm_.setAuto(on);
    return true;
}

bool SimDevice::setConfig(const tl_config& cfg) {
    std::lock_guard<std::mutex> lk(mutex_);
    syncLocked();
    return sm_.setConfig(cfg);
}

bool SimDevice::simulateVehicle() {
    std::lock_guard<std::mutex> lk(mutex_);
    syncLocked();
    sm_.vehicleArrived();
    return true;
}

bool SimDevice::resetStats() {
    std::lock_guard<std::mutex> lk(mutex_);
    sm_.resetStats();
    return true;
}

}  // namespace tls
