// Helper program to generate password hash for SQL insert
// Compile: make build/test_hash_password
// Usage: ./build/test_hash_password <password>
// Example: ./build/test_hash_password alice123

#include <stdio.h>
#include <string.h>
#include "../include/utils/crypto.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <password>\n", argv[0]);
        return 1;
    }

    char hashed[128];
    if (util_password_hash(argv[1], hashed, sizeof(hashed)) != 0) {
        fprintf(stderr, "Failed to hash password\n");
        return 1;
    }

    printf("%s\n", hashed);
    return 0;
}

