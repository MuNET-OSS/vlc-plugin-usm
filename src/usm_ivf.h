#ifndef VLC_USM_IVF_H
#define VLC_USM_IVF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct usm_ivf_header {
    uint32_t width;
    uint32_t height;
    uint32_t frame_rate_num;
    uint32_t frame_rate_den;
    uint32_t frame_count;
} usm_ivf_header_t;

bool usm_ivf_parse_header(const uint8_t *payload, size_t payload_size,
                          usm_ivf_header_t *header);
bool usm_ivf_parse_frame(const uint8_t *payload, size_t payload_size,
                         const uint8_t **frame_data, size_t *frame_size,
                         uint64_t *timestamp);
bool usm_ivf_parse_payload_frame(const uint8_t *payload, size_t payload_size,
                                 const uint8_t **frame_data, size_t *frame_size,
                                 uint64_t *timestamp);

#endif
