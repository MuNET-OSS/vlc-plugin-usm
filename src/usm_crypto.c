#include "usm_crypto.h"

#include <string.h>

void usm_generate_keys(uint64_t key_num, usm_decryption_keys_t *keys)
{
    const uint8_t cipher[8] = {
        (uint8_t)(key_num & 0xff),
        (uint8_t)((key_num >> 8) & 0xff),
        (uint8_t)((key_num >> 16) & 0xff),
        (uint8_t)((key_num >> 24) & 0xff),
        (uint8_t)((key_num >> 32) & 0xff),
        (uint8_t)((key_num >> 40) & 0xff),
        (uint8_t)((key_num >> 48) & 0xff),
        (uint8_t)((key_num >> 56) & 0xff),
    };
    uint8_t key[0x20];

    key[0x00] = cipher[0];
    key[0x01] = cipher[1];
    key[0x02] = cipher[2];
    key[0x03] = (uint8_t)(cipher[3] - 0x34);
    key[0x04] = (uint8_t)(cipher[4] + 0xf9);
    key[0x05] = (uint8_t)(cipher[5] ^ 0x13);
    key[0x06] = (uint8_t)(cipher[6] + 0x61);
    key[0x07] = (uint8_t)(key[0x00] ^ 0xff);
    key[0x08] = (uint8_t)(key[0x01] + key[0x02]);
    key[0x09] = (uint8_t)(key[0x01] - key[0x07]);
    key[0x0a] = (uint8_t)(key[0x02] ^ 0xff);
    key[0x0b] = (uint8_t)(key[0x01] ^ 0xff);
    key[0x0c] = (uint8_t)(key[0x0b] + key[0x09]);
    key[0x0d] = (uint8_t)(key[0x08] - key[0x03]);
    key[0x0e] = (uint8_t)(key[0x0d] ^ 0xff);
    key[0x0f] = (uint8_t)(key[0x0a] - key[0x0b]);
    key[0x10] = (uint8_t)(key[0x08] - key[0x0f]);
    key[0x11] = (uint8_t)(key[0x10] ^ key[0x07]);
    key[0x12] = (uint8_t)(key[0x0f] ^ 0xff);
    key[0x13] = (uint8_t)(key[0x03] ^ 0x10);
    key[0x14] = (uint8_t)(key[0x04] - 0x32);
    key[0x15] = (uint8_t)(key[0x05] + 0xed);
    key[0x16] = (uint8_t)(key[0x06] ^ 0xf3);
    key[0x17] = (uint8_t)(key[0x13] - key[0x0f]);
    key[0x18] = (uint8_t)(key[0x15] + key[0x07]);
    key[0x19] = (uint8_t)(0x21 - key[0x13]);
    key[0x1a] = (uint8_t)(key[0x14] ^ key[0x17]);
    key[0x1b] = (uint8_t)(key[0x16] + key[0x16]);
    key[0x1c] = (uint8_t)(key[0x17] + 0x44);
    key[0x1d] = (uint8_t)(key[0x03] + key[0x04]);
    key[0x1e] = (uint8_t)(key[0x05] - key[0x16]);
    key[0x1f] = (uint8_t)(key[0x1d] ^ key[0x13]);

    static const uint8_t audio_template[4] = {'U', 'R', 'U', 'C'};
    for (size_t i = 0; i < 0x20; i++) {
        keys->video[i] = key[i];
        keys->video[0x20 + i] = (uint8_t)(key[i] ^ 0xff);
        keys->audio[i] = (i % 2) != 0
            ? audio_template[(i >> 1) % 4]
            : (uint8_t)(key[i] ^ 0xff);
    }
}

void usm_decrypt_video(uint8_t *packet, size_t packet_size,
                       const usm_decryption_keys_t *keys)
{
    if (packet_size < 0x240) {
        return;
    }

    const size_t encrypted_size = packet_size - 0x40;
    uint8_t rolling[0x40];
    memcpy(rolling, keys->video, sizeof(rolling));

    for (size_t i = 0x100; i < encrypted_size; i++) {
        const size_t packet_index = 0x40 + i;
        const size_t key_index = 0x20 + (i % 0x20);
        packet[packet_index] = (uint8_t)(packet[packet_index] ^ rolling[key_index]);
        rolling[key_index] = (uint8_t)(packet[packet_index] ^ keys->video[key_index]);
    }

    for (size_t i = 0; i < 0x100; i++) {
        const size_t key_index = i % 0x20;
        rolling[key_index] = (uint8_t)(rolling[key_index] ^ packet[0x140 + i]);
        packet[0x40 + i] = (uint8_t)(packet[0x40 + i] ^ rolling[key_index]);
    }
}

void usm_decrypt_audio(uint8_t *packet, size_t packet_size,
                       const usm_decryption_keys_t *keys)
{
    if (packet_size <= 0x140) {
        return;
    }

    for (size_t i = 0x140; i < packet_size; i++) {
        packet[i] = (uint8_t)(packet[i] ^ keys->audio[i % 0x20]);
    }
}
