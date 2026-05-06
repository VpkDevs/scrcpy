# sc-reverse-host

Listens on `127.0.0.1:27183` (configurable `--port`), sends the duplex v1
24-byte header, then processes framed **control** messages from the Android
viewer (`Duplex Reverse View`).

On **Windows**, pointer messages move the cursor and synthesize mouse buttons
via `SendInput`. **Video capture** (DXGI + hardware encoder) is stubbed in
`src/win/capture_dxgi_stub.c` until wired to emit `VIDEO_NAL` frames per
[doc/duplex-protocol-v1.md](../../doc/duplex-protocol-v1.md).

Build with Meson (`-Dcompile_duplex_host=true`); see [packaging/README.md](../../packaging/README.md).
