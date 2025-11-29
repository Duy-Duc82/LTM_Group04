CC = gcc
CFLAGS = -Wall -Wextra -g -I./include -I/usr/include/postgresql
LDFLAGS = -lpq

BUILD_DIR = build

SRC_MAIN = src/main.c
SRC_DB = src/db.c

SRC_DAO = \
  src/dao/dao_users.c \
  src/dao/dao_sessions.c \
  src/dao/dao_question.c \
  src/dao/dao_quickmode.c

SRC_SERVICE = \
  src/service/auth_service.c \
  src/service/quickmode_service.c

# COMMON OBJECTS (dùng cho tất cả)
OBJS_COMMON = $(SRC_DB:.c=.o) $(SRC_DAO:.c=.o) $(SRC_SERVICE:.c=.o)

# TEST FILES
TEST_DB = src/test/test_db.c
TEST_AUTH = src/test/test_auth.c
TEST_QM = src/test/test_quickmode.c

.PHONY: all clean tests server

all: server tests

server: $(BUILD_DIR)/server

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/server: $(BUILD_DIR) $(OBJS_COMMON) $(SRC_MAIN)
	$(CC) $(CFLAGS) $(OBJS_COMMON) $(SRC_MAIN) -o $@ $(LDFLAGS)

tests: $(BUILD_DIR)/test_db $(BUILD_DIR)/test_auth $(BUILD_DIR)/test_quickmode

$(BUILD_DIR)/test_db: $(BUILD_DIR) $(OBJS_COMMON) $(TEST_DB)
	$(CC) $(CFLAGS) $(OBJS_COMMON) $(TEST_DB) -o $@ $(LDFLAGS)

$(BUILD_DIR)/test_auth: $(BUILD_DIR) $(OBJS_COMMON) $(TEST_AUTH)
	$(CC) $(CFLAGS) $(OBJS_COMMON) $(TEST_AUTH) -o $@ $(LDFLAGS)

$(BUILD_DIR)/test_quickmode: $(BUILD_DIR) $(OBJS_COMMON) $(TEST_QM)
	$(CC) $(CFLAGS) $(OBJS_COMMON) $(TEST_QM) -o $@ $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR) src/**/*.o
