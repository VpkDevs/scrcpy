/*
 * sc-reverse-host — PC side of duplex mirroring (experimental).
 * Listens on loopback, sends stream_caps, streams H.264 (optional), control recv.
 */

#include "sc_duplex_proto.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
# include <winsock2.h>
# include <ws2tcpip.h>
typedef SOCKET sc_sock_t;
# define SC_SOCK_INVALID INVALID_SOCKET
# define sc_sock_close closesocket
# define sc_sock_errno WSAGetLastError()
# ifdef HAVE_DUPLEX_VIDEO
#  include <windows.h>
#  include "duplex_capture.h"
#  include "duplex_encode.h"
# endif
#else
# include <arpa/inet.h>
# include <netinet/in.h>
# include <netinet/tcp.h>
# include <sys/socket.h>
# include <unistd.h>
typedef int sc_sock_t;
# define SC_SOCK_INVALID (-1)
# define sc_sock_close close
# define sc_sock_errno errno
#endif

#include "duplex_input.h"

static int
read_full(sc_sock_t s, void *buf, size_t len) {
    uint8_t *p = buf;
    size_t off = 0;
    while (off < len) {
#ifdef _WIN32
        int r = recv(s, (char *) (p + off), (int) (len - off), 0);
#else
        ssize_t r = recv(s, p + off, len - off, 0);
#endif
        if (r <= 0) {
            return -1;
        }
        off += (size_t) r;
    }
    return 0;
}

static int
write_full(sc_sock_t s, const void *buf, size_t len) {
    const uint8_t *p = buf;
    size_t off = 0;
    while (off < len) {
#ifdef _WIN32
        int r = send(s, (const char *) (p + off), (int) (len - off), 0);
#else
        ssize_t r = send(s, p + off, len - off, 0);
#endif
        if (r <= 0) {
            return -1;
        }
        off += (size_t) r;
    }
    return 0;
}

static bool
send_framed(sc_sock_t s, uint32_t type, const void *payload, uint32_t plen) {
    uint8_t fh[8];
    sc_duplex_write_u32(fh, type);
    sc_duplex_write_u32(fh + 4, plen);
    if (write_full(s, fh, sizeof fh)) {
        return false;
    }
    if (plen && payload && write_full(s, payload, plen)) {
        return false;
    }
    return true;
}

static bool
send_header(sc_sock_t s, uint32_t width, uint32_t height, uint32_t flags) {
    uint8_t h[SC_DUPLEX_HEADER_LEN];
    memcpy(h, SC_DUPLEX_MAGIC, SC_DUPLEX_MAGIC_LEN);
    sc_duplex_write_u16(h + 8, SC_DUPLEX_VERSION);
    sc_duplex_write_u16(h + 10, (uint16_t) flags);
    sc_duplex_write_u32(h + 12, width);
    sc_duplex_write_u32(h + 16, height);
    sc_duplex_write_u32(h + 20, SC_DUPLEX_CODEC_H264);
    return write_full(s, h, sizeof h) == 0;
}

static bool
handle_control(sc_sock_t s) {
    uint8_t hdr[8];
    if (read_full(s, hdr, sizeof hdr)) {
        return false;
    }
    uint32_t type = sc_duplex_read_u32(hdr);
    uint32_t plen = sc_duplex_read_u32(hdr + 4);
    if (plen > 1 << 20) {
        fprintf(stderr, "sc-reverse-host: payload too large (%u)\n", plen);
        return false;
    }
    uint8_t *payload = malloc(plen ? plen : 1);
    if (!payload) {
        return false;
    }
    if (plen && read_full(s, payload, plen)) {
        free(payload);
        return false;
    }

    bool ok = true;
    switch (type) {
        case SC_DUPLEX_MSG_CTRL_POINTER:
            if (plen != SC_DUPLEX_CTRL_POINTER_LEN) {
                fprintf(stderr, "sc-reverse-host: bad pointer len %u\n", plen);
                ok = false;
                break;
            }
            ok = duplex_apply_pointer(payload);
            break;
        case SC_DUPLEX_MSG_CTRL_KEY:
            ok = duplex_apply_key(payload, plen);
            break;
        case SC_DUPLEX_MSG_CTRL_WHEEL:
            ok = duplex_apply_wheel(payload, plen);
            break;
        default:
            break;
    }

    free(payload);
    return ok;
}

#ifdef _WIN32
# ifdef HAVE_DUPLEX_VIDEO

typedef struct {
    sc_sock_t sock;
    HANDLE stop_event;
} duplex_video_ctx;

static DWORD WINAPI
duplex_video_thread(LPVOID param) {
    duplex_video_ctx *v = (duplex_video_ctx *) param;
    uint32_t dw = duplex_dxgi_width();
    uint32_t dh = duplex_dxgi_height();
    size_t bsz = (size_t) dw * (size_t) dh * 4;
    uint8_t *bgra = malloc(bsz);
    if (!bgra) {
        fprintf(stderr, "sc-reverse-host: OOM BGRA buffer\n");
        return 1;
    }

    struct duplex_encoder *enc =
        duplex_encoder_open((int) dw, (int) dh);
    if (!enc) {
        fprintf(stderr, "sc-reverse-host: encoder init failed\n");
        free(bgra);
        return 1;
    }

    const uint8_t *ex = NULL;
    size_t exlen = 0;
    if (duplex_encoder_extradata(enc, &ex, &exlen)) {
        if (!send_framed(v->sock, SC_DUPLEX_MSG_VIDEO_CODEC_CONFIG, ex,
                         (uint32_t) exlen)) {
            fprintf(stderr, "sc-reverse-host: codec config send failed\n");
        }
    }

    for (;;) {
        if (WaitForSingleObject(v->stop_event, 0) == WAIT_OBJECT_0) {
            break;
        }

        if (!duplex_dxgi_grab_frame(bgra, bsz)) {
            Sleep(12);
            continue;
        }

        bool key;
        const uint8_t *pkt;
        size_t pkt_len;

        while (duplex_encoder_encode(enc, bgra, &key, &pkt, &pkt_len)) {
            if (!send_framed(v->sock, SC_DUPLEX_MSG_VIDEO_NAL, pkt,
                             (uint32_t) pkt_len)) {
                goto finish;
            }
        }
        Sleep(20);
    }

finish:
    duplex_encoder_close(enc);
    free(bgra);
    return 0;
}

# endif /* HAVE_DUPLEX_VIDEO */
#endif /* _WIN32 */

static void
usage(const char *argv0) {
    fprintf(stderr,
            "sc-reverse-host (duplex experimental) — mirror PC to Android viewer\n"
            "\n"
            "Usage: %s [--port N]\n"
            "  Default port: 27183 (use adb reverse tcp:27183 tcp:27183)\n",
            argv0);
}

int
main(int argc, char **argv) {
    unsigned port = 27183;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
            usage(argv[0]);
            return 0;
        }
        if (!strcmp(argv[i], "--port") && i + 1 < argc) {
            port = (unsigned) strtoul(argv[++i], NULL, 10);
        } else {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            usage(argv[0]);
            return 2;
        }
    }

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa)) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }
#endif

    sc_sock_t ls = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ls == SC_SOCK_INVALID) {
        perror("socket");
        return 1;
    }

    int yes = 1;
    setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, (const char *) &yes, sizeof yes);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t) port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(ls, (struct sockaddr *) &addr, sizeof addr)) {
        fprintf(stderr, "bind: port %u (%d)\n", port, sc_sock_errno);
        sc_sock_close(ls);
        return 1;
    }

    if (listen(ls, 1)) {
        perror("listen");
        sc_sock_close(ls);
        return 1;
    }

    duplex_input_init();

    printf("sc-reverse-host: listening on 127.0.0.1:%u\n", port);
    printf("Run on the PC: adb reverse tcp:%u tcp:%u\n", port, port);
    printf("Then start the Android \"Duplex Reverse View\" app on the device.\n");

    for (;;) {
        sc_sock_t s = accept(ls, NULL, NULL);
        if (s == SC_SOCK_INVALID) {
            perror("accept");
            break;
        }

#ifdef _WIN32
# ifdef HAVE_DUPLEX_VIDEO
        bool video_ok = duplex_dxgi_init();
        uint32_t hdr_w;
        uint32_t hdr_h;
        uint32_t flags = 0;

        if (video_ok) {
            hdr_w = duplex_dxgi_width() & ~1u;
            hdr_h = duplex_dxgi_height() & ~1u;
            flags = SC_DUPLEX_FLAG_HOST_HAS_VIDEO;
        } else {
            hdr_w = duplex_host_width();
            hdr_h = duplex_host_height();
        }

        if (!send_header(s, hdr_w, hdr_h, flags)) {
            if (video_ok) {
                duplex_dxgi_shutdown();
            }
            sc_sock_close(s);
            continue;
        }

        HANDLE stop_event = NULL;
        HANDLE video_thr = NULL;
        duplex_video_ctx *vctx = NULL;

        if (video_ok) {
            vctx = malloc(sizeof(*vctx));
            if (!vctx) {
                duplex_dxgi_shutdown();
                sc_sock_close(s);
                continue;
            }
            stop_event = CreateEventW(NULL, TRUE, FALSE, NULL);
            if (!stop_event) {
                free(vctx);
                duplex_dxgi_shutdown();
                sc_sock_close(s);
                continue;
            }
            vctx->sock = s;
            vctx->stop_event = stop_event;

            video_thr = CreateThread(NULL, 0, duplex_video_thread, vctx, 0,
                                     NULL);
            if (!video_thr) {
                CloseHandle(stop_event);
                free(vctx);
                duplex_dxgi_shutdown();
                sc_sock_close(s);
                continue;
            }
        }

        while (handle_control(s)) {
        }

        if (stop_event) {
            SetEvent(stop_event);
        }
        if (video_thr) {
            WaitForSingleObject(video_thr, INFINITE);
            CloseHandle(video_thr);
            free(vctx);
        }
        if (stop_event) {
            CloseHandle(stop_event);
        }
        if (video_ok) {
            duplex_dxgi_shutdown();
        }

        sc_sock_close(s);
        printf("sc-reverse-host: client disconnected\n");

# else /* !HAVE_DUPLEX_VIDEO */

        uint32_t w = duplex_host_width();
        uint32_t h = duplex_host_height();
        uint32_t flags = duplex_host_has_video() ? SC_DUPLEX_FLAG_HOST_HAS_VIDEO : 0;
        if (!send_header(s, w, h, flags)) {
            sc_sock_close(s);
            continue;
        }

        while (handle_control(s)) {
        }

        sc_sock_close(s);
        printf("sc-reverse-host: client disconnected\n");

# endif

#else /* !_WIN32 */

        uint32_t w = duplex_host_width();
        uint32_t h = duplex_host_height();
        uint32_t flags = duplex_host_has_video() ? SC_DUPLEX_FLAG_HOST_HAS_VIDEO : 0;
        if (!send_header(s, w, h, flags)) {
            sc_sock_close(s);
            continue;
        }

        while (handle_control(s)) {
        }

        sc_sock_close(s);
        printf("sc-reverse-host: client disconnected\n");

#endif
    }

    sc_sock_close(ls);
#ifdef _WIN32
    WSACleanup();
#endif
    duplex_input_destroy();
    return 0;
}
