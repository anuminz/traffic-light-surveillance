#include "ControlServer.hpp"
#include "EventLogger.hpp"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <poll.h>
#include <sstream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace tls {

namespace {

std::string upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return s;
}

bool parseState(const std::string& token, uint32_t& out) {
    const std::string t = upper(token);
    if (t == "R" || t == "RED")    { out = TL_RED;    return true; }
    if (t == "G" || t == "GREEN")  { out = TL_GREEN;  return true; }
    if (t == "Y" || t == "YELLOW") { out = TL_YELLOW; return true; }
    return false;
}

std::string statusLine(IDevice& dev, Statistics& stats) {
    tl_status st{};
    if (!dev.getStatus(st)) return "ERR cannot read device status";
    return std::string("OK state=") + stateName(st.state) +
           " auto=" + std::to_string(st.auto_mode) +
           " vehicles=" + std::to_string(st.vehicles) +
           " violations=" + std::to_string(st.violations) +
           " red_ms=" + std::to_string(st.cfg.red_ms) +
           " green_ms=" + std::to_string(st.cfg.green_ms) +
           " yellow_ms=" + std::to_string(st.cfg.yellow_ms) +
           " | monitor: " + stats.toText();
}

}  // namespace

std::string ControlServer::handleCommand(IDevice& dev, Statistics& stats, const std::string& line) {
    std::istringstream in(line);
    std::string cmd;
    in >> cmd;
    cmd = upper(cmd);

    if (cmd == "STATUS") return statusLine(dev, stats);
    if (cmd == "STATS")  return "OK " + stats.toText();
    if (cmd == "VEHICLE") return dev.simulateVehicle() ? "OK vehicle detected" : "ERR device failure";
    if (cmd == "RESET")   return dev.resetStats() ? "OK statistics reset" : "ERR device failure";

    if (cmd == "STATE") {
        std::string tok;
        uint32_t s;
        if (!(in >> tok) || !parseState(tok, s)) return "ERR usage: STATE R|G|Y";
        return dev.setState(s) ? std::string("OK state=") + stateName(s) : "ERR device rejected state";
    }
    if (cmd == "AUTO") {
        std::string tok;
        if (!(in >> tok)) return "ERR usage: AUTO 0|1";
        tok = upper(tok);
        if (tok == "1" || tok == "ON")  return dev.setAuto(true)  ? "OK auto=1" : "ERR device failure";
        if (tok == "0" || tok == "OFF") return dev.setAuto(false) ? "OK auto=0" : "ERR device failure";
        return "ERR usage: AUTO 0|1";
    }
    if (cmd == "CONFIG") {
        long r, g, y;
        if (!(in >> r >> g >> y)) return "ERR usage: CONFIG <red_ms> <green_ms> <yellow_ms>";
        if (r < 0 || g < 0 || y < 0) return "ERR values must be positive";
        tl_config c{static_cast<uint32_t>(r), static_cast<uint32_t>(g), static_cast<uint32_t>(y)};
        return dev.setConfig(c) ? "OK config updated" : "ERR config out of range (100..600000 ms)";
    }
    if (cmd == "HELP")
        return "OK commands: STATUS STATS STATE <R|G|Y> AUTO <0|1> VEHICLE CONFIG <r> <g> <y> RESET";
    return "ERR unknown command (try HELP)";
}

ControlServer::ControlServer(IDevice& dev, Statistics& stats, std::string socketPath)
    : dev_(dev), stats_(stats), path_(std::move(socketPath)) {}

ControlServer::~ControlServer() { stop(); }

bool ControlServer::start(std::string& error) {
    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    if (path_.size() >= sizeof(addr.sun_path)) { error = "socket path too long"; return false; }
    std::strncpy(addr.sun_path, path_.c_str(), sizeof(addr.sun_path) - 1);

    listenFd_ = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (listenFd_ < 0) { error = std::strerror(errno); return false; }
    ::unlink(path_.c_str());
    if (::bind(listenFd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0 ||
        ::listen(listenFd_, 8) < 0) {
        error = std::string("bind/listen ") + path_ + ": " + std::strerror(errno);
        ::close(listenFd_);
        listenFd_ = -1;
        return false;
    }
    stop_ = false;
    thread_ = std::thread(&ControlServer::serve, this);
    return true;
}

void ControlServer::stop() {
    stop_ = true;
    if (thread_.joinable()) thread_.join();
    if (listenFd_ >= 0) {
        ::close(listenFd_);
        listenFd_ = -1;
        ::unlink(path_.c_str());
    }
}

void ControlServer::serve() {
    while (!stop_.load()) {
        pollfd p{listenFd_, POLLIN, 0};
        if (::poll(&p, 1, 200) <= 0) continue;
        const int c = ::accept4(listenFd_, nullptr, nullptr, SOCK_CLOEXEC);
        if (c < 0) continue;

        timeval tv{2, 0};
        ::setsockopt(c, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        char buf[256];
        const ssize_t n = ::recv(c, buf, sizeof(buf) - 1, 0);
        if (n > 0) {
            std::string line(buf, static_cast<size_t>(n));
            while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.pop_back();
            const std::string reply = handleCommand(dev_, stats_, line) + "\n";
            ::send(c, reply.data(), reply.size(), MSG_NOSIGNAL);
        }
        ::close(c);
    }
}

}  // namespace tls
