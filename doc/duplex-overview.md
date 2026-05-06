# Duplex mirroring (experimental)

This fork extends classic scrcpy (Android → PC mirror and control) with a
**second channel**: PC screen → Android viewer and touch/keyboard from the
phone back to the PC.

## Documentation map

| Document | Purpose |
|----------|---------|
| [duplex-protocol-v1.md](duplex-protocol-v1.md) | Binary wire format (stream + control) |
| [duplex-polish.md](duplex-polish.md) | Wi‑Fi, security, codecs, multi-monitor checklist |
| [FEATURES_BACKLOG.md](FEATURES_BACKLOG.md) | Prioritized optional modules |
| [../packaging/README.md](../packaging/README.md) | Windows build and release notes |

## Components

1. **`scrcpy`** (existing): device → host video/audio/control.
2. **`sc-reverse-host`** (new, Windows-first): host capture + encode (WIP) +
   listens for reverse connections; applies control events from the device.
3. **`duplex/reverse-android-view/`** (Android Studio): viewer + uplink (WIP).

## Quick start (development)

1. Build the host tool (see [build.md](build.md) and Meson option
   `compile_duplex_host`). On Windows, link **FFmpeg** (`libavcodec`, `libavutil`,
   `libswscale`) so DXGI capture + H.264 encode are enabled.
2. Install the Android viewer APK (build from `duplex/reverse-android-view/`).
3. `adb reverse` the TCP port, start `sc-reverse-host`, then launch the viewer
   on the device pointing at `127.0.0.1` on that port.

The viewer decodes **AVC** into a `SurfaceView` when the host sets the video
flag in the duplex header (see [duplex-protocol-v1.md](duplex-protocol-v1.md)).

Details evolve with each release; see git tags matching `duplex-baseline-*`.
