#!/bin/bash
echo "Compiling test_dao..."

gcc -std=c11 -Wall -Wextra \
    ../src/db.c \
    ../src/dao/dao_users.c \
    ../src/dao/dao_sessions.c \
    test_dao.c \
    -I../include \
    -I../include/dao \
    -I../src \
    -lpq \
    -o test_dao

echo "Running..."
./test_dao
