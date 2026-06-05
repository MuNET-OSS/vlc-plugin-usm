#ifndef VLC_USM_KEYS_H
#define VLC_USM_KEYS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    USM_MAX_KEYS = 128,
};

typedef struct usm_key_list {
    uint64_t values[USM_MAX_KEYS];
    size_t count;
} usm_key_list_t;

bool usm_key_list_add(usm_key_list_t *keys, uint64_t value);
char *usm_default_key_file_path(void);
char *usm_default_key_file_path_from_env(const char *xdg_config_home,
                                         const char *home);
bool usm_parse_key_text(const char *text, usm_key_list_t *keys);
bool usm_parse_key_file(const char *path, usm_key_list_t *keys);
bool usm_parse_optional_key_file(const char *path, usm_key_list_t *keys);

#endif
