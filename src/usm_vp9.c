#include "usm_vp9.h"

bool usm_vp9_is_keyframe(const uint8_t *frame_data, size_t frame_size)
{
    if (frame_data == NULL || frame_size == 0) {
        return false;
    }

    const uint8_t marker = frame_data[0] & 0x03;
    const bool show_existing_frame = ((frame_data[0] >> 4) & 0x01) != 0;
    const bool frame_type = ((frame_data[0] >> 5) & 0x01) != 0;
    return marker == 0x02 && !show_existing_frame && !frame_type;
}
