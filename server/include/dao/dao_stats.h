#ifndef DAO_STATS_H
#define DAO_STATS_H

#include <stdint.h>

int dao_stats_get_profile(int64_t user_id, void **json_profile);
int dao_stats_get_leaderboard(int limit, void **json_leaderboard);
int dao_stats_get_match_history(int64_t user_id, void **json_history);

#endif
