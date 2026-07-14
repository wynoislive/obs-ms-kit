#pragma once
#include "IService.hpp"
#include <string>

namespace mskit::kernel {

enum class LogCategory : uint8_t {
    CORE,
    OUTPUT,
    NETWORK,
    PROTOCOL,
    ENCODER,
    RULES,
    UI,
    CONFIG,
    OBS,
    PERFORMANCE
};

class ILogger : public IService {
public:
    virtual ~ILogger() = default;
    virtual void LogTrace(LogCategory category, const std::string& message) = 0;
    virtual void LogWarning(LogCategory category, const std::string& message) = 0;
    virtual void LogError(LogCategory category, const std::string& message) = 0;
};

} // namespace mskit::kernel
