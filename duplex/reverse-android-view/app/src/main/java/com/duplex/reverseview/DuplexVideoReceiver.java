package com.duplex.reverseview;

import android.media.MediaCodec;
import android.media.MediaFormat;
import android.view.Surface;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Reads VIDEO_CODEC_CONFIG + VIDEO_NAL from the TCP stream after the 24-byte header.
 */
final class DuplexVideoReceiver implements Runnable {

    static final int MSG_VIDEO_CODEC_CONFIG = 0x100;
    static final int MSG_VIDEO_NAL = 0x101;

    private final InputStream in;
    private final int width;
    private final int height;
    private final AtomicBoolean stopped;
    private final Runnable onIOError;

    private volatile Surface surface;
    private byte[] pendingCsd;

    private MediaCodec codec;
    private long ptsUs;

    DuplexVideoReceiver(InputStream in, int width, int height,
                        AtomicBoolean stopped, Runnable onIOError) {
        this.in = in;
        this.width = width;
        this.height = height;
        this.stopped = stopped;
        this.onIOError = onIOError;
    }

    synchronized void setSurface(Surface s) {
        this.surface = s;
        tryStartCodecLocked();
    }

    void releaseCodec() {
        synchronized (this) {
            if (codec != null) {
                try {
                    codec.stop();
                } catch (Exception ignored) {
                }
                codec.release();
                codec = null;
            }
        }
    }

    private synchronized void tryStartCodecLocked() {
        if (codec != null || surface == null || !surface.isValid() || pendingCsd == null) {
            return;
        }
        try {
            MediaFormat fmt =
                    MediaFormat.createVideoFormat(MediaFormat.MIMETYPE_VIDEO_AVC,
                            width, height);
            fmt.setByteBuffer("csd-0", ByteBuffer.wrap(pendingCsd));
            codec = MediaCodec.createDecoderByType(MediaFormat.MIMETYPE_VIDEO_AVC);
            codec.configure(fmt, surface, null, 0);
            codec.start();
            ptsUs = 0;
        } catch (IOException e) {
            codec = null;
        }
    }

    @Override
    public void run() {
        byte[] fh = new byte[8];

        try {
            while (!stopped.get()) {
                readFully(in, fh);
                int type = readLe32(fh, 0);
                int len = readLe32(fh, 4);
                if (len < 0 || len > 8 * 1024 * 1024) {
                    break;
                }
                byte[] payload = len > 0 ? new byte[len] : new byte[0];
                if (len > 0) {
                    readFully(in, payload);
                }

                if (type == MSG_VIDEO_CODEC_CONFIG) {
                    synchronized (this) {
                        pendingCsd = payload;
                        tryStartCodecLocked();
                    }
                } else if (type == MSG_VIDEO_NAL) {
                    queueEncodedFrame(payload);
                }
            }
        } catch (IOException e) {
            if (!stopped.get() && onIOError != null) {
                onIOError.run();
            }
        } finally {
            releaseCodec();
        }
    }

    private void queueEncodedFrame(byte[] nal) throws IOException {
        MediaCodec c;
        synchronized (this) {
            c = codec;
        }
        if (c == null || nal.length == 0) {
            return;
        }
        int tries = 64;
        while (tries-- > 0 && !stopped.get()) {
            int inIx = c.dequeueInputBuffer(30_000);
            if (inIx >= 0) {
                ByteBuffer buf = c.getInputBuffer(inIx);
                if (buf != null && nal.length <= buf.remaining()) {
                    buf.clear();
                    buf.put(nal);
                    c.queueInputBuffer(inIx, 0, nal.length, ptsUs, 0);
                    ptsUs += 33_333;
                }
                drainOutputs(c);
                return;
            }
            drainOutputs(c);
        }
    }

    private void drainOutputs(MediaCodec c) {
        MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
        for (int i = 0; i < 16; i++) {
            int outIx = c.dequeueOutputBuffer(info, 0);
            if (outIx == MediaCodec.INFO_TRY_AGAIN_LATER) {
                break;
            }
            if (outIx == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED
                    || outIx == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
                continue;
            }
            if (outIx >= 0) {
                c.releaseOutputBuffer(outIx, info.size > 0);
            }
        }
    }

    private static int readLe32(byte[] b, int o) {
        return (b[o] & 0xff)
                | ((b[o + 1] & 0xff) << 8)
                | ((b[o + 2] & 0xff) << 16)
                | ((b[o + 3] & 0xff) << 24);
    }

    private static void readFully(InputStream in, byte[] buf) throws IOException {
        int off = 0;
        while (off < buf.length) {
            int n = in.read(buf, off, buf.length - off);
            if (n < 0) {
                throw new IOException("EOF");
            }
            off += n;
        }
    }
}
