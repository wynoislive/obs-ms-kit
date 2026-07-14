#pragma once
#include "MockOutputSession.hpp"
#include "../engine/analytics/PerformanceRuleEngine.hpp"
#include <memory>
#include <vector>
#include <string>

namespace mskit::test {

class PerformanceRuleTestHarness {
private:
    std::shared_ptr<mskit::engine::OutputController> controller;
    std::shared_ptr<mskit::engine::PerformanceRuleEngine> rule_engine;
    std::shared_ptr<MockOutputSession> mock_session;

    // Logs the results of each simulation step
    struct LogEntry {
        uint32_t tick;
        std::string health_state;
        uint32_t current_bitrate;
        int64_t rtt;
        double congestion;
        uint32_t dropped_frames;
    };
    std::vector<LogEntry> simulation_log;

    void LogState(uint32_t tick);

public:
    PerformanceRuleTestHarness();
    ~PerformanceRuleTestHarness() = default;

    // Executes the multi-phase timeline scenario
    bool RunSimulationSuite();
};

} // namespace mskit::test
