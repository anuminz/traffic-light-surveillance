#pragma once
// Thread-safe CSV event logger: ts,event,state,violations
#include <cstdint>
#include <fstream>
#include <mutex>
#include <string>
#include "tlight_uapi.h"

namespace tls {

const char* stateName(uint32_t state);
const char* eventName(uint32_t type);
std::string formatTimestamp(uint64_t ts_ns);            // ISO-8601 UTC, ms precision
std::string formatEvent(const tl_event& e);             // one CSV line, no newline

class EventLogger {
public:
    // path may be empty (no file). echo=true also prints to stdout.
    EventLogger(const std::string& path, bool echo);
    void log(const tl_event& e);
    void info(const std::string& message);              // written as "# message"

private:
    void write(const std::string& line);
    std::mutex mutex_;
    std::ofstream file_;
    bool echo_;
};

}  // namespace tls
