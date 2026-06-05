#include "usm_ivf.h"

#include <string.h>

static uint16_t read_le16(const uint8_t *data)
{
    return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8));
}

static uint32_t read_le32(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}

static uint64_t read_le64(const uint8_t *data)
{
    return (uint64_t)read_le32(data) | ((uint64_t)read_le32(&data[4]) << 32);
}

bool usm_ivf_parse_header(const uint8_t *payload, size_t payload_size,
                          usm_ivf_header_t *header)
{
    if (payload_size < 0x20 || memcmp(payload, "DKIF", 4) != 0 || header == NULL) {
        return false;
    }
    if (read_le16(&payload[6]) != 0x20 || memcmp(&payload[8], "VP90", 4) != 0) {
        return false;
    }

    header->width = read_le16(&payload[12]);
    header->height = read_le16(&payload[14]);
    header->frame_rate_num = read_le32(&payload[16]);
    header->frame_rate_den = read_le32(&payload[20]);
    header->frame_count = read_le32(&payload[24]);
    if (header->frame_rate_num == 0 || header->frame_rate_den == 0) {
        return false;
    }
    return header->width != 0 && header->height != 0;
}

bool usm_ivf_parse_frame(const uint8_t *payload, size_t payload_size,
                         const uint8_t **frame_data, size_t *frame_size,
                         uint64_t *timestamp)
{
    if (payload_size < 12 || frame_data == NULL || frame_size == NULL ||
        timestamp == NULL) {
        return false;
    }

    const uint32_t size = read_le32(payload);
    if ((uint64_t)size > payload_size - 12) {
        return false;
    }

    *frame_data = &payload[12];
    *frame_size = size;
    *timestamp = read_le64(&payload[4]);
    return true;
}

bool usm_ivf_parse_payload_frame(const uint8_t *payload, size_t payload_size,
                                 const uint8_t **frame_data, size_t *frame_size,
                                 uint64_t *timestamp)
{
    usm_ivf_header_t header;
    if (usm_ivf_parse_header(payload, payload_size, &header)) {
        if (payload_size <= 0x20) {
            return false;
        }
        return usm_ivf_parse_frame(&payload[0x20], payload_size - 0x20,
                                   frame_data, frame_size, timestamp);
    }
    return usm_ivf_parse_frame(payload, payload_size, frame_data, frame_size,
                               timestamp);
}
