#ifndef SC_DUPLEX_PROTO_H
#define SC_DUPLEX_PROTO_H

#include <stdint.h>

#define SC_DUPLEX_MAGIC "DUPLEX10"
#define SC_DUPLEX_MAGIC_LEN 8
#define SC_DUPLEX_HEADER_LEN 24
#define SC_DUPLEX_VERSION 1

#define SC_DUPLEX_FLAG_HOST_HAS_VIDEO (1u << 0)

#define SC_DUPLEX_CODEC_H264 0x34363268u /* 'h264' little-endian */

enum sc_duplex_msg_host {
    SC_DUPLEX_MSG_VIDEO_CODEC_CONFIG = 0x100,
    SC_DUPLEX_MSG_VIDEO_NAL = 0x101,
    SC_DUPLEX_MSG_HOST_CLIPBOARD = 0x102,
};

enum sc_duplex_msg_device {
    SC_DUPLEX_MSG_CTRL_POINTER = 0x200,
    SC_DUPLEX_MSG_CTRL_KEY = 0x201,
    SC_DUPLEX_MSG_CTRL_WHEEL = 0x202,
    SC_DUPLEX_MSG_DEVICE_CLIPBOARD = 0x203,
};

#define SC_DUPLEX_CTRL_POINTER_LEN 16

static inline void
sc_duplex_write_u16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t) (v & 0xff);
    p[1] = (uint8_t) ((v >> 8) & 0xff);
}

static inline void
sc_duplex_write_u32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t) (v & 0xff);
    p[1] = (uint8_t) ((v >> 8) & 0xff);
    p[2] = (uint8_t) ((v >> 16) & 0xff);
    p[3] = (uint8_t) ((v >> 24) & 0xff);
}

static inline uint32_t
sc_duplex_read_u32(const uint8_t *p) {
    return (uint32_t) p[0] | ((uint32_t) p[1] << 8) | ((uint32_t) p[2] << 16) |
           ((uint32_t) p[3] << 24);
}

static inline uint16_t
sc_duplex_read_u16(const uint8_t *p) {
    return (uint16_t) p[0] | ((uint16_t) p[1] << 8);
}

static inline void
sc_duplex_write_f32(uint8_t *p, float f) {
    union {
        float f;
        uint32_t u;
    } x;
    x.f = f;
    sc_duplex_write_u32(p, x.u);
}

static inline float
sc_duplex_read_f32(const uint8_t *p) {
    union {
        float f;
        uint32_t u;
    } x;
    x.u = sc_duplex_read_u32(p);
    return x.f;
}

#endif
