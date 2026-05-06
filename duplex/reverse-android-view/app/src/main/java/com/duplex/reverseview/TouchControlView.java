package com.duplex.reverseview;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;

import java.io.IOException;
import java.io.OutputStream;

/**
 * Sends {@link DuplexWire#MSG_CTRL_POINTER} over the outbound PC stream.
 */
public class TouchControlView extends View {

    private OutputStream out;
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
        hintPaint.setTextSize(28f);
        hintPaint.setColor(0x88FFFFFF);
        setBackgroundColor(0x00000000);
    }

    public void bind(OutputStream os) {
        this.out = os;
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (out == null) {
            return;
        }
        canvas.drawText("Touch: remote desktop control", 24f, getHeight() - 48f,
                hintPaint);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (out == null) {
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
            byte[] pl = DuplexWire.pointerPayload(0, xn, yn, buttons);
            DuplexWire.writeFramedMessage(out, DuplexWire.MSG_CTRL_POINTER, pl);
            lastButtons = buttons;
        } catch (IOException e) {
            return false;
        }
        return true;
    }
}
