#include "usm_vlc_demux.h"

#include <vlc_es_out.h>
#include <vpx/vp8dx.h>
#include <vpx/vpx_decoder.h>

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

static bool payload_needs_decrypt(size_t payload_size)
{
    return payload_size >= 0x240;
}

bool usm_vlc_init_key_probes(demux_t *demux)
{
    demux_sys_t *sys = demux->p_sys;
    for (size_t i = 0; i < sys->keys.count; i++) {
        if (vpx_codec_dec_init(&sys->key_probes[i], vpx_codec_vp9_dx(), NULL,
                               0) != VPX_CODEC_OK) {
            msg_Err(demux, "could not initialize VP9 key probe");
            usm_vlc_destroy_key_probes(demux);
            return false;
        }
        sys->key_probe_ready[i] = true;
    }
    return true;
}

void usm_vlc_destroy_key_probes(demux_t *demux)
{
    demux_sys_t *sys = demux->p_sys;
    if (sys == NULL) {
        return;
    }
    for (size_t i = 0; i < sys->keys.count; i++) {
        if (sys->key_probe_ready[i]) {
            vpx_codec_destroy(&sys->key_probes[i]);
            sys->key_probe_ready[i] = false;
        }
    }
}

static bool probe_decode_frame(vpx_codec_ctx_t *probe, const uint8_t *frame_data,
                               size_t frame_size)
{
    return vpx_codec_decode(probe, frame_data, (unsigned int)frame_size, NULL,
                            0) == VPX_CODEC_OK;
}

static bool decrypted_frame_decodes(demux_t *demux, size_t key_index,
                                    block_t *payload,
                                    const usm_decryption_keys_t *keys)
{
    // 用堆而非栈上 VLA：payload 大小来自文件，超大帧会爆栈
    uint8_t *trial = malloc(payload->i_buffer);
    if (trial == NULL) {
        return false;
    }
    memcpy(trial, payload->p_buffer, payload->i_buffer);
    usm_decrypt_video(trial, payload->i_buffer, keys);

    const uint8_t *frame_data = NULL;
    size_t frame_size = 0;
    uint64_t timestamp = 0;
    bool ok = false;
    if (usm_ivf_parse_payload_frame(trial, payload->i_buffer, &frame_data,
                                    &frame_size, &timestamp)) {
        demux_sys_t *sys = demux->p_sys;
        ok = probe_decode_frame(&sys->key_probes[key_index], frame_data,
                                frame_size);
    }
    free(trial);
    return ok;
}

static bool select_matching_key(demux_t *demux, block_t *payload)
{
    demux_sys_t *sys = demux->p_sys;
    if (sys->keys.count == 0) {
        sys->key_selected = true;
        sys->needs_video_decrypt = false;
        return true;
    }

    for (size_t i = 0; i < sys->keys.count; i++) {
        usm_decryption_keys_t candidate;
        usm_generate_keys(sys->keys.values[i], &candidate);
        if (sys->key_probe_ready[i] &&
            decrypted_frame_decodes(demux, i, payload, &candidate)) {
            sys->active_keys = candidate;
            sys->key_selected = true;
            sys->needs_video_decrypt = true;
            msg_Info(demux, "selected USM key 0x%016" PRIx64, sys->keys.values[i]);
            usm_vlc_destroy_key_probes(demux);
            return true;
        }
    }

    msg_Err(demux, "no configured USM key decoded the video stream");
    return false;
}

bool usm_vlc_init_video_es(demux_t *demux, block_t *payload)
{
    demux_sys_t *sys = demux->p_sys;
    if (!usm_ivf_parse_header(payload->p_buffer, payload->i_buffer, &sys->ivf)) {
        return false;
    }

    es_format_t format;
    es_format_Init(&format, VIDEO_ES, VLC_CODEC_VP9);
    format.video.i_width = sys->ivf.width;
    format.video.i_height = sys->ivf.height;
    format.video.i_visible_width = sys->ivf.width;
    format.video.i_visible_height = sys->ivf.height;
    format.video.i_frame_rate = sys->ivf.frame_rate_num;
    format.video.i_frame_rate_base = sys->ivf.frame_rate_den;
    format.b_packetized = true;

    sys->video_es = es_out_Add(demux->out, &format);
    es_format_Clean(&format);
    if (sys->video_es == NULL) {
        return false;
    }

    sys->frame_duration = VLC_TICK_FROM_SEC(sys->ivf.frame_rate_den) /
                          sys->ivf.frame_rate_num;
    if (sys->ivf.frame_count > 0) {
        sys->length = sys->frame_duration * sys->ivf.frame_count;
    }
    sys->have_video_format = true;
    return true;
}

static bool prepare_payload_for_video(demux_t *demux, block_t *payload,
                                      bool encrypted)
{
    demux_sys_t *sys = demux->p_sys;
    if (!encrypted) {
        return true;
    }
    if (!sys->key_selected && !select_matching_key(demux, payload)) {
        return false;
    }
    if (sys->needs_video_decrypt) {
        usm_decrypt_video(payload->p_buffer, payload->i_buffer, &sys->active_keys);
    }
    return true;
}

static void advance_key_probes(demux_t *demux, const uint8_t *frame_data,
                               size_t frame_size)
{
    demux_sys_t *sys = demux->p_sys;
    if (sys->key_selected) {
        return;
    }
    for (size_t i = 0; i < sys->keys.count; i++) {
        if (sys->key_probe_ready[i] &&
            !probe_decode_frame(&sys->key_probes[i], frame_data, frame_size)) {
            vpx_codec_destroy(&sys->key_probes[i]);
            sys->key_probe_ready[i] = false;
        }
    }
}

int usm_vlc_send_video_frame(demux_t *demux, block_t *payload)
{
    demux_sys_t *sys = demux->p_sys;
    const bool encrypted = payload_needs_decrypt(payload->i_buffer);
    if (!prepare_payload_for_video(demux, payload, encrypted)) {
        return VLC_DEMUXER_EGENERIC;
    }

    const uint8_t *frame_data = NULL;
    size_t frame_size = 0;
    uint64_t timestamp = 0;
    if (!usm_ivf_parse_payload_frame(payload->p_buffer, payload->i_buffer,
                                     &frame_data, &frame_size, &timestamp)) {
        return VLC_DEMUXER_SUCCESS;
    }
    if (!encrypted) {
        advance_key_probes(demux, frame_data, frame_size);
    }

    block_t *frame = block_Alloc(frame_size);
    if (frame == NULL) {
        return VLC_DEMUXER_EGENERIC;
    }

    memcpy(frame->p_buffer, frame_data, frame_size);
    frame->i_buffer = frame_size;
    const vlc_tick_t media_time =
        VLC_TICK_FROM_SEC((int64_t)timestamp * sys->ivf.frame_rate_den) /
        sys->ivf.frame_rate_num;
    frame->i_pts = VLC_TICK_0 + media_time;
    frame->i_dts = frame->i_pts;
    frame->i_length = sys->frame_duration;
    sys->current_time = media_time;
    sys->frames_sent++;

    es_out_SetPCR(demux->out, frame->i_dts);
    es_out_Send(demux->out, sys->video_es, frame);
    return VLC_DEMUXER_SUCCESS;
}
