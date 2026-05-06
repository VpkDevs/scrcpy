#ifndef DUPLEX_CAPTURE_H
#define DUPLEX_CAPTURE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * DXGI desktop duplication (Windows). Call from the capture thread only.
 */
bool duplex_dxgi_init(void);
void duplex_dxgi_shutdown(void);

uint32_t duplex_dxgi_width(void);
uint32_t duplex_dxgi_height(void);

/**
 * Copy current desktop frame into @p out_bgra (caller-provided buffer).
 * Row pitch in bytes is width * 4 (tightly packed BGRA).
 */
bool duplex_dxgi_grab_frame(uint8_t *out_bgra, size_t out_size);

#endif
