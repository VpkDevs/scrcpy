# Connectivity troubleshooting (USB and TCP/IP)

This page complements [connection.md](connection.md) with common failure modes
when the device does not appear in `adb devices` or scrcpy exits early.

## USB

- **Cable**: use a data-capable USB cable (some cables are charge-only).
- **Port**: try another USB port (prefer USB 2.0/3.0 on the motherboard rear
  panel on desktops).
- **Windows drivers**: if the phone only charges, install the OEM USB driver
  or the [Google USB Driver](https://developer.android.com/studio/run/oem-usb)
  for your device family.
- **Authorization**: accept the RSA fingerprint dialog on the device; if stuck
  in `unauthorized`, revoke USB debugging authorizations (Developer options) and
  reconnect.
- **Offline state**: run `adb kill-server`, unplug/replug, or toggle USB
  debugging on the device.

## Wireless (TCP/IP)

- **Same network**: PC and phone must reach each other (same Wi‑Fi or routed
  LAN). Guest Wi‑Fi often blocks client-to-client traffic.
- **Firewall**: allow inbound TCP to the port used by `adb connect` (default
  5555 on the device when enabled with `tcpip`). On Windows, add a rule for
  `adb.exe` or the ephemeral port scrcpy uses for reverse tunnels if you block
  local listeners.
- **IP changes**: DHCP may assign a new address; re-run `adb connect` with the
  current IP.
- **Pairing (Android 11+)**: wireless debugging may require pairing with
  `adb pair` before `adb connect`.

## Verify ADB

```bash
adb devices -l
```

You should see `device` (not `offline` or `unauthorized`) for the target serial.

## Duplex (PC → phone) development builds

When using the experimental `sc-reverse-host` tool, forward the listener port to
the device so the Android viewer can connect:

```bash
adb reverse tcp:27183 tcp:27183
```

(Replace `27183` with the port printed by `sc-reverse-host --help`.)
