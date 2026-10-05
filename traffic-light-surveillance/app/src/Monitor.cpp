#include "Monitor.hpp"
#include <chrono>
#include <thread>
#include <vector>

namespace tls {

int Monitor::stepOnce(int timeout_ms) {
    std::vector<tl_event> events;
    const int n = dev_.readEvents(events, timeout_ms);
    for (const auto& e : events) {
        stats_.onEvent(e);
        logger_.log(e);
    }
    return n;
}

int Monitor::run(const std::atomic<bool>& stop) {
    int consecutiveErrors = 0;
    while (!stop.load()) {
        if (stepOnce(500) < 0) {
            logger_.info("device read error");
            if (++consecutiveErrors > 5) return 1;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } else {
            consecutiveErrors = 0;
        }
    }
    return 0;
}

}  // namespace tls
