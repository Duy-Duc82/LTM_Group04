// server/src/utils/timer.c
#include <time.h>
#include "utils/timer.h"

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
