#ifndef DUPLEX_ENCODE_H
#define DUPLEX_ENCODE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct duplex_encoder;

struct duplex_encoder *
duplex_encoder_open(int width, int height);

void
duplex_encoder_close(struct duplex_encoder *enc);

/**
 * Encode one BGRA frame (width*height*4 bytes, tightly packed).
 * On success sets *pkt / *len to encoded buffer (owned by encoder until next call).
 * *is_key set when IDR produced.
 */
bool
duplex_encoder_encode(struct duplex_encoder *enc, const uint8_t *bgra,
                        bool *is_key, const uint8_t **pkt, size_t *pkt_len);

/**
 * Codec extradata (AVCDec) if present — valid after successful open or first keyframe.
 */
bool
duplex_encoder_extradata(struct duplex_encoder *enc, const uint8_t **data,
                         size_t *len);

#endif
