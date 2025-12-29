#ifndef DAO_STATS_H
#define DAO_STATS_H

#include <stdint.h>

int dao_stats_get_profile(int64_t user_id, void **json_profile);
int dao_stats_get_leaderboard(int limit, void **json_leaderboard);
int dao_stats_get_match_history(int64_t user_id, void **json_history);
int dao_stats_update_quickmode_game(int64_t user_id, int is_win);
int dao_stats_update_onevn_game(int64_t winner_id, int64_t *player_ids, int *player_scores, int *player_eliminated, int player_count);

#endif
