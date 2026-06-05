#ifndef VLC_USM_CHUNK_H
#define VLC_USM_CHUNK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

enum {
    USM_CHUNK_HEADER_SIZE = 0x20,
    USM_STREAM_PAYLOAD = 0,
};

typedef enum usm_chunk_kind {
    USM_CHUNK_UNKNOWN = 0,
    USM_CHUNK_CRID,
    USM_CHUNK_VIDEO,
    USM_CHUNK_AUDIO,
} usm_chunk_kind_t;

typedef struct usm_chunk_header {
    usm_chunk_kind_t kind;
    uint32_t chunk_size;
    uint8_t payload_offset;
    uint16_t padding_size;
    uint8_t channel;
    uint8_t payload_type;
    uint32_t frame_time;
    uint32_t frame_rate;
} usm_chunk_header_t;

bool usm_parse_chunk_header(const uint8_t header[USM_CHUNK_HEADER_SIZE],
                            usm_chunk_header_t *chunk);
bool usm_read_exact(FILE *file, uint8_t *buffer, size_t size);
bool usm_skip_exact(FILE *file, size_t size);

#endif
