package com.duplex.reverseview;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;

import java.io.IOException;
import java.io.OutputStream;
import java.net.Socket;

/**
 * Sends {@link DuplexWire#MSG_CTRL_POINTER} for normalized coordinates.
 * Full-screen video will replace this region once MediaCodec decode lands.
 */
public class TouchControlView extends View {

    private Socket socket;
    private int lastButtons;

    private final Paint hintPaint = new Paint(Paint.ANTI_ALIAS_FLAG);

    public TouchControlView(Context c) {
        super(c);
        init();
    }

    public TouchControlView(Context c, AttributeSet a) {
        super(c, a);
        init();
    }

    private void init() {
        hintPaint.setTextSize(36f);
        hintPaint.setColor(0xFF666666);
        setBackgroundColor(0x22000000);
    }

    public void bind(Socket s) {
        this.socket = s;
        setVisibility(VISIBLE);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        canvas.drawText("Touch here → moves PC mouse (duplex v1)", 24f,
                getHeight() / 2f, hintPaint);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (socket == null) {
            return false;
        }
        int action = event.getActionMasked();
        float xn = event.getX() / Math.max(1f, getWidth());
        float yn = event.getY() / Math.max(1f, getHeight());

        int buttons = lastButtons;
        switch (action) {
            case MotionEvent.ACTION_DOWN:
            case MotionEvent.ACTION_POINTER_DOWN:
                buttons |= 1;
                break;
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_POINTER_UP:
            case MotionEvent.ACTION_CANCEL:
                buttons &= ~1;
                break;
            default:
                break;
        }

        try {
            OutputStream out = socket.getOutputStream();
            byte[] pl = DuplexWire.pointerPayload(0, xn, yn, buttons);
            DuplexWire.writeFramedMessage(out, DuplexWire.MSG_CTRL_POINTER, pl);
            lastButtons = buttons;
        } catch (IOException e) {
            return false;
        }
        return true;
    }
}
