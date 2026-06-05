#include "usm_vlc_demux.h"

#include <vlc_block.h>
#include <vlc_stream.h>
#include <vlc_variables.h>

bool usm_vlc_read_configured_keys(demux_t *demux, usm_key_list_t *keys)
{
    char *inline_keys = var_InheritString(demux, "usm-keys");
    const bool inline_ok = usm_parse_key_text(inline_keys, keys);
    free(inline_keys);
    if (!inline_ok) {
        msg_Err(demux, "invalid --usm-keys value");
        return false;
    }

    char *key_file = var_InheritString(demux, "usm-key-file");
    const bool file_ok = usm_parse_key_file(key_file, keys);
    if (!file_ok) {
        msg_Err(demux, "could not read --usm-key-file '%s'", key_file);
    }
    free(key_file);
    return file_ok;
}

bool usm_vlc_read_next_payload(demux_t *demux, usm_chunk_header_t *chunk,
                               block_t **payload)
{
    uint8_t header[USM_CHUNK_HEADER_SIZE];
    const ssize_t read = vlc_stream_Read(demux->s, header, sizeof(header));
    if (read == 0) {
        return false;
    }
    if (read != (ssize_t)sizeof(header) || !usm_parse_chunk_header(header, chunk)) {
        msg_Err(demux, "invalid USM chunk header");
        return false;
    }

    const size_t extra_offset = chunk->payload_offset - 0x18;
    const size_t payload_size = chunk->chunk_size - chunk->payload_offset -
                                chunk->padding_size;
    if (extra_offset > 0 &&
        vlc_stream_Read(demux->s, NULL, extra_offset) != (ssize_t)extra_offset) {
        msg_Err(demux, "unexpected EOF before USM payload");
        return false;
    }

    block_t *block = block_Alloc(payload_size);
    if (block == NULL) {
        return false;
    }
    if (payload_size > 0 &&
        vlc_stream_Read(demux->s, block->p_buffer, payload_size) !=
            (ssize_t)payload_size) {
        block_Release(block);
        msg_Err(demux, "unexpected EOF in USM payload");
        return false;
    }
    block->i_buffer = payload_size;

    if (chunk->padding_size > 0 &&
        vlc_stream_Read(demux->s, NULL, chunk->padding_size) !=
            (ssize_t)chunk->padding_size) {
        block_Release(block);
        msg_Err(demux, "unexpected EOF in USM padding");
        return false;
    }

    *payload = block;
    return true;
}
