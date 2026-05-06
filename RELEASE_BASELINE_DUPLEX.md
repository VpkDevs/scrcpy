# Duplex baseline release (fork maintainers)

This file marks the **foundation** milestone for the duplex roadmap: improved ADB
diagnostics, connectivity documentation, protocol specification, and
experimental host/Android stubs.

## Tagging a baseline

After merging the duplex foundation commits:

```bash
git tag -a duplex-baseline-1 -m "Duplex roadmap: docs, protocol v1, reverse host stub, Android viewer skeleton"
git push fork duplex-baseline-1
```

Compare the client behavior to upstream [Genymobile/scrcpy](https://github.com/Genymobile/scrcpy)
at the same upstream revision; forward-path features remain compatible until
explicitly superseded by duplex-specific options.

## Contents of baseline-1

- `doc/connectivity-troubleshooting.md`
- `doc/duplex-overview.md`, `doc/duplex-protocol-v1.md`, `doc/duplex-polish.md`
- `doc/FEATURES_BACKLOG.md`
- `duplex/reverse-host/` — `sc-reverse-host` (Windows control path + listen)
- `duplex/reverse-android-view/` — Android Studio project skeleton
- `packaging/README.md`
