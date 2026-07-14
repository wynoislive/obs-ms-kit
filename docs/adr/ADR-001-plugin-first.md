# ADR-001: Plugin-First Architecture

## Status
Accepted

## Context
Building a multi-destination streaming solution for OBS Studio could be achieved by either compiling a custom fork of OBS Studio with built-in multi-output support, or writing a modular plugin. 

Creating a fork of OBS Studio introduces substantial technical debt, requiring continuous merging of upstream commits, causing version divergence, and complicating installation for end-users.

## Decision
We will build `obs-ms-kit` strictly as a modular, out-of-tree plugin. We will utilize only the public OBS APIs (`obs-module.h` and `obs-frontend-api.h`) to intercept composition canvases and manage output streams.

## Consequences
- Bypasses upstream merge complexity.
- Retains 100% compatibility with official OBS Studio releases.
- Simplifies distribution (distributed as a single library or installer package).
