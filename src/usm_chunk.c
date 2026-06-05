#include "usm_chunk.h"

#include <string.h>

static uint16_t read_be16(const uint8_t *data)
{
    return (uint16_t)(((uint16_t)data[0] << 8) | data[1]);
}

static uint32_t read_be32(const uint8_t *data)
{
    return ((uint32_t)data[0] << 24) |
           ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] << 8) |
           (uint32_t)data[3];
}

static usm_chunk_kind_t chunk_kind(const uint8_t *signature)
{
    if (memcmp(signature, "CRID", 4) == 0) {
        return USM_CHUNK_CRID;
    }
    if (memcmp(signature, "@SFV", 4) == 0) {
        return USM_CHUNK_VIDEO;
    }
    if (memcmp(signature, "@SFA", 4) == 0) {
        return USM_CHUNK_AUDIO;
    }
    return USM_CHUNK_UNKNOWN;
}

bool usm_parse_chunk_header(const uint8_t header[USM_CHUNK_HEADER_SIZE],
                            usm_chunk_header_t *chunk)
{
    if (chunk == NULL) {
        return false;
    }

    const uint32_t chunk_size = read_be32(&header[4]);
    const uint8_t payload_offset = header[9];
    const uint16_t padding_size = read_be16(&header[10]);

    if (payload_offset < 0x18 || chunk_size < payload_offset) {
        return false;
    }
    if ((uint32_t)padding_size > chunk_size - payload_offset) {
        return false;
    }

    chunk->kind = chunk_kind(header);
    chunk->chunk_size = chunk_size;
    chunk->payload_offset = payload_offset;
    chunk->padding_size = padding_size;
    chunk->channel = header[12];
    chunk->payload_type = header[15] & 0x03;
    chunk->frame_time = read_be32(&header[16]);
    chunk->frame_rate = read_be32(&header[20]);
    return true;
}

bool usm_read_exact(FILE *file, uint8_t *buffer, size_t size)
{
    return size == 0 || fread(buffer, 1, size, file) == size;
}

bool usm_skip_exact(FILE *file, size_t size)
{
    if (size == 0) {
        return true;
    }
    return fseek(file, (long)size, SEEK_CUR) == 0;
}
