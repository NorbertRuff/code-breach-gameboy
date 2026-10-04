#include <gb/gb.h>
#include "utils.h"

// Waits a number of frames.
void wait_frames(uint16_t frames)
{
    for (uint16_t i = 0; i < frames; i++) {
        wait_vbl_done();
    }
}
