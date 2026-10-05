// tlightctl - command line client for tlightd.
// Example: tlightctl STATUS | tlightctl STATE G | tlightctl VEHICLE
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int main(int argc, char** argv) {
    std::string path = std::getenv("TLIGHT_SOCK") ? std::getenv("TLIGHT_SOCK") : "/tmp/tlightd.sock";
    int first = 1;
    if (argc > 2 && std::string(argv[1]) == "-s") { path = argv[2]; first = 3; }
    if (first >= argc) {
        std::cerr << "usage: " << argv[0] << " [-s socket] COMMAND [args]\n"
                  << "commands: STATUS STATS STATE <R|G|Y> AUTO <0|1> VEHICLE CONFIG <r> <g> <y> RESET HELP\n";
        return 2;
    }

    std::string line;
    for (int i = first; i < argc; ++i) { if (i > first) line += ' '; line += argv[i]; }
    line += '\n';

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    if (path.size() >= sizeof(addr.sun_path)) { std::cerr << "socket path too long\n"; return 2; }
    std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

    const int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0 || ::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "cannot connect to " << path << " (is tlightd running?): "
                  << std::strerror(errno) << "\n";
        return 1;
    }
    ::send(fd, line.data(), line.size(), 0);

    std::string reply;
    char buf[512];
    ssize_t n;
    while ((n = ::recv(fd, buf, sizeof(buf), 0)) > 0) reply.append(buf, static_cast<size_t>(n));
    ::close(fd);

    std::cout << reply;
    return reply.rfind("OK", 0) == 0 ? 0 : 1;
}
