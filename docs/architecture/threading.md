# Threading Model Specification

To prevent UI stuttering and rendering frame drops, `obs-ms-kit` enforces strict thread boundaries and lock-free execution paths.

## 1. Thread Domains

The engine runs across three primary thread domains:

### A. The OBS Render Thread
- **Role**: Renders the canvas composition.
- **Rules**: Must never execute blocking operations (disk I/O, network socket writes, or complex allocations).
- **Integration**: The `FrameDistributor` runs inside this thread. It performs lock-free fan-out of texture references using shared GPU handles.

### B. The Network Task Pool
- **Role**: Services active network sockets (RTMP/SRT/WHIP) and drains the `NetworkBuffer`.
- **Rules**: Thread priorities are managed dynamically by the `IScheduler`. Stalled sockets or slow TCP windows must block these background tasks only, leaving the OBS render thread untouched.

### C. The UI / Control Thread
- **Role**: Handles layout repaints, user configuration inputs, and status updates.
- **Rules**: Dispatches configuration changes as cloned immutable struct swaps (`OutputNodeConfig`) rather than modifying active runtime properties directly.
