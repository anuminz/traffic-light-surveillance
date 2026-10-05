#include "EventLogger.hpp"
#include <cstdio>
#include <ctime>
#include <iostream>
#include <stdexcept>

namespace tls {

const char* stateName(uint32_t s) {
    switch (s) {
        case TL_RED:    return "RED";
        case TL_GREEN:  return "GREEN";
        case TL_YELLOW: return "YELLOW";
        default:        return "UNKNOWN";
    }
}

const char* eventName(uint32_t t) {
    switch (t) {
        case TL_EVT_STATE_CHANGE: return "STATE_CHANGE";
        case TL_EVT_VIOLATION:    return "VIOLATION";
        default:                  return "UNKNOWN";
    }
}

std::string formatTimestamp(uint64_t ts_ns) {
    const std::time_t secs = static_cast<std::time_t>(ts_ns / 1000000000ULL);
    const unsigned ms = static_cast<unsigned>((ts_ns / 1000000ULL) % 1000ULL);
    std::tm tm{};
    gmtime_r(&secs, &tm);
    char base[32];
    std::strftime(base, sizeof(base), "%Y-%m-%dT%H:%M:%S", &tm);
    char out[48];
    std::snprintf(out, sizeof(out), "%s.%03uZ", base, ms);
    return out;
}

std::string formatEvent(const tl_event& e) {
    return formatTimestamp(e.ts_ns) + "," + eventName(e.type) + "," +
           stateName(e.state) + "," + std::to_string(e.violations);
}

EventLogger::EventLogger(const std::string& path, bool echo) : echo_(echo) {
    if (path.empty()) return;
    bool needHeader;
    {
        std::ifstream probe(path);
        needHeader = !probe || probe.peek() == std::ifstream::traits_type::eof();
    }
    file_.open(path, std::ios::app);
    if (!file_) throw std::runtime_error("cannot open log file: " + path);
    if (needHeader) file_ << "timestamp,event,state,violations\n" << std::flush;
}

void EventLogger::write(const std::string& line) {
    std::lock_guard<std::mutex> lk(mutex_);
    if (file_.is_open()) file_ << line << '\n' << std::flush;
    if (echo_) std::cout << line << std::endl;
}

void EventLogger::log(const tl_event& e) { write(formatEvent(e)); }

void EventLogger::info(const std::string& message) { write("# " + message); }

}  // namespace tls
