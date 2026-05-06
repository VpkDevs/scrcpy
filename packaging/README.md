# Packaging and distribution (duplex fork)

## Windows

### Prerequisites

- MSYS2 / MinGW-w64 **or** Visual Studio Build Tools (match upstream
  [build.md](../doc/build.md)).
- Android SDK / NDK if you rebuild `scrcpy-server` and the duplex viewer APK.

### Build scrcpy + duplex host

```bash
meson setup build --buildtype=release -Dcompile_duplex_host=true
ninja -C build
```

Artifacts:

- `build/app/scrcpy.exe` (or `scrcpy` on Unix)
- `build/duplex/reverse-host/sc-reverse-host.exe` (Windows; console subsystem)

### adb reverse for the viewer

Default duplex TCP port is **27183**. After starting `sc-reverse-host`:

```bat
adb reverse tcp:27183 tcp:27183
```

### Signing and releases

1. Build release binaries with consistent `meson` options.
2. Compute SHA-256 sums for each zip artifact.
3. Sign binaries with your Authenticode certificate (Windows) before wide
   distribution.
4. Publish `THIRD_PARTY_NOTICES.txt` alongside the zip (see template below).

### THIRD_PARTY_NOTICES template

Include at minimum:

- scrcpy / Genymobile and Romain Vimont — Apache-2.0
- SDL2 / ffmpeg / libusb — per their licenses when linked
- Duplex components you add — your SPDX identifiers

## macOS / Linux

`sc-reverse-host` currently implements **full pointer injection on Windows**
only. Other hosts can still run the listener for protocol testing; implement
Quartz / PipeWire capture in a follow-up milestone.

## Legal

- Keep [LICENSE](../LICENSE) and copyright notices in source distributions.
- Add a `PRIVACY.md` if you ship telemetry or notification mirroring.
