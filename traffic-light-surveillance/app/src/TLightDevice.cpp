#include "TLightDevice.hpp"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <stdexcept>
#include <sys/ioctl.h>
#include <unistd.h>

namespace tls {

TLightDevice::TLightDevice(const std::string& path) {
    fd_ = ::open(path.c_str(), O_RDWR | O_NONBLOCK | O_CLOEXEC);
    if (fd_ < 0)
        throw std::runtime_error("cannot open " + path + ": " + std::strerror(errno));
}

TLightDevice::~TLightDevice() {
    if (fd_ >= 0) ::close(fd_);
}

bool TLightDevice::ioc(unsigned long request, void* arg) {
    return ::ioctl(fd_, request, arg) == 0;
}

int TLightDevice::readEvents(std::vector<tl_event>& out, int timeout_ms) {
    pollfd p{fd_, POLLIN, 0};
    const int r = ::poll(&p, 1, timeout_ms);
    if (r == 0) return 0;
    if (r < 0) return errno == EINTR ? 0 : -1;

    tl_event buf[16];
    const ssize_t n = ::read(fd_, buf, sizeof(buf));
    if (n < 0) return (errno == EAGAIN || errno == EINTR) ? 0 : -1;
    const int count = static_cast<int>(n / sizeof(tl_event));
    out.insert(out.end(), buf, buf + count);
    return count;
}

bool TLightDevice::getStatus(tl_status& st) { return ioc(TL_IOC_GET_STATUS, &st); }

bool TLightDevice::setState(uint32_t state) { return ioc(TL_IOC_SET_STATE, &state); }

bool TLightDevice::setAuto(bool on) {
    uint32_t v = on ? 1 : 0;
    return ioc(TL_IOC_SET_AUTO, &v);
}

bool TLightDevice::setConfig(const tl_config& cfg) {
    tl_config c = cfg;
    return ioc(TL_IOC_SET_CONFIG, &c);
}

bool TLightDevice::simulateVehicle() { return ioc(TL_IOC_SIM_VEHICLE, nullptr); }

bool TLightDevice::resetStats() { return ioc(TL_IOC_RESET_STATS, nullptr); }

}  // namespace tls
