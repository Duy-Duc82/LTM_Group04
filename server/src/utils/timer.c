// server/src/utils/timer.c
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "utils/timer.h"

// Simple timer struct for callback-based timers
typedef struct {
    int timer_id;
    time_t start_time;
    int timeout_seconds;
    int64_t context_id;
    timer_callback_t callback;
    void *user_data;
    int active;
} CallbackTimer;

#define MAX_CALLBACK_TIMERS 100
static CallbackTimer callback_timers[MAX_CALLBACK_TIMERS];
static int next_timer_id = 1;

Timer timer_init(int timeout_seconds) {
    Timer t;
    t.start_time = time(NULL);
    t.timeout_seconds = timeout_seconds;
    return t;
}

int timer_is_expired(Timer *timer) {
    time_t now = time(NULL);
    int elapsed = (int)(now - timer->start_time);
    return elapsed >= timer->timeout_seconds ? 1 : 0;
}

int timer_get_remaining(Timer *timer) {
    time_t now = time(NULL);
    int elapsed = (int)(now - timer->start_time);
    int remaining = timer->timeout_seconds - elapsed;
    return remaining > 0 ? remaining : 0;
}

void timer_reset(Timer *timer) {
    timer->start_time = time(NULL);
}

// Callback-based timer functions (game_timer_* to avoid conflict with POSIX timer_create)
int game_timer_create(int timeout_seconds, int64_t context_id, timer_callback_t callback, void *user_data) {
    if (!callback) return -1;
    
    // Find free slot
    for (int i = 0; i < MAX_CALLBACK_TIMERS; i++) {
        if (!callback_timers[i].active) {
            callback_timers[i].timer_id = next_timer_id++;
            callback_timers[i].start_time = time(NULL);
            callback_timers[i].timeout_seconds = timeout_seconds;
            callback_timers[i].context_id = context_id;
            callback_timers[i].callback = callback;
            callback_timers[i].user_data = user_data;
            callback_timers[i].active = 1;
            return callback_timers[i].timer_id;
        }
    }
    return -1; // No free slots
}

void game_timer_cancel(int timer_id) {
    if (timer_id < 0) return;
    
    for (int i = 0; i < MAX_CALLBACK_TIMERS; i++) {
        if (callback_timers[i].active && callback_timers[i].timer_id == timer_id) {
            callback_timers[i].active = 0;
            return;
        }
    }
}

void game_timer_check_and_run(void) {
    time_t now = time(NULL);
    
    for (int i = 0; i < MAX_CALLBACK_TIMERS; i++) {
        if (!callback_timers[i].active) continue;
        
        int elapsed = (int)(now - callback_timers[i].start_time);
        if (elapsed >= callback_timers[i].timeout_seconds) {
            // Timer expired, call callback
            timer_callback_t cb = callback_timers[i].callback;
            int64_t ctx_id = callback_timers[i].context_id;
            void *user_data = callback_timers[i].user_data;
            
            // Mark as inactive before calling callback (in case callback cancels it)
            callback_timers[i].active = 0;
            
            // Call callback
            if (cb) {
                cb(ctx_id, user_data);
            }
        }
    }
}

void game_timer_cleanup(void) {
    // Remove inactive timers (already handled in game_timer_check_and_run)
    // This function can be used for additional cleanup if needed
}
