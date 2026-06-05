#include "usm_keys.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *join_key_file_path(const char *base, const char *suffix)
{
    const size_t base_len = strlen(base);
    const size_t suffix_len = strlen(suffix);
    char *path = malloc(base_len + suffix_len + 1);
    if (path == NULL) {
        return NULL;
    }
    memcpy(path, base, base_len);
    memcpy(path + base_len, suffix, suffix_len + 1);
    return path;
}

bool usm_key_list_add(usm_key_list_t *keys, uint64_t value)
{
    if (keys == NULL || keys->count >= USM_MAX_KEYS) {
        return false;
    }

    for (size_t i = 0; i < keys->count; i++) {
        if (keys->values[i] == value) {
            return true;
        }
    }

    keys->values[keys->count] = value;
    keys->count++;
    return true;
}

static bool is_separator(char c)
{
    return c == ',' || c == ';' || isspace((unsigned char)c) || c == '#';
}

bool usm_parse_key_text(const char *text, usm_key_list_t *keys)
{
    if (text == NULL || keys == NULL) {
        return true;
    }

    const char *cursor = text;
    while (*cursor != '\0') {
        while (*cursor == ',' || *cursor == ';' || isspace((unsigned char)*cursor)) {
            cursor++;
        }
        if (*cursor == '#') {
            while (*cursor != '\0' && *cursor != '\n') {
                cursor++;
            }
            continue;
        }
        if (*cursor == '\0') {
            return true;
        }

        errno = 0;
        char *end = NULL;
        const unsigned long long value = strtoull(cursor, &end, 0);
        if (end == cursor || errno == ERANGE || (*end != '\0' && !is_separator(*end))) {
            return false;
        }
        if (!usm_key_list_add(keys, (uint64_t)value)) {
            return false;
        }
        cursor = end;
    }
    return true;
}

char *usm_default_key_file_path_from_env(const char *xdg_config_home,
                                         const char *home)
{
    if (xdg_config_home != NULL && xdg_config_home[0] != '\0') {
        return join_key_file_path(xdg_config_home, "/vlc/usm-keys.txt");
    }
    if (home != NULL && home[0] != '\0') {
        return join_key_file_path(home, "/.config/vlc/usm-keys.txt");
    }
    return NULL;
}

char *usm_default_key_file_path(void)
{
    return usm_default_key_file_path_from_env(getenv("XDG_CONFIG_HOME"), getenv("HOME"));
}

static bool parse_key_stream(FILE *file, usm_key_list_t *keys)
{
    bool ok = true;
    char line[512];
    while (ok && fgets(line, sizeof(line), file) != NULL) {
        ok = usm_parse_key_text(line, keys);
    }
    if (ferror(file) != 0) {
        ok = false;
    }
    return ok;
}

static bool parse_key_file_at_path(const char *path, usm_key_list_t *keys,
                                   bool missing_is_ok)
{
    if (path == NULL || path[0] == '\0') {
        return true;
    }

    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return missing_is_ok && errno == ENOENT;
    }

    const bool ok = parse_key_stream(file, keys);
    fclose(file);
    return ok;
}

bool usm_parse_key_file(const char *path, usm_key_list_t *keys)
{
    return parse_key_file_at_path(path, keys, false);
}

bool usm_parse_optional_key_file(const char *path, usm_key_list_t *keys)
{
    return parse_key_file_at_path(path, keys, true);
}
