#pragma once
#include "IService.hpp"
#include <string>
#include <cstdint>


namespace mskit::kernel {

class IConfigManager : public IService {
public:
    virtual ~IConfigManager() = default;
    virtual bool ImportProfile(const std::string& file_path) = 0;
    virtual bool ExportProfile(const std::string& file_path) = 0;
    virtual uint32_t GetProfileSchemaVersion() const = 0;
};

} // namespace mskit::kernel
