# Feature backlog (prioritized, optional modules)

Reimplement **ideas** from the market; do not copy proprietary code or assets.
Each item should ship behind explicit user consent where privacy applies.

## P0 — Core duplex (blocking “revolution”)

1. PC DXGI (or equivalent) capture + hardware encode → `VIDEO_NAL`.
2. Android MediaCodec decode + fullscreen Surface.
3. Control uplink: pointer + keyboard MVP on Windows (`SendInput`).
4. Session lifecycle: connect, disconnect, error surfaces.

## P1 — Productivity

5. Drag-and-drop file transfer (chunked over ADB or duplex side channel).
6. Symmetric clipboard (wire format already in [duplex-protocol-v1.md](duplex-protocol-v1.md)).
7. Saved device list + last-used codec settings.

## P2 — OS integration

8. Windows system tray icon + quick mute / disconnect.
9. Android Quick Settings tile to launch viewer (requires APK install).
10. Optional notification listener (Android permission + privacy policy).

## P3 — Power user

11. Multi-device dashboard (several Android tiles on one desktop).
12. Recording duplex sessions (mux to Matroska).
13. HDR desktop capture feasibility.

## Deferred / high policy risk

- SMS / call integration (carrier policies, consent UX).
- Remote access over internet without VPN (security review mandatory).
