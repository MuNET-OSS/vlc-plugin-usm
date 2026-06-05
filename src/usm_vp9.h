#ifndef VLC_USM_VP9_H
#define VLC_USM_VP9_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool usm_vp9_is_keyframe(const uint8_t *frame_data, size_t frame_size);

#endif
