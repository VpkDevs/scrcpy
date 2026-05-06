/*
 * Placeholder for DXGI Desktop Duplication + hardware H.264 (Media Foundation
 * or vendor encoder). When implemented, duplex_host_has_video() should query
 * this module and VIDEO_NAL frames should be emitted on the duplex socket.
 */

#include <stdbool.h>

bool
duplex_capture_init(void) {
    return false;
}

void
duplex_capture_shutdown(void) {
}
