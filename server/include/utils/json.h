#ifndef UTIL_JSON_H
#define UTIL_JSON_H

// Escape a C string for JSON and return a newly allocated string.
// Caller must free the returned pointer.
char *util_json_escape(const char *s);

// Very small helper: extract a JSON string field value by key ("key": "value")
// Returns newly allocated string with the value or NULL if not found.
char *util_json_get_string(const char *json, const char *key);

// Get integer value for key (e.g. "friend_id": 123). Returns 1 on success and stores value in out, 0 otherwise.
int util_json_get_int64(const char *json, const char *key, long long *out);

#endif
