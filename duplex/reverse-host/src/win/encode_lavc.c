/*
 * H.264 encoder via libavcodec (libx264 or system encoder).
 */

#include "duplex_encode.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libavcodec/avcodec.h>
#include <libavutil/error.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>

struct duplex_encoder {
    AVCodecContext *ctx;
    struct SwsContext *sws;
    AVFrame *frame;
    AVPacket *pkt;
    int64_t pts;
    int src_w;
    int src_h;
    int enc_w;
    int enc_h;
    uint8_t *pkt_copy;
    size_t pkt_cap;
};

struct duplex_encoder *
duplex_encoder_open(int width, int height) {
    int enc_w = width & ~1;
    int enc_h = height & ~1;
    if (enc_w < 16 || enc_h < 16) {
        fprintf(stderr, "duplex_encode: dimensions too small\n");
        return NULL;
    }

    const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        fprintf(stderr, "duplex_encode: no H.264 encoder (install libavcodec with "
                        "libx264)\n");
        return NULL;
    }

    struct duplex_encoder *e = calloc(1, sizeof(*e));
    if (!e) {
        return NULL;
    }

    e->src_w = width;
    e->src_h = height;
    e->enc_w = enc_w;
    e->enc_h = enc_h;

    e->ctx = avcodec_alloc_context3(codec);
    if (!e->ctx) {
        free(e);
        return NULL;
    }

    e->ctx->width = enc_w;
    e->ctx->height = enc_h;
    e->ctx->time_base = (AVRational){1, 30};
    e->ctx->framerate = (AVRational){30, 1};
    e->ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    e->ctx->bit_rate = 4500000;
    e->ctx->gop_size = 30;
    e->ctx->max_b_frames = 0;

    if (e->ctx->priv_data && codec->priv_data_class) {
        av_opt_set(e->ctx->priv_data, "preset", "ultrafast", 0);
        av_opt_set(e->ctx->priv_data, "tune", "zerolatency", 0);
    }

    int err = avcodec_open2(e->ctx, codec, NULL);
    if (err < 0) {
        char buf[128];
        av_strerror(err, buf, sizeof buf);
        fprintf(stderr, "duplex_encode: avcodec_open2 failed: %s\n", buf);
        avcodec_free_context(&e->ctx);
        free(e);
        return NULL;
    }

    e->frame = av_frame_alloc();
    e->pkt = av_packet_alloc();
    if (!e->frame || !e->pkt) {
        goto fail;
    }

    e->frame->format = AV_PIX_FMT_YUV420P;
    e->frame->width = enc_w;
    e->frame->height = enc_h;
    err = av_frame_get_buffer(e->frame, 32);
    if (err < 0) {
        goto fail;
    }

    e->sws = sws_getContext(width, height, AV_PIX_FMT_BGRA, enc_w, enc_h,
                            AV_PIX_FMT_YUV420P, SWS_BILINEAR, NULL, NULL, NULL);
    if (!e->sws) {
        fprintf(stderr, "duplex_encode: sws_getContext failed\n");
        goto fail;
    }

    e->pts = 0;
    return e;

fail:
    if (e->sws) {
        sws_freeContext(e->sws);
    }
    av_packet_free(&e->pkt);
    av_frame_free(&e->frame);
    avcodec_free_context(&e->ctx);
    free(e);
    return NULL;
}

void
duplex_encoder_close(struct duplex_encoder *e) {
    if (!e) {
        return;
    }
    free(e->pkt_copy);
    if (e->sws) {
        sws_freeContext(e->sws);
    }
    av_packet_free(&e->pkt);
    av_frame_free(&e->frame);
    avcodec_free_context(&e->ctx);
    free(e);
}

bool
duplex_encoder_extradata(struct duplex_encoder *e, const uint8_t **data,
                         size_t *len) {
    if (!e || !e->ctx || e->ctx->extradata_size <= 0) {
        return false;
    }
    *data = e->ctx->extradata;
    *len = (size_t) e->ctx->extradata_size;
    return true;
}

bool
duplex_encoder_encode(struct duplex_encoder *e, const uint8_t *bgra,
                        bool *is_key, const uint8_t **pkt, size_t *pkt_len) {
    const uint8_t *srcslice[4] = {bgra, NULL, NULL, NULL};
    int srcstride[4] = {(int) ((size_t) e->src_w * 4), 0, 0, 0};

    int err = av_frame_make_writable(e->frame);
    if (err < 0) {
        return false;
    }

    sws_scale(e->sws, srcslice, srcstride, 0, e->src_h, e->frame->data,
              e->frame->linesize);

    e->frame->pts = e->pts++;

    err = avcodec_send_frame(e->ctx, e->frame);
    if (err < 0) {
        return false;
    }

    err = avcodec_receive_packet(e->ctx, e->pkt);
    if (err == AVERROR(EAGAIN) || err == AVERROR_EOF) {
        return false;
    }
    if (err < 0) {
        return false;
    }

    if (e->pkt->size > (int) e->pkt_cap) {
        uint8_t *n = realloc(e->pkt_copy, (size_t) e->pkt->size);
        if (!n) {
            av_packet_unref(e->pkt);
            return false;
        }
        e->pkt_copy = n;
        e->pkt_cap = (size_t) e->pkt->size;
    }
    memcpy(e->pkt_copy, e->pkt->data, (size_t) e->pkt->size);

    *is_key = (e->pkt->flags & AV_PKT_FLAG_KEY) != 0;
    *pkt = e->pkt_copy;
    *pkt_len = (size_t) e->pkt->size;

    av_packet_unref(e->pkt);
    return true;
}
