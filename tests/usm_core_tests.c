#include "usm_chunk.h"
#include "usm_crypto.h"
#include "usm_ivf.h"
#include "usm_keys.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int expect_true(const char *name, bool value)
{
    if (!value) {
        fprintf(stderr, "failed: %s\n", name);
        return 1;
    }
    return 0;
}

static int test_key_parser_accepts_config_lists(void)
{
    usm_key_list_t keys = {0};
    int failed = 0;

    failed += expect_true("parse key text",
                          usm_parse_key_text("0x7F4551499DF55E68, 1234\n# comment\n",
                                             &keys));
    failed += expect_true("key count", keys.count == 2);
    failed += expect_true("hex key", keys.values[0] == 0x7F4551499DF55E68ULL);
    failed += expect_true("decimal key", keys.values[1] == 1234);
    return failed;
}

static int test_usm_header_parses_video_payload(void)
{
    const uint8_t raw[USM_CHUNK_HEADER_SIZE] = {
        '@', 'S', 'F', 'V', 0x00, 0x00, 0x00, 0xf8,
        0x00, 0x18, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0b, 0xb8,
    };
    usm_chunk_header_t chunk;
    int failed = 0;

    failed += expect_true("parse chunk", usm_parse_chunk_header(raw, &chunk));
    failed += expect_true("video kind", chunk.kind == USM_CHUNK_VIDEO);
    failed += expect_true("stream payload", chunk.payload_type == 0);
    failed += expect_true("payload offset", chunk.payload_offset == 0x18);
    failed += expect_true("padding", chunk.padding_size == 0x1f);
    failed += expect_true("frame rate", chunk.frame_rate == 3000);
    return failed;
}

static int test_ivf_header_and_frame_parse(void)
{
    const uint8_t header[0x20] = {
        'D', 'K', 'I', 'F', 0x00, 0x00, 0x20, 0x00,
        'V', 'P', '9', '0', 0x38, 0x04, 0x38, 0x04,
        0xb8, 0x0b, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00,
        0x10, 0x00, 0x00, 0x00,
    };
    const uint8_t frame[16] = {
        0x04, 0x00, 0x00, 0x00, 0x2a, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 1, 2, 3, 4,
    };
    usm_ivf_header_t parsed;
    const uint8_t *frame_data = NULL;
    size_t frame_size = 0;
    uint64_t timestamp = 0;
    int failed = 0;

    failed += expect_true("parse ivf header",
                          usm_ivf_parse_header(header, sizeof(header), &parsed));
    failed += expect_true("width", parsed.width == 1080);
    failed += expect_true("height", parsed.height == 1080);
    failed += expect_true("rate num", parsed.frame_rate_num == 3000);
    failed += expect_true("rate den", parsed.frame_rate_den == 100);
    failed += expect_true("parse frame",
                          usm_ivf_parse_frame(frame, sizeof(frame), &frame_data,
                                              &frame_size, &timestamp));
    failed += expect_true("frame size", frame_size == 4);
    failed += expect_true("timestamp", timestamp == 42);
    failed += expect_true("frame payload", memcmp(frame_data, "\x01\x02\x03\x04", 4) == 0);
    return failed;
}

static int test_ivf_payload_frame_skips_file_header(void)
{
    uint8_t payload[0x30] = {
        'D', 'K', 'I', 'F', 0x00, 0x00, 0x20, 0x00,
        'V', 'P', '9', '0', 0x38, 0x04, 0x38, 0x04,
        0xb8, 0x0b, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00,
        0x10, 0x00, 0x00, 0x00,
    };
    const uint8_t frame[16] = {
        0x04, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 5, 6, 7, 8,
    };
    memcpy(&payload[0x20], frame, sizeof(frame));

    const uint8_t *frame_data = NULL;
    size_t frame_size = 0;
    uint64_t timestamp = 0;
    int failed = 0;

    failed += expect_true("parse payload frame",
                          usm_ivf_parse_payload_frame(payload, sizeof(payload),
                                                      &frame_data, &frame_size,
                                                      &timestamp));
    failed += expect_true("payload frame size", frame_size == 4);
    failed += expect_true("payload timestamp", timestamp == 9);
    failed += expect_true("payload frame bytes",
                          memcmp(frame_data, "\x05\x06\x07\x08", 4) == 0);
    return failed;
}

static int test_known_key_generates_reference_prefix(void)
{
    usm_decryption_keys_t keys;
    usm_generate_keys(0x7F4551499DF55E68ULL, &keys);
    const uint8_t expected_video_prefix[8] = {
        0x68, 0x5e, 0xf5, 0x69, 0x42, 0x42, 0xa6, 0x97,
    };
    int failed = 0;

    failed += expect_true("video key prefix",
                          memcmp(keys.video, expected_video_prefix,
                                 sizeof(expected_video_prefix)) == 0);
    failed += expect_true("audio odd bytes",
                          keys.audio[1] == 'U' && keys.audio[3] == 'R' &&
                              keys.audio[5] == 'U' && keys.audio[7] == 'C');
    return failed;
}

int main(void)
{
    int failed = 0;
    failed += test_key_parser_accepts_config_lists();
    failed += test_usm_header_parses_video_payload();
    failed += test_ivf_header_and_frame_parse();
    failed += test_ivf_payload_frame_skips_file_header();
    failed += test_known_key_generates_reference_prefix();
    return failed == 0 ? 0 : 1;
}
