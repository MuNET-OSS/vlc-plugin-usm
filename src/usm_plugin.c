#include "usm_vlc_compat.h"

#include <vlc_common.h>
#include <vlc_demux.h>
#include <vlc_plugin.h>
#include <vlc_stream.h>

#include "usm_vlc_demux.h"

#include <string.h>

#define USM_KEYS_TEXT N_("USM keys")
#define USM_KEYS_LONGTEXT N_("List of decimal or 0x-prefixed hexadecimal CRIWARE USM keys.")
#define USM_KEY_FILE_TEXT N_("USM key file")
#define USM_KEY_FILE_LONGTEXT N_("Text file containing decimal or 0x-prefixed hexadecimal CRIWARE USM keys.")

static int Open(vlc_object_t *object);
static void Close(vlc_object_t *object);
static int Demux(demux_t *demux);
static int Control(demux_t *demux, int query, va_list args);

vlc_module_begin()
    set_shortname("USM")
    set_category(CAT_INPUT)
    set_subcategory(SUBCAT_INPUT_DEMUX)
    set_description(N_("CRIWARE USM demuxer"))
    set_capability("demux", 80)
    add_shortcut("usm")
    add_string("usm-keys", "", USM_KEYS_TEXT, USM_KEYS_LONGTEXT, false)
    add_loadfile("usm-key-file", "", USM_KEY_FILE_TEXT, USM_KEY_FILE_LONGTEXT, false)
    set_callbacks(Open, Close)
vlc_module_end()

static int Open(vlc_object_t *object)
{
    demux_t *demux = (demux_t *)object;
    const uint8_t *peek = NULL;
    if (vlc_stream_Peek(demux->s, &peek, 4) < 4 || memcmp(peek, "CRID", 4) != 0) {
        return VLC_EGENERIC;
    }

    demux_sys_t *sys = calloc(1, sizeof(*sys));
    if (sys == NULL) {
        return VLC_ENOMEM;
    }
    demux->p_sys = sys;

    if (!usm_vlc_read_configured_keys(demux, &sys->keys)) {
        free(sys);
        return VLC_EGENERIC;
    }
    if (!usm_vlc_init_key_probes(demux)) {
        free(sys);
        return VLC_EGENERIC;
    }

    while (!sys->have_video_format) {
        const uint64_t chunk_offset = vlc_stream_Tell(demux->s);
        usm_chunk_header_t chunk;
        block_t *payload = NULL;
        if (!usm_vlc_read_next_payload(demux, &chunk, &payload)) {
            usm_vlc_destroy_key_probes(demux);
            free(sys);
            return VLC_EGENERIC;
        }

        if (chunk.kind == USM_CHUNK_VIDEO &&
            chunk.payload_type == USM_STREAM_PAYLOAD && payload->i_buffer > 0) {
            sys->first_payload_offset = chunk_offset;
            if (!usm_vlc_init_video_es(demux, payload)) {
                block_Release(payload);
                usm_vlc_destroy_key_probes(demux);
                free(sys);
                return VLC_EGENERIC;
            }
            sys->pending_payload = payload;
            payload = NULL;
        }
        if (payload != NULL) {
            block_Release(payload);
        }
    }

    demux->pf_demux = Demux;
    demux->pf_control = Control;
    return sys->video_es != NULL ? VLC_SUCCESS : VLC_EGENERIC;
}

static void Close(vlc_object_t *object)
{
    demux_t *demux = (demux_t *)object;
    demux_sys_t *sys = demux->p_sys;
    if (sys == NULL) {
        return;
    }
    if (sys->video_es != NULL) {
        es_out_Del(demux->out, sys->video_es);
    }
    if (sys->pending_payload != NULL) {
        block_Release(sys->pending_payload);
    }
    usm_vlc_destroy_key_probes(demux);
    free(sys);
}

static int Demux(demux_t *demux)
{
    demux_sys_t *sys = demux->p_sys;
    if (sys->pending_payload != NULL) {
        block_t *payload = sys->pending_payload;
        sys->pending_payload = NULL;
        const int result = usm_vlc_send_video_frame(demux, payload);
        block_Release(payload);
        return result;
    }

    while (true) {
        usm_chunk_header_t chunk;
        block_t *payload = NULL;
        if (!usm_vlc_read_next_payload(demux, &chunk, &payload)) {
            return VLC_DEMUXER_EOF;
        }

        if (chunk.kind == USM_CHUNK_VIDEO &&
            chunk.payload_type == USM_STREAM_PAYLOAD && payload->i_buffer > 0) {
            const int result = usm_vlc_send_video_frame(demux, payload);
            block_Release(payload);
            return result;
        }
        block_Release(payload);
    }
}

static int Control(demux_t *demux, int query, va_list args)
{
    demux_sys_t *sys = demux->p_sys;
    switch (query) {
    case DEMUX_CAN_SEEK:
    case DEMUX_CAN_PAUSE:
    case DEMUX_CAN_CONTROL_PACE:
        *va_arg(args, bool *) = true;
        return VLC_SUCCESS;
    case DEMUX_GET_TIME:
        *va_arg(args, int64_t *) = sys->current_time;
        return VLC_SUCCESS;
    case DEMUX_GET_LENGTH:
        *va_arg(args, int64_t *) = sys->length;
        return VLC_SUCCESS;
    case DEMUX_GET_POSITION:
        *va_arg(args, double *) = sys->length > 0
            ? (double)sys->current_time / (double)sys->length
            : 0.0;
        return VLC_SUCCESS;
    case DEMUX_SET_TIME: {
        const int64_t target_time = va_arg(args, int64_t);
        (void)va_arg(args, int);
        return usm_vlc_seek(demux, target_time);
    }
    case DEMUX_SET_POSITION: {
        double position = va_arg(args, double);
        (void)va_arg(args, int);
        if (position < 0.0) {
            position = 0.0;
        } else if (position > 1.0) {
            position = 1.0;
        }
        return usm_vlc_seek(demux, (vlc_tick_t)(position * (double)sys->length));
    }
    default:
        return demux_vaControlHelper(demux->s, 0, -1, 0, 1, query, args);
    }
}
