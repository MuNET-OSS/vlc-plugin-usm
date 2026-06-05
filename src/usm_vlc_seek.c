#include "usm_vlc_demux.h"

#include <vlc_es_out.h>
#include <vlc_stream.h>

typedef struct usm_seek_target {
    uint64_t offset;
    vlc_tick_t time;
    bool found;
} usm_seek_target_t;

static vlc_tick_t frame_time_to_tick(const demux_sys_t *sys, uint64_t timestamp)
{
    return VLC_TICK_FROM_SEC((int64_t)timestamp * sys->ivf.frame_rate_den) /
           sys->ivf.frame_rate_num;
}

static bool payload_keyframe_time(demux_t *demux, block_t *payload,
                                  vlc_tick_t *time)
{
    demux_sys_t *sys = demux->p_sys;
    const uint8_t *frame_data = NULL;
    size_t frame_size = 0;
    uint64_t timestamp = 0;
    if (!usm_ivf_parse_payload_frame(payload->p_buffer, payload->i_buffer,
                                     &frame_data, &frame_size, &timestamp)) {
        return false;
    }
    if (!usm_vp9_is_keyframe(frame_data, frame_size)) {
        return false;
    }

    *time = frame_time_to_tick(sys, timestamp);
    return true;
}

static bool find_seek_target(demux_t *demux, vlc_tick_t target_time,
                             usm_seek_target_t *target)
{
    demux_sys_t *sys = demux->p_sys;
    if (vlc_stream_Seek(demux->s, sys->first_payload_offset) != VLC_SUCCESS) {
        return false;
    }

    target->offset = sys->first_payload_offset;
    target->time = 0;
    target->found = false;

    while (true) {
        const uint64_t chunk_offset = vlc_stream_Tell(demux->s);
        usm_chunk_header_t chunk;
        block_t *payload = NULL;
        if (!usm_vlc_read_next_payload(demux, &chunk, &payload)) {
            break;
        }

        if (chunk.kind == USM_CHUNK_VIDEO &&
            chunk.payload_type == USM_STREAM_PAYLOAD && payload->i_buffer > 0) {
            vlc_tick_t time = 0;
            if (payload_keyframe_time(demux, payload, &time)) {
                if (time > target_time && target->found) {
                    block_Release(payload);
                    break;
                }
                target->offset = chunk_offset;
                target->time = time;
                target->found = true;
                if (time >= target_time) {
                    block_Release(payload);
                    break;
                }
            }
        }
        block_Release(payload);
    }

    return target->found;
}

int usm_vlc_seek(demux_t *demux, vlc_tick_t target_time)
{
    demux_sys_t *sys = demux->p_sys;
    if (target_time < 0) {
        target_time = 0;
    }
    if (sys->length > 0 && target_time > sys->length) {
        target_time = sys->length;
    }

    usm_seek_target_t target;
    if (!find_seek_target(demux, target_time, &target)) {
        return VLC_EGENERIC;
    }
    if (vlc_stream_Seek(demux->s, target.offset) != VLC_SUCCESS) {
        return VLC_EGENERIC;
    }

    if (sys->pending_payload != NULL) {
        block_Release(sys->pending_payload);
        sys->pending_payload = NULL;
    }
    sys->current_time = target.time;
    sys->frames_sent = 0;
    es_out_Control(demux->out, ES_OUT_RESET_PCR);
    return VLC_SUCCESS;
}
