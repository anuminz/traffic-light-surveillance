#pragma once
// RAII wrapper around the /dev/tlight file descriptor (system programming:
// open/read/poll/ioctl/close).
#include <string>
#include "IDevice.hpp"

namespace tls {

class TLightDevice : public IDevice {
public:
    explicit TLightDevice(const std::string& path);   // throws std::runtime_error
    ~TLightDevice() override;
    TLightDevice(const TLightDevice&) = delete;
    TLightDevice& operator=(const TLightDevice&) = delete;

    int  readEvents(std::vector<tl_event>& out, int timeout_ms) override;
    bool getStatus(tl_status& st) override;
    bool setState(uint32_t state) override;
    bool setAuto(bool on) override;
    bool setConfig(const tl_config& cfg) override;
    bool simulateVehicle() override;
    bool resetStats() override;

private:
    bool ioc(unsigned long request, void* arg);
    int fd_ = -1;
};

}  // namespace tls
