# Release Notes: obs-ms-kit v1.0.0 (Production General Availability)

We are thrilled to announce the official v1.0.0 release of **obs-ms-kit**. This release represents a complete, ground-up re-architecture of the legacy multi-streaming engine. It establishes a thread-safe, high-performance, and telemetry-driven multi-destination routing platform for OBS Studio 30+.

## Key Architectural Advancements

### 1. Hardened Threading & Pipeline Isolation (ADR-002, ADR-003, ADR-005)
*   **The Problem:** Legacy plugins often perform render-blocking network handshakes, leading to severe frame drops and UI freezes.
*   **Our Solution:** Complete decoupling of the Graphics, Control, and Data planes. High-frequency render ticks write to isolated GPU staging slots. Task payloads are dispatched asynchronously to a priority-bucketed `Scheduler` worker pool running under `TaskPriority::Critical` entirely off the main OBS rendering and GUI threads.

### 2. Autonomous Closed-Loop Telemetry (ADR-007)
*   **Stateful Performance Rule Engine:** Features an active 1000ms evaluation loop that monitors dynamic line health.
*   **Anti-Thrashing Cooldown:** Implements defensive down-stepping (instant 20% bitrate reduction under congestion) paired with a stateful 5-second stabilization window before attempting controlled $+300\text{ kbps}$ up-steps, preventing destructive keyframe-regeneration loops.

### 3. Unified Transport Plane (ADR-004)
*   **RTMP (Legacy):** Fully isolated, thread-safe `obs_service_t` wrappers.
*   **SRT (Low-Latency):** Direct pipeline integration utilizing the native `ffmpeg_mpegts` output module, configured with microsecond-precision buffering, Too-Late Packet Drop (TLPKTDROP), and ARQ bandwidth headroom tuning.
*   **WebRTC / WHIP (Modern Ingestion):** Out-of-band HTTP POST SDP handshakes and ICE negotiations routed natively via `whip_output` and authorized with custom bearer tokens.

### 4. Interactive Configuration & UI Dock (ADR-003)
*   **Creator Hub Dashboard:** A high-performance, asynchronous Qt6 widget registered directly into OBS Studio's dock layouts via `obs_frontend_add_dock_by_id`.
*   **Add Destination Dialog:** A modern, context-aware modal configurator supporting RTMP stream keys, SRT passphrases, and WHIP authentication.
*   **ConfigManager:** A robust serialization engine featuring automatic JSON schema validation and safe profile hot-swapping.
