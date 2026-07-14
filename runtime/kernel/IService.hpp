#pragma once

namespace mskit::kernel {

class IService {
public:
    virtual ~IService() = default;
    virtual const char* GetServiceName() const = 0;
};

} // namespace mskit::kernel
