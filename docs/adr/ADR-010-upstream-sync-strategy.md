# ADR-010: Upstream Synchronization Strategy

## Status
Accepted

## Context
Upstream Wyno will receive bug fixes, platform updates, and feature changes. Manually copying files isolates our repository from upstream development, making maintenance difficult.

## Decision
We establish Wyno as a Git remote named `upstream`. All external baseline files are integrated directly into our repository history. Upstream updates will be fetched and merged into our `develop` branch, maintaining a clear audit trail.

## Consequences
- Allows clean integration of upstream patches and bug fixes.
- Preserves local code rebrandings and customized structural layers.
