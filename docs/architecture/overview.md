# System Architecture Overview: obs-ms-kit

This document outlines the architectural layers and service flows of the `obs-ms-kit` streaming engine.

## 1. Modular Architectural Layers

To ensure strict separation of concerns, data and control flows are partitioned into five isolated execution layers:

```
+-------------------------------------------------+
|               Creator Hub UI                    |
+-----------------------+-------------------------+
                        | (User actions / state views)
                        v
+-------------------------------------------------+
|                Control Plane                    |
+-----------------------+-------------------------+
                        | (Config updates / Command dispatches)
                        v
+-------------------------------------------------+
|                MS-Kit Kernel                    |
+-----------------------+-------------------------+
                        | (Service Registry, Scheduler, Telemetry)
                        v
+-------------------------------------------------+
|              Streaming Engine                   |
+-----------------------+-------------------------+
                        | (FrameDistributor, OutputNodes, Scalers)
                        v
+-------------------------------------------------+
|               Protocol Layer                    |
+-----------------------+-------------------------+
                        | (RTMP, SRT, WHIP Transport Socket Clients)
                        v
+-------------------------------------------------+
|                 OBS Core                        |
+-------------------------------------------------+
```

## 2. The Kernel & Service Registry

The MS-Kit Kernel acts as the central orchestrator and service coordinator. Subsystems interact strictly through interface definitions resolved from a thread-safe registry:
- `IResourceManager`: Manages system limits (CPU, GPU, VRAM) and assigns resource budgets.
- `IScheduler`: Coordinates asynchronous task execution priorities.
- `ICapabilityManager`: Evaluates destination platform capabilities.
- `ITelemetryService`: Collects and aggregates real-time performance telemetry.
