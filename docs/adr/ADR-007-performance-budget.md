# ADR-007: Performance Budget Enforcement

## Status
Accepted

## Context
Creator systems are highly sensitive to CPU, GPU, and VRAM utilization. Overhead from additional stream outputs must not impact the host game or the OBS render engine.

## Decision
We enforce rigid performance budgets as regression targets:
- **Idle CPU**: <0.5% total CPU resource usage when outputs are inactive.
- **Idle Memory**: <40 MB total memory footprint.
- **GPU Texture Copies**: Zero memory copies on the render path; only lock-free pointer distribution is allowed.
- **Polling**: Prohibited. Telemetry gathering must be event-driven or run on coarse, scheduled intervals.

## Consequences
- Protects high-framerate gameplay on gaming rigs.
- Forces all future features to justify their resource cost before merging.
