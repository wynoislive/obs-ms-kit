# ADR-009: Configuration Management and Swapping

## Status
Accepted

## Context
Mutating configuration fields (like resolution or bitrate) on live running outputs leads to race conditions, partial stream tears, or crashes.

## Decision
We separate configuration from runtime state:
- `OutputNodeConfig` is completely immutable during streaming.
- To change settings on an active node, the control plane clones the configuration, updates the settings, and triggers a safe swap transition in the `OutputNodeRuntime` during an inter-frame window.

## Consequences
- Guarantees thread-safe configuration updates.
- Simplifies configuration versioning and profiles serialization.
