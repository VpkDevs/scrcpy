# Duplex Reverse View (Android)

Experimental viewer for the **PC → phone** leg of duplex mirroring.

## Prerequisite on the PC

1. Build `sc-reverse-host` (see [packaging/README.md](../../packaging/README.md)).
2. Start it, then run `adb reverse tcp:27183 tcp:27183` (adjust port if needed).

## Build

Open this directory in **Android Studio** (Hedgehog or newer). Let Gradle sync
and generate the wrapper if prompted, then **Run** on a USB-connected device.

The app connects to `127.0.0.1:27183` on the device (forwarded to the host),
reads the duplex v1 header, then shows status text. **MediaCodec** decode and
fullscreen touch are the next implementation steps (see
[doc/duplex-overview.md](../../doc/duplex-overview.md)).

## Exit safety

Use the system **Back** gesture or the on-screen **Disconnect** button to close
the socket and finish the activity.
