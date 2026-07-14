# ADR-004: Network Pipeline Buffer

## Status
Accepted

## Context
Various destinations require different queueing strategies for manual stream delays, adaptive congestion control, or reconnection buffering. Designing separate buffers for each feature or protocol leads to code duplication and hard-to-maintain network pipelines.

## Decision
We implement a generalized `NetworkBuffer` component. Rather than acting purely as a "stream delay" buffer, it is a protocol-agnostic pipeline buffer. It manages packet queues and supports multiple configurations (e.g., static delay, dynamic smoothing, or socket retry caching).

## Consequences
- Single, unified data structure for delay and congestion handling.
- Decouples transport clients (RTMP, SRT) from the queueing logic.
