#pragma once
// UNIX-domain-socket control interface used by tlightctl.
// Protocol: one text line per connection, one text line reply ("OK ..." / "ERR ...").
#include <atomic>
#include <string>
#include <thread>
#include "IDevice.hpp"
#include "Statistics.hpp"

namespace tls {

class ControlServer {
public:
    ControlServer(IDevice& dev, Statistics& stats, std::string socketPath);
    ~ControlServer();

    bool start(std::string& error);
    void stop();

    // Pure command interpreter (unit-testable without sockets).
    static std::string handleCommand(IDevice& dev, Statistics& stats, const std::string& line);

private:
    void serve();

    IDevice& dev_;
    Statistics& stats_;
    std::string path_;
    int listenFd_ = -1;
    std::atomic<bool> stop_{false};
    std::thread thread_;
};

}  // namespace tls
