#include "usm_keys.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

bool usm_parse_key_file(const char *path, usm_key_list_t *keys)
{
    if (path == NULL || path[0] == '\0') {
        return true;
    }

    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return false;
    }

    bool ok = true;
    char line[512];
    while (ok && fgets(line, sizeof(line), file) != NULL) {
        ok = usm_parse_key_text(line, keys);
    }

    if (ferror(file) != 0) {
        ok = false;
    }
    fclose(file);
    return ok;
}
