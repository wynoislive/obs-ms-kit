#include "PerformanceRuleTestHarness.hpp"
#include <util/platform.h>
#include <iostream>
#include <iomanip>

namespace mskit::test {

PerformanceRuleTestHarness::PerformanceRuleTestHarness() {
    controller = std::make_shared<mskit::engine::OutputController>();

    // Set up our target baseline: 1080p60 at 5000 kbps
    mskit::OutputProfile base_profile;
    base_profile.video_bitrate = 5000;
    base_profile.target_width = 1920;
    base_profile.target_height = 1080;
    base_profile.encoder_preset = "NVENC";

    mock_session = std::make_shared<MockOutputSession>("test_node", base_profile);
    mock_session->Open();

    // Register our mock session directly into the engine's controller
    controller->RegisterSession("test_node", mock_session);
}

void PerformanceRuleTestHarness::LogState(uint32_t tick) {
    uint32_t bitrate = 0, dropped = 0;
    double fps = 0.0;
    mock_session->GetLiveTelemetry(bitrate, fps, dropped);

    int64_t rtt = 0;
    double congestion = 0.0;
    mock_session->GetNetworkTelemetry(rtt, congestion);

    std::string health_label = "Healthy";
    auto monitor = mock_session->GetHealthMonitor();
    if (monitor) {
        auto state = monitor->GetHealthState();
        if (state == mskit::engine::OutputHealth::Congested) health_label = "Congested";
        else if (state == mskit::engine::OutputHealth::Warning) health_label = "Warning";
    }

    simulation_log.push_back({tick, health_label, bitrate, rtt, congestion, dropped});
}

bool PerformanceRuleTestHarness::RunSimulationSuite() {
    blog(LOG_INFO, "[MSK-TEST] Starting automated PerformanceRuleEngine validation suite...");

    rule_engine = std::make_shared<mskit::engine::PerformanceRuleEngine>();
    rule_engine->Initialize(controller);

    // --- STEP-BY-STEP SCENARIO TIMELINE ---

    // T = 0 to 2s: Pristine Network Conditions (Stable Baseline)
    mock_session->InjectNetworkTelemetry(15, 0.02); // RTT = 15ms, Congestion = 2%
    mock_session->InjectHealthState(mskit::engine::OutputHealth::Healthy);
    for (uint32_t t = 0; t <= 2; ++t) {
        std::shared_ptr<mskit::IOutputSession> cast_session = mock_session;
        rule_engine->EvaluateSessionHealth(cast_session);
        LogState(t);
    }

    // T = 3s: Sudden Squeeze (Bandwidth Constrained, Congested state injected)
    mock_session->InjectNetworkTelemetry(280, 0.85); // RTT = 280ms, Congestion = 85%
    mock_session->InjectHealthState(mskit::engine::OutputHealth::Congested);
    {
        std::shared_ptr<mskit::IOutputSession> cast_session = mock_session;
        rule_engine->EvaluateSessionHealth(cast_session);
        LogState(3);
    }

    // T = 4s: Sustained Congestion (Accumulating dropped frames)
    mock_session->InjectLiveTelemetry(4000, 60.0, 15); // Drop 15 frames
    {
        std::shared_ptr<mskit::IOutputSession> cast_session = mock_session;
        rule_engine->EvaluateSessionHealth(cast_session);
        LogState(4);
    }

    // T = 5s: Network Clears (Pristine Health restored)
    mock_session->InjectNetworkTelemetry(12, 0.01); // RTT = 12ms, Congestion = 1%
    mock_session->InjectHealthState(mskit::engine::OutputHealth::Healthy);
    {
        std::shared_ptr<mskit::IOutputSession> cast_session = mock_session;
        rule_engine->EvaluateSessionHealth(cast_session);
        LogState(5);
    }

    // T = 6 to 9s: Cooldown Phase (Sustained clean line, but still waiting)
    for (uint32_t t = 6; t <= 9; ++t) {
        std::shared_ptr<mskit::IOutputSession> cast_session = mock_session;
        rule_engine->EvaluateSessionHealth(cast_session);
        LogState(t);
    }

    // T = 10s: Cooldown Expires (5th consecutive clean tick - Recovery kicks in!)
    {
        std::shared_ptr<mskit::IOutputSession> cast_session = mock_session;
        rule_engine->EvaluateSessionHealth(cast_session);
        LogState(10);
    }

    // T = 11s: Continued Recovery
    {
        std::shared_ptr<mskit::IOutputSession> cast_session = mock_session;
        rule_engine->EvaluateSessionHealth(cast_session);
        LogState(11);
    }

    // --- RENDER RESULTS TABLE ---
    std::cout << "\n========================================================================\n";
    std::cout << "               MS-KIT CONTROL LOOP SIMULATION SUMMARY                   \n";
    std::cout << "========================================================================\n";
    std::cout << " Tick |   State   | Bitrate (kbps) | RTT (ms) | Congestion | Frame Drops\n";
    std::cout << "------|-----------|----------------|----------|------------|------------\n";

    for (const auto& log : simulation_log) {
        std::cout << "  " << std::setw(3) << log.tick << " | "
                  << std::setw(9) << log.health_state << " | "
                  << std::setw(14) << log.current_bitrate << " | "
                  << std::setw(8) << log.rtt << " | "
                  << std::setw(10) << std::fixed << std::setprecision(2) << log.congestion << " | "
                  << std::setw(11) << log.dropped_frames << "\n";
    }
    std::cout << "========================================================================\n\n";

    // --- EVALUATE ASSERTIONS FOR TEST PASS/FAIL ---
    bool pass = true;

    // Assertion 1: Bitrate must start at baseline (5000)
    if (simulation_log[0].current_bitrate != 5000) pass = false;

    // Assertion 2: Direct 20% cut must occur on T=3 congestion (5000 -> 4000)
    if (simulation_log[3].current_bitrate != 4000) pass = false;

    // Assertion 3: Bitrate must remain flat at T=5 to T=9 during the cooldown (3200 kbps, due to another 20% cut at T=4 congestion because drop delta was > 5)
    for (size_t i = 5; i <= 9; ++i) {
        if (simulation_log[i].current_bitrate != 3200) {
            pass = false;
        }
    }

    // Assertion 4: Bitrate must step up by 300 kbps at T=10 (3200 -> 3500)
    if (simulation_log[10].current_bitrate != 3500) pass = false;

    if (pass) {
        blog(LOG_INFO, "[MSK-TEST] PerformanceRuleEngine control loops verified successfully! All assertions passed.");
    } else {
        blog(LOG_ERROR, "[MSK-TEST] PerformanceRuleEngine control loops failed verification.");
    }

    return pass;
}

} // namespace mskit::test
