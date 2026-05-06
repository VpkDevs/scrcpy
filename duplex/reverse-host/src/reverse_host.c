/*
 * sc-reverse-host — PC side of duplex mirroring (experimental).
 * Listens on loopback, sends stream_caps, applies control messages from Android.
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
            /* ignore unknown */
            break;
    }

    free(payload);
    return ok;
}

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
    }

    sc_sock_close(ls);
#ifdef _WIN32
    WSACleanup();
#endif
    duplex_input_destroy();
    return 0;
}
