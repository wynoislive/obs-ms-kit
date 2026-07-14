# ADR-006: OutputNode State Machine

## Status
Accepted

## Context
Ad-hoc state flags (e.g., `is_active`, `reconnecting`) lead to race conditions, partial memory leaks during crashes, and state drift.

## Decision
We enforce a rigid state machine for each `OutputNode`. The node must transition only along defined paths:
`Stopped` -> `Initializing` -> `Ready` -> `Encoding` -> `Streaming` -> `Congested` -> `Reconnecting` -> `Recovering` -> `Stopped` (or `Error`).
All state transitions are logged and handled via explicit, synchronous control events.

## Consequences
- Predictable lifecycle behavior.
- Clean state reporting for UI dashboard integrations.
