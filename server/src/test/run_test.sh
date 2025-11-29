#!/usr/bin/env bash
set -e

export DB_CONN="host=localhost port=5432 dbname=ltm_group04 user=postgres password=1"

make tests

echo -e "\n=== RUN test_db ==="
./build/test_db

echo -e "\n=== RUN test_auth ==="
./build/test_auth

echo -e "\n=== RUN test_quickmode ==="
./build/test_quickmode
