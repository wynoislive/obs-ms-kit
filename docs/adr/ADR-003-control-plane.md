# ADR-003: Data Plane vs Control Plane Separation

## Status
Accepted

## Context
High-throughput video encoding and streaming tasks can be interrupted if they share threads or synchronization locks with control operations (like reading configuration files, performing UI repaints, or handling network settings changes). This leads to frame drops and UI micro-stutters.

## Decision
We enforce a strict separation of execution planes:
- **Data Plane**: Handles frame distribution, GPU scaling, video/audio encoding, and network socket operations. It operates in lock-free render paths and worker pools.
- **Control Plane**: Handles user commands, UI state changes, configuration persistence, and telemetry evaluation. It interacts with the Data Plane exclusively through safe, atomic handshakes (e.g., config swapping).

## Consequences
- Protects the rendering thread from disk, UI, and network I/O blockages.
- Eliminates common race conditions when changing settings while streaming.
