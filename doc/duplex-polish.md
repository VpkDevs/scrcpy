# Duplex polish checklist (post-MVP)

Use this list after the first end-to-end PC → Android video path works.

## Connectivity

- [ ] Mirror `adb reverse` / `adb forward` ergonomics used by classic scrcpy
  tunnels ([tunnels.md](tunnels.md)).
- [ ] Wi‑Fi: same pairing flows as USB; document firewall ports per OS.
- [ ] Reconnect backoff when the viewer drops.

## Security

- [ ] Replace bind-127.0.0.1-only with optional **pinned token** in the first
  frame after `stream_caps`.
- [ ] Mutual TLS or Noise protocol for LAN mode (if ever enabled).
- [ ] Rate-limit control messages to mitigate injection abuse.

## Codecs and quality

- [ ] Negotiate H.264 vs H.265 vs AV1 in an extended `stream_caps` v2.
- [ ] Adaptive bitrate from RTT / dequeue times on the viewer.
- [ ] Optional desktop audio (Opus) alongside video.

## Display topology

- [ ] Multi-monitor: enumerate outputs; send `MONITOR_CHANGED` message.
- [ ] DPI-aware normalization for `CTRL_POINTER` (per-monitor scaling).
- [ ] Handle host resolution changes mid-session.

## Input

- [ ] Full keyboard mapping table (Android → Windows virtual keys).
- [ ] Gamepad passthrough where host OS exposes XInput / Raw Input.

## Android UX

- [ ] Immersive fullscreen, safe-area insets, pinch-to-zoom → scroll wheel.
- [ ] Foreground service + persistent notification while connected.
