# Non-Functional Requirements (NFR) Specification

This document defines the strict quality constraints and regression targets for the `obs-ms-kit` codebase.

## 1. Performance Budgets

Every feature integration must justify its runtime overhead against the following thresholds:

| Metric | Target | Verification Method |
| --- | --- | --- |
| **Idle CPU Overhead** | <0.5% total CPU consumption | Profiler trace |
| **Idle Memory Footprint** | <40 MB total heap allocation | Heap allocation tracking |
| **GPU Texture Duplication** | Zero duplicate copies (render-once) | Graphics diagnostics tools |
| **Execution Paths** | Fully event-driven (no polling loops) | Code review checklist |

## 2. Reliability & Isolation
- **Node Failure Domain**: If a single `OutputNode` fails due to network disconnection or encoding errors, it must enter the `Error`/`Reconnecting` state without affecting parallel output destinations, recording tasks, or the OBS rendering pipeline.
- **Self-Healing**: Sockets must tear down cleanly and attempt reconnection under exponential backoff timers without UI lockups.

## 3. Security Requirements
- **Secrets Management**: Stream keys and credentials must never be written to application logs. They must be masked as `[REDACTED]` in telemetry payloads.
- **Storage**: Stream keys should be stored and queried through OBS's native secure password storage where supported.
- **Telemetry**: Zero remote tracking by default. All monitoring and metric evaluations must happen locally.
