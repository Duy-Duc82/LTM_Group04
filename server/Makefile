CC      = gcc
CFLAGS  = -Wall -Wextra -g -I./include -I/usr/include/postgresql
LDFLAGS = -lpq -lcrypto

BUILD_DIR = build

# ==== OBJECTS CHUNG (KHÔNG GỒM main, KHÔNG GỒM test) ====

DAO_OBJS = \
    src/dao/dao_chat.o \
    src/dao/dao_friends.o \
    src/dao/dao_question.o \
    src/dao/dao_quickmode.o \
    src/dao/dao_rooms.o \
    src/dao/dao_sessions.o \
    src/dao/dao_stats.o \
    src/dao/dao_users.o

SERVICE_OBJS = \
    src/service/auth_service.o \
    src/service/client_session.o \
    src/service/dispatcher.o \
    src/service/protocol.o \
    src/service/quickmode_service.o \
    src/service/server.o \
    src/service/stats_service.o

UTIL_OBJS = \
    src/utils/crypto.o \
    src/utils/json.o

DB_OBJS = src/db.o

COMMON_OBJS = $(DAO_OBJS) $(SERVICE_OBJS) $(UTIL_OBJS) $(DB_OBJS)

MAIN_OBJ = src/main.o

# ==== TEST OBJECTS ====

TEST_AUTH_OBJ      = src/test/test_auth.o
TEST_DB_OBJ        = src/test/test_db.o
TEST_CHAT_OBJ      = src/test/test_chat.o
TEST_FRIENDS_OBJ   = src/test/test_friends.o
TEST_QUICKMODE_OBJ = src/test/test_quickmode.o
TEST_ROOMS_OBJ     = src/test/test_rooms.o

# ==== TARGET MẶC ĐỊNH ====

.PHONY: all
all: $(BUILD_DIR)/server \
     $(BUILD_DIR)/test_auth \
     $(BUILD_DIR)/test_db \
     $(BUILD_DIR)/test_chat \
     $(BUILD_DIR)/test_friends \
     $(BUILD_DIR)/test_quickmode \
     $(BUILD_DIR)/test_rooms

# ==== SERVER ====

$(BUILD_DIR)/server: $(COMMON_OBJS) $(MAIN_OBJ)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(COMMON_OBJS) $(MAIN_OBJ) -o $@ $(LDFLAGS)

# ==== TEST BINARIES ====

# Mỗi test link file test_xxx + COMMON_OBJS
$(BUILD_DIR)/test_auth: $(COMMON_OBJS) $(TEST_AUTH_OBJ)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(COMMON_OBJS) $(TEST_AUTH_OBJ) -o $@ $(LDFLAGS)

$(BUILD_DIR)/test_db: $(COMMON_OBJS) $(TEST_DB_OBJ)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(COMMON_OBJS) $(TEST_DB_OBJ) -o $@ $(LDFLAGS)

$(BUILD_DIR)/test_chat: $(COMMON_OBJS) $(TEST_CHAT_OBJ)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(COMMON_OBJS) $(TEST_CHAT_OBJ) -o $@ $(LDFLAGS)

$(BUILD_DIR)/test_friends: $(COMMON_OBJS) $(TEST_FRIENDS_OBJ)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(COMMON_OBJS) $(TEST_FRIENDS_OBJ) -o $@ $(LDFLAGS)

$(BUILD_DIR)/test_quickmode: $(COMMON_OBJS) $(TEST_QUICKMODE_OBJ)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(COMMON_OBJS) $(TEST_QUICKMODE_OBJ) -o $@ $(LDFLAGS)

$(BUILD_DIR)/test_rooms: $(COMMON_OBJS) $(TEST_ROOMS_OBJ)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(COMMON_OBJS) $(TEST_ROOMS_OBJ) -o $@ $(LDFLAGS)

# ==== RULE BIÊN DỊCH CHUNG ====

# Compile .c -> .o
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# ==== DỌN DẸP ====

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) src/**/*.o src/*.o
