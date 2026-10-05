#pragma once
// Reads events from a device and forwards them to the logger and statistics.
#include <atomic>
#include "EventLogger.hpp"
#include "IDevice.hpp"
#include "Statistics.hpp"

namespace tls {

class Monitor {
public:
    Monitor(IDevice& dev, EventLogger& logger, Statistics& stats)
        : dev_(dev), logger_(logger), stats_(stats) {}

    int stepOnce(int timeout_ms);                 // returns #events, <0 on error
    int run(const std::atomic<bool>& stop);       // 0 = clean stop, 1 = device failure

private:
    IDevice& dev_;
    EventLogger& logger_;
    Statistics& stats_;
};

}  // namespace tls
