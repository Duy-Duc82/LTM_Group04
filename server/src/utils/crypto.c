#include "utils/crypto.h"
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <string.h>

// helper: convert bytes to hex
static void bytes_to_hex(const unsigned char *in, size_t len, char *out, size_t out_sz) {
    static const char hex[] = "0123456789abcdef";
    size_t i;
    if (out_sz < (len * 2 + 1)) {
        if (out_sz > 0) out[0] = '\0';
        return;
    }
    for (i = 0; i < len; ++i) {
        out[i * 2] = hex[(in[i] >> 4) & 0xF];
        out[i * 2 + 1] = hex[in[i] & 0xF];
    }
    out[len * 2] = '\0';
}

int util_password_hash(const char *password, char *out, size_t out_len) {
    if (!password || !out) return -1;
    // generate 16 bytes salt
    unsigned char salt[16];
    if (RAND_bytes(salt, sizeof(salt)) != 1) return -1;

    char salt_hex[33];
    bytes_to_hex(salt, sizeof(salt), salt_hex, sizeof(salt_hex));

    // compute sha256(salt + password)
    SHA256_CTX ctx;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, salt, sizeof(salt));
    SHA256_Update(&ctx, (const unsigned char *)password, strlen(password));
    SHA256_Final(hash, &ctx);

    char hash_hex[SHA256_DIGEST_LENGTH * 2 + 1];
    bytes_to_hex(hash, sizeof(hash), hash_hex, sizeof(hash_hex));

    // format salt$hash
    int needed = snprintf(NULL, 0, "%s$%s", salt_hex, hash_hex) + 1;
    if ((size_t)needed > out_len) return -1;
    snprintf(out, out_len, "%s$%s", salt_hex, hash_hex);
    return 0;
}

int util_password_verify(const char *password, const char *stored) {
    if (!password || !stored) return -1;
    // expected format salt$hash
    const char *sep = strchr(stored, '$');
    if (!sep) return -1;
    size_t salt_len = sep - stored;
    if (salt_len == 0 || salt_len > 64) return -1;

    char salt_hex[65];
    if (salt_len >= sizeof(salt_hex)) return -1;
    memcpy(salt_hex, stored, salt_len);
    salt_hex[salt_len] = '\0';

    // convert salt_hex back to bytes
    size_t salt_bytes_len = salt_len / 2;
    unsigned char salt[32];
    for (size_t i = 0; i < salt_bytes_len; ++i) {
        unsigned int v;
        // parse two hex chars (one byte)
        if (sscanf(salt_hex + i*2, "%2x", &v) != 1) return -1;
        salt[i] = (unsigned char)v;
    }

    // compute hash(salt + password)
    SHA256_CTX ctx;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, salt, salt_bytes_len);
    SHA256_Update(&ctx, (const unsigned char *)password, strlen(password));
    SHA256_Final(hash, &ctx);

    char computed_hex[SHA256_DIGEST_LENGTH * 2 + 1];
    bytes_to_hex(hash, sizeof(hash), computed_hex, sizeof(computed_hex));

    const char *stored_hash = sep + 1;
    if (strcmp(computed_hex, stored_hash) == 0) return 1;
    return 0;
}
