// Simple password hashing helpers using SHA256 + random salt
#ifndef UTIL_CRYPTO_H
#define UTIL_CRYPTO_H

#include <stddef.h>

// Hash password and write into out (NUL-terminated). Format: <salt_hex>$<sha256_hex>
// out must be large enough (recommended >= 128).
// Returns 0 on success, -1 on failure.
int util_password_hash(const char *password, char *out, size_t out_len);

// Verify plaintext password against stored value (salt$hash). Returns 1 if match, 0 if not, -1 on error.
int util_password_verify(const char *password, const char *stored);

#endif
