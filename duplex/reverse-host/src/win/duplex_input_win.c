#include "duplex_input.h"

#include <stddef.h>

#include "sc_duplex_proto.h"

#include <stdio.h>
#include <string.h>

#include <windows.h>

static uint32_t last_buttons;

void
duplex_input_init(void) {
    last_buttons = 0;
}

void
duplex_input_destroy(void) {
}

uint32_t
duplex_host_width(void) {
    return (uint32_t) GetSystemMetrics(SM_CXSCREEN);
}

uint32_t
duplex_host_height(void) {
    return (uint32_t) GetSystemMetrics(SM_CYSCREEN);
}

bool
duplex_host_has_video(void) {
#ifdef HAVE_DUPLEX_VIDEO
    /* Actual header flags come from DXGI + encoder init in reverse_host.c */
    return true;
#else
    return false;
#endif
}

bool
duplex_apply_pointer(const uint8_t *payload) {
    (void) sc_duplex_read_u32(payload); /* pointer_id */
    float xn = sc_duplex_read_f32(payload + 4);
    float yn = sc_duplex_read_f32(payload + 8);
    uint32_t buttons = sc_duplex_read_u32(payload + 12);

    if (xn < 0.f) {
        xn = 0.f;
    }
    if (xn > 1.f) {
        xn = 1.f;
    }
    if (yn < 0.f) {
        yn = 0.f;
    }
    if (yn > 1.f) {
        yn = 1.f;
    }

    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);
    int x = (int) (xn * (w > 1 ? w - 1 : 1));
    int y = (int) (yn * (h > 1 ? h - 1 : 1));

    SetCursorPos(x, y);

    uint32_t changed = buttons ^ last_buttons;
    INPUT in[2];
    memset(in, 0, sizeof in);
    in[0].type = INPUT_MOUSE;
    in[1].type = INPUT_MOUSE;

    size_t n = 0;
    if (changed & 1u) {
        in[n].mi.dwFlags = (buttons & 1u) ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
        ++n;
    }
    if (changed & 2u) {
        in[n].mi.dwFlags = (buttons & 2u) ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
        ++n;
    }
    if (changed & 4u) {
        in[n].mi.dwFlags = (buttons & 4u) ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
        ++n;
    }

    if (n) {
        UINT sent = SendInput((UINT) n, in, sizeof(INPUT));
        if (sent != n) {
            fprintf(stderr, "SendInput mouse failed (%u)\n", (unsigned) GetLastError());
        }
    }

    last_buttons = buttons;
    return true;
}

bool
duplex_apply_key(const uint8_t *payload, size_t len) {
    if (len < 3) {
        return false;
    }
    uint16_t vk = sc_duplex_read_u16(payload);
    uint8_t down = payload[2];

    INPUT in;
    memset(&in, 0, sizeof in);
    in.type = INPUT_KEYBOARD;
    in.ki.wVk = vk;
    in.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP;

    if (!SendInput(1, &in, sizeof in)) {
        fprintf(stderr, "SendInput key failed\n");
        return false;
    }
    return true;
}

bool
duplex_apply_wheel(const uint8_t *payload, size_t len) {
    if (len < 4) {
        return false;
    }
    /* v1: first int32 is scroll delta in wheel ticks (WHEEL_DELTA units) */
    int32_t delta = (int32_t) sc_duplex_read_u32(payload);

    INPUT in;
    memset(&in, 0, sizeof in);
    in.type = INPUT_MOUSE;
    in.mi.dwFlags = MOUSEEVENTF_WHEEL;
    in.mi.mouseData = (DWORD) delta;

    if (!SendInput(1, &in, sizeof in)) {
        return false;
    }
    return true;
}
