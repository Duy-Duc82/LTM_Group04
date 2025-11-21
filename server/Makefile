CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Iinclude -Iinclude/dao

DAO_SRC = src/dao/dao_users.c src/dao/dao_sessions.c
CORE_SRC = src/db.c

TEST_SRC = test/test_dao.c

LIBS = -lpq

test:
	$(CC) $(CFLAGS) $(DAO_SRC) $(CORE_SRC) $(TEST_SRC) $(LIBS) -o test/test_dao
	./test/test_dao
