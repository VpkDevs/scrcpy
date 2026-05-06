# Duplex protocol v1 (PC ↔ Android)

All multi-byte integers are **little-endian** unless noted. The first message
after the TCP connection is always `stream_caps` from the **host** (PC).

## Magic and versioning

The connection begins with a fixed **file header** (24 bytes):

| Offset | Size | Field |
|--------|------|--------|
| 0 | 8 | Magic ASCII `DUPLEX10` (no NUL) |
| 8 | 2 | `version` = `1` |
| 10 | 2 | `flags` (bit0 = host_has_video) |
| 12 | 4 | `host_width` (pixels; 0 if unknown yet) |
| 16 | 4 | `host_height` |
| 20 | 4 | `codec_fourcc` (`h264` as 0x34363268 LE) |

The Android viewer **must** reject unknown `version` values.

## Framed messages (after header)

Each message:

| Field | Size |
|-------|------|
| `msg_type` | `uint32` |
| `payload_len` | `uint32` |
| `payload` | `payload_len` bytes |

`msg_type` values (host → device):

| Value | Name | Payload |
|-------|------|---------|
| `0x100` | `VIDEO_CODEC_CONFIG` | Decoder-specific config (e.g. AVCDecoderConfigurationRecord) |
| `0x101` | `VIDEO_NAL` | One or more length-prefixed NAL units (host-defined; v1 may use Annex-B) |
| `0x102` | `HOST_CLIPBOARD` | UTF-8 text (optional) |

`msg_type` values (device → host):

| Value | Name | Payload |
|-------|------|---------|
| `0x200` | `CTRL_POINTER` | See below |
| `0x201` | `CTRL_KEY` | `uint16` Android keycode + `uint8` action (down/up) — mapped on host |
| `0x202` | `CTRL_WHEEL` | `int32` scroll delta (Windows `mouseData` / `WHEEL_DELTA` units; extend later) |
| `0x203` | `DEVICE_CLIPBOARD` | UTF-8 text |

### `CTRL_POINTER` payload (16 bytes)

| Offset | Type | Field |
|--------|------|--------|
| 0 | `uint32` | `pointer_id` (0 = primary) |
| 4 | `float` | `x_norm` 0..1 relative to **host** desktop width |
| 8 | `float` | `y_norm` 0..1 relative to host desktop height |
| 12 | `uint32` | `buttons` bit0=left, bit1=right, bit2=middle |

Multi-touch: multiple `pointer_id` values; host synthesizes contacts when OS
allows.

### `CTRL_KEY` payload (4 bytes)

| Offset | Type | Field |
|--------|------|--------|
| 0 | `uint16` | `sym` (host maps from Android keycodes in v1) |
| 2 | `uint8` | `action` 1=down 0=up |
| 3 | `uint8` | reserved |

## Security (v1 minimal)

- Bind to `127.0.0.1` only; require `adb reverse` so the stream is not exposed
  on LAN.
- v2+: session token in `flags` / first authenticated frame (see
  [duplex-polish.md](duplex-polish.md)).

## Clipboard hooks

Clipboard is **optional** and **one-shot per message**: host may push
`HOST_CLIPBOARD` when the desktop clipboard changes; device may push
`DEVICE_CLIPBOARD` when the Android clipboard changes. Deduplication is
implementation-defined.
