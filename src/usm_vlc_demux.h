#ifndef VLC_USM_VLC_DEMUX_H
#define VLC_USM_VLC_DEMUX_H

#include "usm_vlc_compat.h"

#include <vlc_common.h>
#include <vlc_demux.h>
#include <vlc_es.h>
#include <vlc_block.h>
#include <vpx/vpx_decoder.h>

#include "usm_chunk.h"
#include "usm_crypto.h"
#include "usm_ivf.h"
#include "usm_keys.h"
#include "usm_vp9.h"

typedef struct demux_sys_t {
    es_out_id_t *video_es;
    usm_key_list_t keys;
    usm_decryption_keys_t active_keys;
    bool key_selected;
    bool needs_video_decrypt;
    bool have_video_format;
    usm_ivf_header_t ivf;
    block_t *pending_payload;
    vpx_codec_ctx_t key_probes[USM_MAX_KEYS];
    bool key_probe_ready[USM_MAX_KEYS];
    uint64_t frames_sent;
    vlc_tick_t current_time;
    vlc_tick_t frame_duration;
    vlc_tick_t length;
    uint64_t first_payload_offset;
} demux_sys_t;

bool usm_vlc_read_configured_keys(demux_t *demux, usm_key_list_t *keys);
bool usm_vlc_read_next_payload(demux_t *demux, usm_chunk_header_t *chunk,
                               block_t **payload);
bool usm_vlc_init_key_probes(demux_t *demux);
void usm_vlc_destroy_key_probes(demux_t *demux);
bool usm_vlc_init_video_es(demux_t *demux, block_t *payload);
int usm_vlc_send_video_frame(demux_t *demux, block_t *payload);
int usm_vlc_seek(demux_t *demux, vlc_tick_t target_time);

#endif
