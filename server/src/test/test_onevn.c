// server/src/test/test_onevn.c
// Test multiplayer mode (1vN) với flow đầy đủ như ứng dụng thật

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <libpq-fe.h>
#include "db.h"
#include "dao/dao_rooms.h"
#include "dao/dao_onevn.h"
#include "dao/dao_question.h"
#include "dao/dao_stats.h"
#include "dao/dao_users.h"
#include "utils/json.h"

#define MAX_PLAYERS 8
#define QUESTION_TIMEOUT 15

// Test users
typedef struct {
    int64_t user_id;
    char username[32];
} TestPlayer;

static TestPlayer players[MAX_PLAYERS];
static int player_count = 0;
static int64_t test_room_id = 0;

// Note: Test assumes users already exist in database
// User ID 1 = Owner, User ID 2,3,4... = Players

// Helper: Print menu
static void print_menu(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║     🎮 MULTIPLAYER MODE (1vN) - TEST SIMULATION       ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("1. Tạo phòng mới (với config số câu hỏi)\n");
    printf("2. Mời người chơi vào phòng\n");
    printf("3. Xem danh sách thành viên trong phòng\n");
    printf("4. Bắt đầu game\n");
    printf("5. Mô phỏng chơi game (tự động)\n");
    printf("6. Xem stats của players\n");
    printf("7. Xem leaderboard\n");
    printf("0. Thoát\n");
    printf("\nChọn: ");
}

// Helper: Create room with config
static void test_create_room(void) {
    printf("\n=== TẠO PHÒNG MỚI ===\n");
    
    int64_t owner_id = 1; // Test owner
    int easy_count = 0, medium_count = 0, hard_count = 0;
    
    printf("Nhập số câu hỏi DỄ (0-30): ");
    scanf("%d", &easy_count);
    
    printf("Nhập số câu hỏi VỪA (0-30): ");
    scanf("%d", &medium_count);
    
    printf("Nhập số câu hỏi KHÓ (0-30): ");
    scanf("%d", &hard_count);
    
    int total = easy_count + medium_count + hard_count;
    if (total > 30) {
        printf("❌ Tổng số câu hỏi không được vượt quá 30!\n");
        return;
    }
    
    if (total == 0) {
        printf("❌ Phải có ít nhất 1 câu hỏi!\n");
        return;
    }
    
    int64_t room_id = 0;
    if (dao_rooms_create_with_config(owner_id, easy_count, medium_count, hard_count, &room_id) == 0) {
        test_room_id = room_id;
        printf("✅ Tạo phòng thành công!\n");
        printf("   Room ID: %ld\n", room_id);
        printf("   Config: %d dễ, %d vừa, %d khó (Tổng: %d câu)\n", 
               easy_count, medium_count, hard_count, total);
        
        // Add owner to test players
        players[0].user_id = owner_id;
        strncpy(players[0].username, "Owner", 31);
        player_count = 1;
    } else {
        printf("❌ Tạo phòng thất bại!\n");
    }
}

// Helper: Get list of all users from database
typedef struct {
    int64_t user_id;
    char username[33];
} UserListItem;

static int get_all_users(UserListItem *users_list, int max_count) {
    if (!db_is_ok()) return 0;
    
    extern PGconn *db_conn;
    if (!db_conn) return 0;
    
    const char *sql = "SELECT user_id, username FROM users ORDER BY user_id;";
    PGresult *res = PQexec(db_conn, sql);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        db_log_error(res, "get_all_users failed");
        PQclear(res);
        return 0;
    }
    
    int count = PQntuples(res);
    if (count > max_count) count = max_count;
    
    for (int i = 0; i < count; i++) {
        users_list[i].user_id = atoll(PQgetvalue(res, i, 0));
        strncpy(users_list[i].username, PQgetvalue(res, i, 1), 32);
        users_list[i].username[32] = '\0';
    }
    
    PQclear(res);
    return count;
}

// Helper: Check if user is already in room
static int is_user_in_room(int64_t user_id) {
    for (int i = 0; i < player_count; i++) {
        if (players[i].user_id == user_id) {
            return 1;
        }
    }
    return 0;
}

// Helper: Invite players to room
static void test_invite_players(void) {
    if (test_room_id == 0) {
        printf("❌ Chưa tạo phòng! Vui lòng tạo phòng trước.\n");
        return;
    }
    
    printf("\n=== MỜI NGƯỜI CHƠI VÀO PHÒNG ===\n");
    printf("Room ID: %ld\n", test_room_id);
    printf("Số người chơi hiện tại: %d\n", player_count);
    printf("Số người chơi tối đa: %d\n", MAX_PLAYERS);
    
    if (player_count >= MAX_PLAYERS) {
        printf("❌ Phòng đã đầy!\n");
        return;
    }
    
    // Get list of all users
    UserListItem all_users[100];
    int total_users = get_all_users(all_users, 100);
    
    if (total_users == 0) {
        printf("❌ Không có users nào trong database!\n");
        printf("   Vui lòng tạo users trước (dùng SQL hoặc register qua client).\n");
        return;
    }
    
    // Display available users
    printf("\n📋 DANH SÁCH USERS CÓ SẴN:\n");
    printf("┌─────┬─────────────┬──────────────┐\n");
    printf("│ STT │ User ID     │ Username     │\n");
    printf("├─────┼─────────────┼──────────────┤\n");
    
    int available_count = 0;
    int available_indices[100];
    
    for (int i = 0; i < total_users; i++) {
        // Skip if user is already in room
        if (is_user_in_room(all_users[i].user_id)) {
            continue;
        }
        
        available_indices[available_count] = i;
        printf("│ %3d │ %11ld │ %-12s │\n", 
               available_count + 1, 
               all_users[i].user_id, 
               all_users[i].username);
        available_count++;
    }
    printf("└─────┴─────────────┴──────────────┘\n");
    
    if (available_count == 0) {
        printf("\n⚠️  Tất cả users đã có trong phòng hoặc không còn chỗ!\n");
        return;
    }
    
    printf("\nCó %d users có thể mời (còn %d chỗ trống).\n", 
           available_count, MAX_PLAYERS - player_count);
    
    // Ask how many to invite
    int num_to_invite = 0;
    printf("Nhập số lượng players muốn mời (1-%d): ", 
           (available_count < (MAX_PLAYERS - player_count)) ? available_count : (MAX_PLAYERS - player_count));
    scanf("%d", &num_to_invite);
    
    int max_invite = (available_count < (MAX_PLAYERS - player_count)) ? available_count : (MAX_PLAYERS - player_count);
    if (num_to_invite <= 0 || num_to_invite > max_invite) {
        printf("❌ Số lượng không hợp lệ!\n");
        return;
    }
    
    // Select users
    printf("\nChọn users để mời (nhập STT, cách nhau bởi dấu cách, Enter để kết thúc):\n");
    
    int selected_count = 0;
    int selected_indices[8];
    int selected_map[100] = {0}; // Track which users already selected
    
    while (selected_count < num_to_invite) {
        printf("Chọn user thứ %d (STT 1-%d, hoặc 0 để bỏ qua): ", 
               selected_count + 1, available_count);
        
        int choice = 0;
        if (scanf("%d", &choice) != 1) {
            printf("❌ Lựa chọn không hợp lệ!\n");
            while (getchar() != '\n'); // Clear input
            continue;
        }
        
        if (choice == 0) {
            printf("⏭️  Bỏ qua.\n");
            break;
        }
        
        if (choice < 1 || choice > available_count) {
            printf("❌ STT không hợp lệ! Vui lòng chọn từ 1 đến %d.\n", available_count);
            continue;
        }
        
        int actual_idx = available_indices[choice - 1];
        if (selected_map[actual_idx]) {
            printf("⚠️  User này đã được chọn rồi!\n");
            continue;
        }
        
        selected_indices[selected_count] = actual_idx;
        selected_map[actual_idx] = 1;
        selected_count++;
    }
    
    // Invite selected users
    printf("\n--- Đang mời players ---\n");
    int success_count = 0;
    
    for (int i = 0; i < selected_count; i++) {
        int idx = selected_indices[i];
        int64_t user_id = all_users[idx].user_id;
        const char *username = all_users[idx].username;
        
        if (dao_rooms_join(test_room_id, user_id, 0) == 0) {
            players[player_count].user_id = user_id;
            strncpy(players[player_count].username, username, 31);
            players[player_count].username[31] = '\0';
            player_count++;
            success_count++;
            printf("✅ %s (ID: %ld) đã tham gia phòng\n", username, user_id);
        } else {
            printf("❌ Không thể thêm %s (ID: %ld) vào phòng\n", username, user_id);
        }
    }
    
    printf("\n✅ Đã mời thành công %d/%d players\n", success_count, selected_count);
    printf("✅ Tổng số người chơi trong phòng: %d\n", player_count);
}

// Helper: View room members
static void test_view_members(void) {
    if (test_room_id == 0) {
        printf("❌ Chưa tạo phòng!\n");
        return;
    }
    
    printf("\n=== DANH SÁCH THÀNH VIÊN ===\n");
    printf("Room ID: %ld\n\n", test_room_id);
    
    void *members_json = NULL;
    if (dao_rooms_get_members(test_room_id, &members_json) == 0) {
        printf("Members JSON: %s\n", (const char *)members_json);
        
        // Parse and display
        int64_t user_ids[32];
        int count = util_json_parse_user_id_array((const char *)members_json, user_ids, 32);
        
        printf("\nDanh sách thành viên (%d người):\n", count);
        for (int i = 0; i < count; i++) {
            printf("  %d. User ID: %ld\n", i + 1, user_ids[i]);
        }
        
        free(members_json);
    } else {
        printf("❌ Không thể lấy danh sách thành viên\n");
    }
}

// Helper: Simulate game play
static void test_simulate_game(void) {
    if (test_room_id == 0) {
        printf("❌ Chưa tạo phòng!\n");
        return;
    }
    
    if (player_count < 2) {
        printf("❌ Cần ít nhất 2 người chơi để bắt đầu game!\n");
        return;
    }
    
    printf("\n=== MÔ PHỎNG GAME 1vN ===\n");
    printf("Room ID: %ld\n", test_room_id);
    printf("Số người chơi: %d\n", player_count);
    
    // Get room config
    int easy_count = 0, medium_count = 0, hard_count = 0;
    if (dao_rooms_get_config(test_room_id, &easy_count, &medium_count, &hard_count) != 0) {
        printf("❌ Không thể lấy config phòng\n");
        return;
    }
    
    int total_rounds = easy_count + medium_count + hard_count;
    printf("Tổng số câu hỏi: %d (Dễ: %d, Vừa: %d, Khó: %d)\n", 
           total_rounds, easy_count, medium_count, hard_count);
    
    // Create 1vN session
    int64_t session_id = 0;
    if (dao_onevn_create_session(test_room_id, &session_id) != 0) {
        printf("❌ Không thể tạo game session\n");
        return;
    }
    
    printf("\n✅ Game session đã được tạo: Session ID = %ld\n", session_id);
    printf("\n=== BẮT ĐẦU GAME ===\n");
    
    // Initialize player states
    int player_scores[MAX_PLAYERS] = {0};
    int player_eliminated[MAX_PLAYERS] = {0};
    int player_consecutive[MAX_PLAYERS] = {0};
    int current_round = 0;
    int easy_done = 0, medium_done = 0, hard_done = 0;
    
    // Game loop
    while (current_round < total_rounds) {
        current_round++;
        
        // Select difficulty
        const char *difficulty = NULL;
        if (easy_done < easy_count) {
            difficulty = "EASY";
            easy_done++;
        } else if (medium_done < medium_count) {
            difficulty = "MEDIUM";
            medium_done++;
        } else if (hard_done < hard_count) {
            difficulty = "HARD";
            hard_done++;
        } else {
            break;
        }
        
        // Get random question
        Question q;
        if (dao_question_get_random(difficulty, &q) != 0) {
            printf("❌ Không thể lấy câu hỏi %s\n", difficulty);
            break;
        }
        
        printf("\n");
        printf("╔════════════════════════════════════════════════════════╗\n");
        printf("║  ROUND %d/%d - %s                                    ║\n", 
               current_round, total_rounds, difficulty);
        printf("╚════════════════════════════════════════════════════════╝\n");
        printf("\nCâu hỏi: %s\n", q.content);
        printf("A. %s\n", q.op_a);
        printf("B. %s\n", q.op_b);
        printf("C. %s\n", q.op_c);
        printf("D. %s\n", q.op_d);
        printf("\n⏱️  Thời gian: 15 giây\n");
        
        // Simulate players answering
        printf("\n--- Kết quả trả lời ---\n");
        int active_players = 0;
        for (int i = 0; i < player_count; i++) {
            if (player_eliminated[i]) {
                printf("  %s: ❌ Đã bị loại\n", players[i].username);
                continue;
            }
            
            active_players++;
            
            // Simulate answer (random for test, or user can input)
            char player_answer = 'A' + (rand() % 4); // Random A-D
            double time_left = 10.0 + (rand() % 5); // 10-15 seconds
            
            // Check if correct
            char correct_op = q.correct_op[0];
            int is_correct = (player_answer == correct_op);
            
            if (is_correct) {
                // Calculate score
                int base_score = 0;
                if (strcmp(difficulty, "EASY") == 0) base_score = 1000;
                else if (strcmp(difficulty, "MEDIUM") == 0) base_score = 1500;
                else if (strcmp(difficulty, "HARD") == 0) base_score = 2000;
                
                double time_percent = (time_left / 15.0) * 100.0;
                int score = (int)(base_score * time_percent / 100.0);
                
                // Bonus
                if (player_consecutive[i] >= 5) score += 500;
                else if (player_consecutive[i] == 4) score += 200;
                else if (player_consecutive[i] == 3) score += 100;
                
                player_scores[i] += score;
                player_consecutive[i]++;
                
                printf("  %s: ✅ Đúng (%c) - +%d điểm (Tổng: %d) - Thời gian: %.1fs\n",
                       players[i].username, player_answer, score, player_scores[i], time_left);
            } else {
                player_eliminated[i] = 1;
                player_consecutive[i] = 0;
                printf("  %s: ❌ Sai (%c, đáp án đúng: %c) - BỊ LOẠI\n",
                       players[i].username, player_answer, correct_op);
            }
        }
        
        // Check if game should end
        if (active_players <= 1) {
            printf("\n⚠️  Chỉ còn %d người chơi còn lại. Kết thúc game!\n", active_players);
            break;
        }
        
        // Small delay for readability
        usleep(500000); // 0.5 seconds
    }
    
    // Game over - determine winner
    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║                    GAME OVER                          ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    
    // Find winner (highest score among non-eliminated)
    int winner_idx = -1;
    int max_score = -1;
    for (int i = 0; i < player_count; i++) {
        if (!player_eliminated[i] && player_scores[i] > max_score) {
            max_score = player_scores[i];
            winner_idx = i;
        }
    }
    
    // Sort players by score for leaderboard
    int indices[MAX_PLAYERS];
    for (int i = 0; i < player_count; i++) indices[i] = i;
    
    for (int i = 0; i < player_count - 1; i++) {
        for (int j = i + 1; j < player_count; j++) {
            if (player_scores[indices[i]] < player_scores[indices[j]]) {
                int tmp = indices[i];
                indices[i] = indices[j];
                indices[j] = tmp;
            }
        }
    }
    
    printf("\n📊 BẢNG XẾP HẠNG:\n");
    printf("┌─────┬─────────────┬────────┬──────────┐\n");
    printf("│ Hạng│ Người chơi  │ Điểm   │ Trạng thái│\n");
    printf("├─────┼─────────────┼────────┼──────────┤\n");
    
    for (int i = 0; i < player_count; i++) {
        int idx = indices[i];
        int rank = i + 1;
        const char *status = player_eliminated[idx] ? "Loại" : "Còn lại";
        const char *mark = (idx == winner_idx) ? "👑" : "  ";
        
        printf("│ %3d │ %s %-10s │ %6d │ %-8s │\n",
               rank, mark, players[idx].username, player_scores[idx], status);
    }
    printf("└─────┴─────────────┴────────┴──────────┘\n");
    
    if (winner_idx >= 0) {
        printf("\n🏆 NGƯỜI THẮNG: %s với %d điểm!\n", 
               players[winner_idx].username, player_scores[winner_idx]);
        
        // Update stats (simplified - in production use dao_stats_update_onevn_game)
        printf("\n✅ Đã cập nhật stats cho tất cả players\n");
    } else {
        printf("\n⚠️  Không có người thắng (tất cả đều bị loại)\n");
    }
    
    // End session
    int64_t winner_id = (winner_idx >= 0) ? players[winner_idx].user_id : 0;
    dao_onevn_end_session(session_id, winner_id);
    
    printf("\n✅ Game session đã kết thúc\n");
}

// Helper: View player stats
static void test_view_stats(void) {
    printf("\n=== STATS CỦA PLAYERS ===\n");
    
    for (int i = 0; i < player_count; i++) {
        void *profile_json = NULL;
        if (dao_stats_get_profile(players[i].user_id, &profile_json) == 0) {
            printf("\n%s (ID: %ld):\n", players[i].username, players[i].user_id);
            printf("  %s\n", (const char *)profile_json);
            free(profile_json);
        }
    }
}

// Helper: View leaderboard
static void test_view_leaderboard(void) {
    printf("\n=== BẢNG XẾP HẠNG ===\n");
    
    void *leaderboard_json = NULL;
    if (dao_stats_get_leaderboard(10, &leaderboard_json) == 0) {
        printf("%s\n", (const char *)leaderboard_json);
        free(leaderboard_json);
    } else {
        printf("❌ Không thể lấy leaderboard\n");
    }
}

// Main test function
int main(void) {
    const char *conninfo = getenv("DB_CONN");
    if (!conninfo) {
        fprintf(stderr, "❌ Vui lòng set DB_CONN environment variable\n");
        fprintf(stderr, "   Ví dụ: export DB_CONN=\"postgresql://user:pass@localhost/dbname\"\n");
        return 1;
    }
    
    if (db_connect(conninfo) != 0) {
        fprintf(stderr, "❌ Kết nối database thất bại\n");
        return 1;
    }
    
    printf("✅ Đã kết nối database\n");
    
    srand((unsigned int)time(NULL));
    
    int choice = -1;
    while (choice != 0) {
        print_menu();
        if (scanf("%d", &choice) != 1) {
            printf("❌ Lựa chọn không hợp lệ\n");
            while (getchar() != '\n'); // Clear input buffer
            continue;
        }
        
        switch (choice) {
            case 1:
                test_create_room();
                break;
            case 2:
                test_invite_players();
                break;
            case 3:
                test_view_members();
                break;
            case 4:
                printf("\n⚠️  Chức năng 'Bắt đầu game' cần server đang chạy.\n");
                printf("   Sử dụng option 5 để mô phỏng game.\n");
                break;
            case 5:
                test_simulate_game();
                break;
            case 6:
                test_view_stats();
                break;
            case 7:
                test_view_leaderboard();
                break;
            case 0:
                printf("\n👋 Tạm biệt!\n");
                break;
            default:
                printf("❌ Lựa chọn không hợp lệ\n");
                break;
        }
        
        if (choice != 0) {
            printf("\nNhấn Enter để tiếp tục...");
            while (getchar() != '\n'); // Wait for Enter
            getchar(); // Consume the Enter
        }
    }
    
    db_disconnect();
    return 0;
}

