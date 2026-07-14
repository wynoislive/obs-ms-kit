# obs-ms-kit
### Developed by Wyno

A modern, high-performance, telemetry-driven multi-streaming engine for OBS Studio.

## Features
* **Multi-Platform Support**: Build natively for Windows, macOS (VideoToolbox), and Linux (VAAPI).
* **Closed-Loop Telemetry**: Real-time bitrate adaptation.
* **Low Latency**: Native support for SRT and WHIP/WebRTC protocols.

## Installation
1. Download the latest release from the [Releases Page](https://github.com/wynoislive/obs-ms-kit/releases).
2. Follow the platform-specific instructions below:
    - **Windows**: Copy files to `obs-plugins/64bit/` in your OBS installation.
    - **macOS**: Install the `.dmg` package.
    - **Linux**: Install the `.deb` package via `sudo apt install ./obs-ms-kit.deb`.

## Support
Built with modularity in mind. See `docs/architecture/` for ADRs.
