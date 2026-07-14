# ADR-005: Render-Once Canvas Distribution

## Status
Accepted

## Context
Re-rendering or duplicatively copying the OBS canvas for every target stream scales poorly, saturating GPU VRAM and composition cycles.

## Decision
We enforce a **Render Once** design. The core OBS composition pipeline renders the frame exactly once. The `FrameDistributor` intercepts the output texture pointer and broadcasts it to each active `OutputNode` using lock-free reference sharing. Scaling is done per-output using GPU shaders inside isolated execution windows.

## Consequences
- Canvas composition overhead does not multiply with the number of outputs.
- Keeps VRAM copies to a minimum.
