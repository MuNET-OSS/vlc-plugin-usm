#ifndef VLC_USM_CRYPTO_H
#define VLC_USM_CRYPTO_H

#include <stddef.h>
#include <stdint.h>

typedef struct usm_decryption_keys {
    uint8_t video[0x40];
    uint8_t audio[0x20];
} usm_decryption_keys_t;

void usm_generate_keys(uint64_t key, usm_decryption_keys_t *keys);
void usm_decrypt_video(uint8_t *packet, size_t packet_size,
                       const usm_decryption_keys_t *keys);
void usm_decrypt_audio(uint8_t *packet, size_t packet_size,
                       const usm_decryption_keys_t *keys);

#endif
