#ifndef DUPLEX_INPUT_H
#define DUPLEX_INPUT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void duplex_input_init(void);
void duplex_input_destroy(void);

uint32_t duplex_host_width(void);
uint32_t duplex_host_height(void);
bool duplex_host_has_video(void);

bool duplex_apply_pointer(const uint8_t *payload /* SC_DUPLEX_CTRL_POINTER_LEN */);
bool duplex_apply_key(const uint8_t *payload, size_t len);
bool duplex_apply_wheel(const uint8_t *payload, size_t len);

#endif
