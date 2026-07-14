# ADR-002: OutputNode Architecture

## Status
Accepted

## Context
Traditional multistreaming plugins spin up a permanent OS worker thread for each stream destination. As users scale up to 6–10 outputs, this approach causes high context-switching overhead, complex synchronization issues, and increased likelihood of global crashes when one connection stalls.

## Decision
Each stream destination is managed as an independent `OutputNode` state machine. An `OutputNode` does not own a dedicated permanent thread; instead, its network, scaling, and encoding states are executed as tasks scheduled via the central Kernel scheduler.

## Consequences
- Eliminates thread bloat and reduces CPU overhead.
- State transitions (e.g., from Streaming to Reconnecting) are explicit and testable.
- A failure in one `OutputNode` cannot affect the thread of another.
