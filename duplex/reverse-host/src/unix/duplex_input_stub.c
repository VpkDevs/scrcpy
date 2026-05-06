#include "duplex_input.h"

#include <stdio.h>

void
duplex_input_init(void) {
}

void
duplex_input_destroy(void) {
}

uint32_t
duplex_host_width(void) {
    return 1920;
}

uint32_t
duplex_host_height(void) {
    return 1080;
}

bool
duplex_host_has_video(void) {
    return false;
}

bool
duplex_apply_pointer(const uint8_t *payload) {
    (void) payload;
    fprintf(stderr,
            "sc-reverse-host: pointer injection is implemented on Windows; "
            "this build accepts connections for protocol testing only.\n");
    return true;
}

bool
duplex_apply_key(const uint8_t *payload, size_t len) {
    (void) payload;
    (void) len;
    return true;
}

bool
duplex_apply_wheel(const uint8_t *payload, size_t len) {
    (void) payload;
    (void) len;
    return true;
}
