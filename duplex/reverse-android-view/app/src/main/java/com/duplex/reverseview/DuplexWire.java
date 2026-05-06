package com.duplex.reverseview;

import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/** Client-side helpers for duplex-protocol-v1 control messages. */
final class DuplexWire {

    static final int MSG_CTRL_POINTER = 0x200;

    private DuplexWire() {
    }

    static void writeFramedMessage(OutputStream out, int type, byte[] payload)
            throws IOException {
        byte[] hdr = new byte[8];
        ByteBuffer.wrap(hdr).order(ByteOrder.LITTLE_ENDIAN).putInt(type).putInt(payload.length);
        synchronized (out) {
            out.write(hdr);
            if (payload.length > 0) {
                out.write(payload);
            }
            out.flush();
        }
    }

    static byte[] pointerPayload(int pointerId, float xNorm, float yNorm, int buttons) {
        byte[] p = new byte[16];
        ByteBuffer bb = ByteBuffer.wrap(p).order(ByteOrder.LITTLE_ENDIAN);
        bb.putInt(pointerId);
        bb.putFloat(xNorm);
        bb.putFloat(yNorm);
        bb.putInt(buttons);
        return p;
    }
}
