// server/include/utils/timer.h
#ifndef TIMER_H
#define TIMER_H

#include <time.h>
#include <stdint.h>

typedef struct {
    time_t start_time;
    int timeout_seconds;
} Timer;

/**
 * Khởi tạo timer với timeout (giây)
 */
Timer timer_init(int timeout_seconds);

/**
 * Kiểm tra xem timer có hết hạn không
 * Returns: 1 nếu hết hạn, 0 nếu còn thời gian
 */
int timer_is_expired(Timer *timer);

/**
 * Lấy thời gian còn lại (giây)
 * Returns: số giây còn lại, hoặc 0 nếu đã hết
 */
int timer_get_remaining(Timer *timer);

/**
 * Reset timer
 */
void timer_reset(Timer *timer);

// ========== Callback-based Timer System ==========

// Timer callback function type
typedef void (*timer_callback_t)(int64_t context_id, void *user_data);

/**
 * Create a game timer with callback (for async operations)
 * Returns: timer ID (>= 0) on success, -1 on error
 * Note: Named game_timer_* to avoid conflict with POSIX timer_create
 */
int game_timer_create(int timeout_seconds, int64_t context_id, timer_callback_t callback, void *user_data);

/**
 * Cancel a game timer by ID
 */
void game_timer_cancel(int timer_id);

/**
 * Check and run expired game timers (call this periodically in main loop)
 */
void game_timer_check_and_run(void);

/**
 * Cleanup expired game timers
 */
void game_timer_cleanup(void);

#endif
