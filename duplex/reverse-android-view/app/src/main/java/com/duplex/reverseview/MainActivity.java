package com.duplex.reverseview;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.Gravity;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.io.IOException;
import java.io.InputStream;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;

/**
 * Duplex Reverse View: receives optional H.264 from sc-reverse-host (after the v1
 * header) and sends touch control on the same TCP session.
 */
public class MainActivity extends Activity implements SurfaceHolder.Callback {

    private static final String HOST = "127.0.0.1";
    private static final int PORT = 27183;

    private final AtomicBoolean stopped = new AtomicBoolean(false);
    private final AtomicReference<Socket> socketRef = new AtomicReference<>();
    private final Handler mainHandler = new Handler(Looper.getMainLooper());

    private TextView status;
    private TouchControlView touchPad;
    private SurfaceView surfaceView;
    private DuplexVideoReceiver videoReceiver;

    private int hdrWidth;
    private int hdrHeight;
    private boolean hdrVideo;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setGravity(Gravity.CENTER_HORIZONTAL);

        status = new TextView(this);
        status.setText(R.string.status_connecting);
        root.addView(status);

        FrameLayout videoWrap = new FrameLayout(this);
        videoWrap.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, 0, 1f));

        surfaceView = new SurfaceView(this);
        surfaceView.getHolder().addCallback(this);
        videoWrap.addView(surfaceView,
                new FrameLayout.LayoutParams(
                        FrameLayout.LayoutParams.MATCH_PARENT,
                        FrameLayout.LayoutParams.MATCH_PARENT));

        touchPad = new TouchControlView(this);
        touchPad.setLayoutParams(new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                FrameLayout.LayoutParams.MATCH_PARENT));
        videoWrap.addView(touchPad);

        root.addView(videoWrap);

        setContentView(root);

        new Thread(this::connectPipeline, "duplex-connect").start();
    }

    private void connectPipeline() {
        try {
            Socket sock = new Socket();
            sock.connect(new InetSocketAddress(HOST, PORT), 15_000);
            socketRef.set(sock);

            InputStream in = sock.getInputStream();
            byte[] hdr = new byte[24];
            readFully(in, hdr);

            if (!new String(hdr, 0, 8, StandardCharsets.US_ASCII).equals("DUPLEX10")) {
                try {
                    sock.close();
                } catch (IOException ignored) {
                }
                socketRef.compareAndSet(sock, null);
                postErr("Bad magic from host (run sc-reverse-host on PC?)");
                return;
            }

            ByteBuffer bb = ByteBuffer.wrap(hdr).order(ByteOrder.LITTLE_ENDIAN);
            bb.position(8);
            bb.getShort(); /* version */
            int flags = bb.getShort() & 0xffff;
            hdrWidth = bb.getInt();
            hdrHeight = bb.getInt();
            bb.getInt(); /* codec tag */
            hdrVideo = (flags & 1) != 0;

            touchPad.bind(sock.getOutputStream());

            if (hdrVideo) {
                videoReceiver =
                        new DuplexVideoReceiver(in, hdrWidth, hdrHeight, stopped,
                                () -> postErr("Video stream ended unexpectedly"));
                new Thread(videoReceiver, "duplex-video").start();
                mainHandler.post(() -> {
                    SurfaceHolder h = surfaceView.getHolder();
                    if (h.getSurface().isValid()) {
                        videoReceiver.setSurface(h.getSurface());
                    }
                });
            }

            String msg = "Duplex v1 connected\nhost stream " + hdrWidth + "x" + hdrHeight
                    + (hdrVideo ? "\n\nDrag on screen to move PC mouse." : "");
            mainHandler.post(() -> status.setText(msg));

        } catch (IOException e) {
            postErr("Connect failed: " + e.getMessage()
                    + "\nadb reverse tcp:" + PORT + " tcp:" + PORT);
        }
    }

    private void postErr(String text) {
        mainHandler.post(() -> status.setText(text));
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
    public void surfaceCreated(SurfaceHolder holder) {
        if (videoReceiver != null) {
            videoReceiver.setSurface(holder.getSurface());
        }
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        if (videoReceiver != null) {
            videoReceiver.setSurface(null);
        }
    }

    private void shutdown() {
        stopped.set(true);
        if (videoReceiver != null) {
            videoReceiver.releaseCodec();
        }
        Socket s = socketRef.getAndSet(null);
        if (s != null) {
            try {
                s.close();
            } catch (IOException ignored) {
            }
        }
    }

    @Override
    public void onBackPressed() {
        shutdown();
        super.onBackPressed();
    }

    @Override
    protected void onDestroy() {
        shutdown();
        super.onDestroy();
    }
}
