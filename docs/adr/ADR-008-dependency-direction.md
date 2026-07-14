# ADR-008: Dependency Direction

## Status
Accepted

## Context
Circular dependencies between UI layout widgets, network handlers, and rendering controllers lead to rigid codebases, memory leak cycles, and make unit testing impossible.

## Decision
We enforce a strict unidirectional dependency rule:
`UI` -> `Control Plane` -> `MS-Kit Kernel` -> `Streaming Engine` -> `Protocol Layer` -> `OBS Core`
Lower layers must never have build or include dependencies on higher layers. Subsystems interact only via stable, abstract interfaces resolved through the Kernel's Service Registry.

## Consequences
- Clean compile boundaries.
- Subsystems can be mocked or unit-tested in complete isolation.
