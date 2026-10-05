// tlightd - traffic light surveillance daemon.
// Monitors /dev/tlight (or a built-in simulator), logs events to CSV and
// serves control commands on a UNIX socket for tlightctl.
#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>

#include "ControlServer.hpp"
#include "EventLogger.hpp"
#include "Monitor.hpp"
#include "SimDevice.hpp"
#include "Statistics.hpp"
#include "TLightDevice.hpp"

static_assert(ATOMIC_BOOL_LOCK_FREE == 2, "atomic<bool> must be lock-free for signal use");
static std::atomic<bool> g_stop{false};

static void onSignal(int) { g_stop.store(true); }

static void usage(const char* prog) {
    std::cout <<
        "Usage: " << prog << " [options]\n"
        "  --device PATH   device node (default /dev/tlight)\n"
        "  --sim           use the built-in user-space simulator (no kernel module)\n"
        "  --log FILE      CSV event log (default tlight_events.csv)\n"
        "  --socket PATH   control socket (default /tmp/tlightd.sock)\n"
        "  --quiet         do not echo events to stdout\n"
        "  --daemon        run in the background\n"
        "  --help          show this help\n";
}

int main(int argc, char** argv) {
    std::string devicePath = "/dev/tlight";
    std::string logPath = "tlight_events.csv";
    std::string sockPath = "/tmp/tlightd.sock";
    bool sim = false, quiet = false, daemonize = false;

    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto next = [&](std::string& dst) {
            if (i + 1 >= argc) { std::cerr << a << " needs a value\n"; std::exit(2); }
            dst = argv[++i];
        };
        if (a == "--device") next(devicePath);
        else if (a == "--log") next(logPath);
        else if (a == "--socket") next(sockPath);
        else if (a == "--sim") sim = true;
        else if (a == "--quiet") quiet = true;
        else if (a == "--daemon") daemonize = true;
        else if (a == "--help" || a == "-h") { usage(argv[0]); return 0; }
        else { std::cerr << "unknown option: " << a << "\n"; usage(argv[0]); return 2; }
    }

    // Signal handling: graceful shutdown on SIGINT/SIGTERM, ignore SIGPIPE.
    struct sigaction sa{};
    sa.sa_handler = onSignal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    std::signal(SIGPIPE, SIG_IGN);

    std::unique_ptr<tls::IDevice> dev;
    try {
        if (sim) dev = std::make_unique<tls::SimDevice>(true);
        else     dev = std::make_unique<tls::TLightDevice>(devicePath);
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n"
                  << "hint: load the driver (sudo insmod driver/tlight.ko) or use --sim\n";
        return 1;
    }

    if (daemonize) {
        if (logPath[0] != '/' || sockPath[0] != '/') {
            std::cerr << "--daemon requires absolute --log and --socket paths\n";
            return 2;
        }
        if (::daemon(1, 0) != 0) { std::perror("daemon"); return 1; }
        quiet = true;
    }

    try {
        tls::EventLogger logger(logPath, !quiet);
        tls::Statistics stats;
        tls::Monitor monitor(*dev, logger, stats);
        tls::ControlServer server(*dev, stats, sockPath);

        std::string err;
        if (!server.start(err)) { std::cerr << "control server: " << err << "\n"; return 1; }
        logger.info(std::string("tlightd started, source=") + (sim ? "simulator" : devicePath));

        const int rc = monitor.run(g_stop);

        logger.info("tlightd stopping: " + stats.toText());
        server.stop();
        return rc;
    } catch (const std::exception& e) {
        std::cerr << "fatal: " << e.what() << "\n";
        return 1;
    }
}
