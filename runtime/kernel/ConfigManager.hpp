#pragma once
#include "IConfigManager.hpp"
#include "../engine/output/OutputController.hpp"
#include <obs.h>
#include <mutex>
#include <memory>

namespace mskit::kernel {

class ConfigManager : public IConfigManager {
private:
    static constexpr uint32_t kCurrentSchemaVersion = 1;

    mutable std::mutex config_mutex;
    std::weak_ptr<mskit::engine::OutputController> controller_hook;

    // Internal structural schema sanitizers
    bool ValidateAndMigrate(obs_data_t* raw_data) const;

public:
    ConfigManager() = default;
    virtual ~ConfigManager() override = default;

    // Core Control Plane Linkage
    void Initialize(std::shared_ptr<mskit::engine::OutputController> controller);

    // IService Overrides
    virtual const char* GetServiceName() const override { return "ConfigManager"; }

    // IConfigManager Overrides
    virtual bool ImportProfile(const std::string& file_path) override;
    virtual bool ExportProfile(const std::string& file_path) override;
    virtual uint32_t GetProfileSchemaVersion() const override { return kCurrentSchemaVersion; }
};

} // namespace mskit::kernel
