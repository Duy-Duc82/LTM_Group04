#include "utils/json.h"
#include <stdlib.h>
#include <string.h>


char *util_json_escape(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    size_t out_cap = len * 2 + 1;
    char *out = malloc(out_cap);
    if (!out) return NULL;
    size_t o = 0;
    for (size_t i = 0; i < len; ++i) {
        unsigned char c = (unsigned char)s[i];
        switch (c) {
            case '"': case '\\': out[o++] = '\\'; out[o++] = c; break;
            case '\n': out[o++] = '\\'; out[o++] = 'n'; break;
            case '\r': out[o++] = '\\'; out[o++] = 'r'; break;
            case '\t': out[o++] = '\\'; out[o++] = 't'; break;
            default:
                if (c < 0x20) {
                    const char *hex = "0123456789abcdef";
                    if (o + 6 >= out_cap) {
                        // expand
                        out_cap *= 2;
                        char *tmp = realloc(out, out_cap);
                        if (!tmp) { free(out); return NULL; }
                        out = tmp;
                    }
                    out[o++] = '\\'; out[o++] = 'u'; out[o++] = '0'; out[o++] = '0';
                    out[o++] = hex[(c >> 4) & 0xF]; out[o++] = hex[c & 0xF];
                } else {
                    out[o++] = c;
                }
        }
        if (o + 1 >= out_cap) {
            out_cap *= 2;
            char *tmp = realloc(out, out_cap);
            if (!tmp) { free(out); return NULL; }
            out = tmp;
        }
    }
    out[o] = '\0';
    return out;
}

char *util_json_get_string(const char *json, const char *key) {
    if (!json || !key) return NULL;
    // find "key"
    const char *p = strstr(json, key);
    if (!p) return NULL;
    // find colon after key
    p = strchr(p, ':');
    if (!p) return NULL;
    // find starting quote
    p = strchr(p, '"');
    if (!p) return NULL;
    ++p;
    const char *q = p;
    while (*q && *q != '"') {
        if (*q == '\\' && *(q+1)) q += 2; else q++;
    }
    if (!*q) return NULL;
    size_t len = q - p;
    char *out = malloc(len + 1);
    if (!out) return NULL;
    strncpy(out, p, len);
    out[len] = '\0';
    return out;
}

int util_json_get_int64(const char *json, const char *key, long long *out) {
    if (!json || !key || !out) return 0;
    const char *p = strstr(json, key);
    if (!p) return 0;
    p = strchr(p, ':');
    if (!p) return 0;
    ++p;
    // skip spaces
    while (*p == ' ' || *p == '\t') ++p;
    char *end;
    long long v = strtoll(p, &end, 10);
    if (p == end) return 0;
    *out = v;
    return 1;
}

int util_json_get_double(const char *json, const char *key, double *out) {
    if (!json || !key || !out) return 0;
    const char *p = strstr(json, key);
    if (!p) return 0;
    p = strchr(p, ':');
    if (!p) return 0;
    ++p;
    // skip spaces
    while (*p == ' ' || *p == '\t') ++p;
    char *end;
    double v = strtod(p, &end);
    if (p == end) return 0;
    *out = v;
    return 1;
}

int util_json_parse_user_id_array(const char *json, int64_t *out_ids, int max_count) {
    if (!json || !out_ids || max_count <= 0) return 0;
    
    int count = 0;
    const char *p = json;
    
    // Find opening bracket
    p = strchr(p, '[');
    if (!p) return 0;
    p++;
    
    // Check if it's an array of objects (has "user_id" field) or simple numbers
    const char *check_p = p;
    int is_object_array = (strstr(check_p, "\"user_id\"") != NULL);
    
    if (is_object_array) {
        // Parse array of objects: [{"user_id": 13, ...}, {"user_id": 14, ...}]
        while (*p && count < max_count) {
            // Skip whitespace and commas
            while (*p == ' ' || *p == '\t' || *p == '\n' || *p == ',') p++;
            if (*p == ']') break;
            
            // Find start of object
            const char *obj_start = strchr(p, '{');
            if (!obj_start) break;
            
            // Find end of object
            const char *obj_end = strchr(obj_start + 1, '}');
            if (!obj_end) break;
            
            // Find "user_id" field in this object (must be between obj_start and obj_end)
            const char *user_id_str = strstr(obj_start, "\"user_id\"");
            if (user_id_str && user_id_str < obj_end) {
                // Find colon after "user_id"
                user_id_str = strchr(user_id_str, ':');
                if (user_id_str && user_id_str < obj_end) {
                    user_id_str++; // Skip colon
                    // Skip whitespace
                    while (*user_id_str == ' ' || *user_id_str == '\t') user_id_str++;
                    // Parse number
                    char *end;
                    long long v = strtoll(user_id_str, &end, 10);
                    if (end != user_id_str && end <= obj_end) {
                        out_ids[count++] = (int64_t)v;
                    }
                }
            }
            
            // Move to after this object
            p = obj_end + 1;
        }
    } else {
        // Parse simple array of numbers: [1, 2, 3]
        while (*p && count < max_count) {
            // Skip whitespace
            while (*p == ' ' || *p == '\t' || *p == '\n') p++;
            if (*p == ']') break;
            
            // Parse number
            char *end;
            long long v = strtoll(p, &end, 10);
            if (end == p) break; // Not a number
            
            out_ids[count++] = (int64_t)v;
            p = end;
            
            // Skip comma or whitespace
            while (*p == ' ' || *p == '\t' || *p == ',') p++;
        }
    }
    
    return count;
}