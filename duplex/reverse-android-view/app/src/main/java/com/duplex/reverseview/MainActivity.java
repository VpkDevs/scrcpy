package com.duplex.reverseview;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.io.IOException;
import java.io.InputStream;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.atomic.AtomicReference;

/**
 * Duplex viewer: connects via adb reverse, reads v1 header, then touch events
 * control the PC (with sc-reverse-host on Windows). MediaCodec video decode
 * is the next step.
 */
public class MainActivity extends Activity {

    private static final String HOST = "127.0.0.1";
    private static final int PORT = 27183;

    private final AtomicReference<Socket> socketRef = new AtomicReference<>();
    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private LinearLayout root;
    private TextView status;
    private TouchControlView touchPad;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setGravity(Gravity.CENTER_HORIZONTAL);
        int pad = (int) (16 * getResources().getDisplayMetrics().density);
        root.setPadding(pad, pad, pad, pad);

        status = new TextView(this);
        status.setText(R.string.status_connecting);
        root.addView(status);

        touchPad = new TouchControlView(this);
        int h = (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 220f,
                getResources().getDisplayMetrics());
        touchPad.setLayoutParams(new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, h));
        touchPad.setVisibility(android.view.View.GONE);
        root.addView(touchPad);

        Button disconnect = new Button(this);
        disconnect.setText(R.string.disconnect);
        disconnect.setOnClickListener(v -> closeSocketAndFinish());
        root.addView(disconnect);

        setContentView(root);

        new Thread(this::connectAndReadHeader, "duplex-socket").start();
    }

    private void closeSocketAndFinish() {
        Socket s = socketRef.getAndSet(null);
        if (s != null) {
            try {
                s.close();
            } catch (IOException ignored) {
            }
        }
        finish();
    }

    private void connectAndReadHeader() {
        try {
            Socket s = new Socket();
            s.connect(new InetSocketAddress(HOST, PORT), 10_000);
            socketRef.set(s);

            InputStream in = s.getInputStream();
            byte[] hdr = new byte[24];
            readFully(in, hdr);

            if (!new String(hdr, 0, 8, StandardCharsets.US_ASCII).equals("DUPLEX10")) {
                postUi(() -> status.setText(
                        "Bad magic from host (is sc-reverse-host running?)"));
                return;
            }

            ByteBuffer bb = ByteBuffer.wrap(hdr).order(ByteOrder.LITTLE_ENDIAN);
            bb.position(8);
            int version = bb.getShort() & 0xffff;
            int flags = bb.getShort() & 0xffff;
            int w = bb.getInt();
            int h = bb.getInt();
            int codec = bb.getInt();

            byte[] cb = new byte[]{(byte) codec, (byte) (codec >> 8), (byte) (codec >> 16),
                    (byte) (codec >> 24)};
            String codecStr = new String(cb, StandardCharsets.US_ASCII);

            String msg = "Duplex v1 OK\nversion=" + version + " flags=0x"
                    + Integer.toHexString(flags)
                    + "\nhost " + w + "x" + h + "\ncodec=" + codecStr.trim()
                    + "\n\nDrag on the pad below to move the PC cursor.";
            postUi(() -> {
                status.setText(msg);
                touchPad.bind(s);
                touchPad.invalidate();
            });
        } catch (IOException e) {
            String err = "Connect failed: " + e.getMessage()
                    + "\n\nOn PC: adb reverse tcp:" + PORT + " tcp:" + PORT;
            postUi(() -> status.setText(err));
        }
    }

    private void postUi(Runnable r) {
        mainHandler.post(r);
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

    @Override
    protected void onDestroy() {
        Socket s = socketRef.getAndSet(null);
        if (s != null) {
            try {
                s.close();
            } catch (IOException ignored) {
            }
        }
        super.onDestroy();
    }
}
