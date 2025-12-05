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

#endif
