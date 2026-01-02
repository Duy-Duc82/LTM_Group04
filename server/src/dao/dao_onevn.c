#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>
#include "db.h"
#include "dao/dao_onevn.h"
#include "utils/json.h"

int dao_onevn_create_session(int64_t room_id, int64_t *out_session_id) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql =
        "INSERT INTO onevn_sessions (room_id, status, players) "
        "VALUES ($1, 'IN_PROGRESS', '[]'::jsonb) "
        "RETURNING session_id;";

    char buf_room[32];
    snprintf(buf_room, sizeof(buf_room), "%ld", room_id);
    const char *params[1] = { buf_room };

    PGresult *res = PQexecParams(conn, sql, 1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "[DAO_ONEVN] create_session error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    if (PQntuples(res) == 1) {
        *out_session_id = atoll(PQgetvalue(res, 0, 0));
    } else {
        PQclear(res);
        return -1;
    }
    PQclear(res);
    return 0;
}

int dao_onevn_update_players(int64_t session_id, const char *players_json) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql =
        "UPDATE onevn_sessions SET players = $2::jsonb "
        "WHERE session_id = $1;";

    char buf_session[32];
    snprintf(buf_session, sizeof(buf_session), "%ld", session_id);
    const char *params[2] = { buf_session, players_json };

    PGresult *res = PQexecParams(conn, sql, 2, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "[DAO_ONEVN] update_players error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }
    PQclear(res);
    return 0;
}

int dao_onevn_end_session(int64_t session_id, int64_t winner_id) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql =
        "UPDATE onevn_sessions SET status = 'FINISHED', winner_id = $2, ended_at = NOW() "
        "WHERE session_id = $1;";

    char buf_session[32], buf_winner[32];
    snprintf(buf_session, sizeof(buf_session), "%ld", session_id);
    if (winner_id > 0) {
        snprintf(buf_winner, sizeof(buf_winner), "%ld", winner_id);
        const char *params[2] = { buf_session, buf_winner };
        PGresult *res = PQexecParams(conn, sql, 2, NULL, params, NULL, NULL, 0);
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            fprintf(stderr, "[DAO_ONEVN] end_session error: %s\n", PQerrorMessage(conn));
            PQclear(res);
            return -1;
        }
        PQclear(res);
    } else {
        // No winner (aborted)
        const char *sql2 =
            "UPDATE onevn_sessions SET status = 'ABORTED', ended_at = NOW() "
            "WHERE session_id = $1;";
        const char *params[1] = { buf_session };
        PGresult *res = PQexecParams(conn, sql2, 1, NULL, params, NULL, NULL, 0);
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            fprintf(stderr, "[DAO_ONEVN] end_session error: %s\n", PQerrorMessage(conn));
            PQclear(res);
            return -1;
        }
        PQclear(res);
    }
    return 0;
}

int dao_onevn_get_session(int64_t session_id, int64_t *room_id, int64_t *winner_id, char *status, char *players_json, size_t json_len) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql =
        "SELECT room_id, winner_id, status, players::text "
        "FROM onevn_sessions WHERE session_id = $1;";

    char buf_session[32];
    snprintf(buf_session, sizeof(buf_session), "%ld", session_id);
    const char *params[1] = { buf_session };

    PGresult *res = PQexecParams(conn, sql, 1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "[DAO_ONEVN] get_session error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    if (PQntuples(res) == 0) {
        PQclear(res);
        return -1;
    }

    if (room_id) *room_id = atoll(PQgetvalue(res, 0, 0));
    const char *winner_str = PQgetvalue(res, 0, 1);
    if (winner_id) *winner_id = (winner_str && strlen(winner_str) > 0) ? atoll(winner_str) : 0;
    if (status) strncpy(status, PQgetvalue(res, 0, 2), 31);
    if (players_json) {
        const char *players_str = PQgetvalue(res, 0, 3);
        strncpy(players_json, players_str ? players_str : "[]", json_len - 1);
        players_json[json_len - 1] = '\0';
    }

    PQclear(res);
    return 0;
}

int dao_onevn_get_user_history(int64_t user_id, void **json_history) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    // Query sessions where user has played (based on onevn_player_answers)
    // Use LEFT JOIN to get player data from JSONB if available, otherwise use player_answers data
    const char *sql =
        "SELECT DISTINCT s.session_id, s.room_id, s.winner_id, s.status, "
        "       s.started_at, s.ended_at, "
        "       COALESCE((p.player->>'score')::int, a.total_score, 0) as final_score, "
        "       COALESCE((p.player->>'rank')::int, 0) as final_rank, "
        "       CASE WHEN s.winner_id = $1 THEN 1 ELSE 0 END as is_winner "
        "FROM onevn_sessions s "
        "INNER JOIN ("
        "    SELECT r.session_id, pa.user_id, SUM(pa.score_gained) as total_score "
        "    FROM onevn_rounds r "
        "    INNER JOIN onevn_player_answers pa ON r.round_id = pa.round_id "
        "    WHERE pa.user_id = $1 "
        "    GROUP BY r.session_id, pa.user_id"
        ") a ON s.session_id = a.session_id "
        "LEFT JOIN LATERAL ("
        "    SELECT elem as player "
        "    FROM jsonb_array_elements(s.players) elem "
        "    WHERE (elem->>'user_id')::bigint = $1 "
        "    LIMIT 1"
        ") p ON true "
        "WHERE s.status = 'FINISHED' "
        "ORDER BY s.ended_at DESC NULLS LAST, s.started_at DESC "
        "LIMIT 50;";

    char buf[32];
    snprintf(buf, sizeof(buf), "%ld", user_id);
    const char *params[1] = { buf };

    PGresult *res = PQexecParams(conn, sql, 1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "[DAO_ONEVN] get_user_history error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    int rows = PQntuples(res);
    size_t cap = 1024;
    size_t used = 0;
    char *out = malloc(cap);
    if (!out) { PQclear(res); return -1; }
    out[used++] = '[';

    for (int i = 0; i < rows; ++i) {
        const char *session_id = PQgetvalue(res, i, 0);
        const char *room_id = PQgetvalue(res, i, 1);
        const char *winner_id = PQgetvalue(res, i, 2);
        const char *status = PQgetvalue(res, i, 3);
        const char *started_at = PQgetvalue(res, i, 4);
        const char *ended_at = PQgetvalue(res, i, 5);
        const char *final_score = PQgetvalue(res, i, 6);
        const char *final_rank = PQgetvalue(res, i, 7);
        const char *is_winner = PQgetvalue(res, i, 8);

        char *esc_status = util_json_escape(status ? status : "");
        if (!esc_status) esc_status = strdup("");
        char *esc_started = util_json_escape(started_at ? started_at : "");
        if (!esc_started) esc_started = strdup("");
        char *esc_ended = util_json_escape(ended_at ? ended_at : "");
        if (!esc_ended) esc_ended = strdup("");

        int need = snprintf(NULL, 0,
            "{\"session_id\":%s,\"room_id\":%s,\"winner_id\":%s,\"status\":\"%s\","
            "\"started_at\":\"%s\",\"ended_at\":\"%s\",\"final_score\":%s,"
            "\"final_rank\":%s,\"is_winner\":%s,\"played_at\":\"%s\"}",
            session_id, room_id, winner_id && strlen(winner_id) > 0 ? winner_id : "null", esc_status,
            esc_started, esc_ended, final_score ? final_score : "0",
            final_rank ? final_rank : "0", is_winner ? is_winner : "0", esc_ended);

        if (used + (size_t)need + 3 >= cap) {
            cap = (used + (size_t)need + 3) * 2;
            char *tmp = realloc(out, cap);
            if (!tmp) { free(out); free(esc_status); free(esc_started); free(esc_ended); PQclear(res); return -1; }
            out = tmp;
        }

        if (i > 0) out[used++] = ',';
        used += snprintf(out + used, cap - used,
            "{\"session_id\":%s,\"room_id\":%s,\"winner_id\":%s,\"status\":\"%s\","
            "\"started_at\":\"%s\",\"ended_at\":\"%s\",\"final_score\":%s,"
            "\"final_rank\":%s,\"is_winner\":%s,\"played_at\":\"%s\"}",
            session_id, room_id, winner_id && strlen(winner_id) > 0 ? winner_id : "null", esc_status,
            esc_started, esc_ended, final_score ? final_score : "0",
            final_rank ? final_rank : "0", is_winner ? is_winner : "0", esc_ended);

        free(esc_status);
        free(esc_started);
        free(esc_ended);
    }

    if (used + 2 >= cap) {
        char *tmp = realloc(out, used + 2);
        if (!tmp) { free(out); PQclear(res); return -1; }
        out = tmp;
        cap = used + 2;
    }
    out[used++] = ']';
    out[used] = '\0';
    *json_history = out;
    PQclear(res);
    return 0;
}

int dao_onevn_create_round(int64_t session_id, int round_number, int64_t question_id, const char *difficulty, int64_t *out_round_id) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql =
        "INSERT INTO onevn_rounds (session_id, round_number, question_id, difficulty) "
        "VALUES ($1, $2, $3, $4) "
        "RETURNING round_id;";

    char buf_session[32], buf_round[16], buf_question[32];
    snprintf(buf_session, sizeof(buf_session), "%ld", session_id);
    snprintf(buf_round, sizeof(buf_round), "%d", round_number);
    snprintf(buf_question, sizeof(buf_question), "%ld", question_id);
    const char *params[4] = { buf_session, buf_round, buf_question, difficulty };

    PGresult *res = PQexecParams(conn, sql, 4, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "[DAO_ONEVN] create_round error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    if (PQntuples(res) == 1) {
        *out_round_id = atoll(PQgetvalue(res, 0, 0));
    } else {
        PQclear(res);
        return -1;
    }
    PQclear(res);
    return 0;
}

int dao_onevn_end_round(int64_t round_id) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql =
        "UPDATE onevn_rounds SET ended_at = NOW() "
        "WHERE round_id = $1;";

    char buf_round[32];
    snprintf(buf_round, sizeof(buf_round), "%ld", round_id);
    const char *params[1] = { buf_round };

    PGresult *res = PQexecParams(conn, sql, 1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "[DAO_ONEVN] end_round error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }
    PQclear(res);
    return 0;
}

int dao_onevn_save_player_answer(int64_t round_id, int64_t user_id, char answer, int is_correct, int score_gained, double time_left) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql = "INSERT INTO onevn_player_answers (round_id, user_id, answer, is_correct, score_gained, time_left) "
                      "VALUES ($1, $2, $3, $4, $5, $6) "
                      "ON CONFLICT (round_id, user_id) DO UPDATE SET "
                      "  answer = EXCLUDED.answer, "
                      "  is_correct = EXCLUDED.is_correct, "
                      "  score_gained = EXCLUDED.score_gained, "
                      "  time_left = EXCLUDED.time_left, "
                      "  answered_at = NOW();";

    char buf_round[32], buf_user[32], buf_answer[2], buf_correct[8], buf_score[16], buf_time[32];
    snprintf(buf_round, sizeof(buf_round), "%ld", round_id);
    snprintf(buf_user, sizeof(buf_user), "%ld", user_id);
    snprintf(buf_correct, sizeof(buf_correct), "%s", is_correct ? "true" : "false");
    snprintf(buf_score, sizeof(buf_score), "%d", score_gained);
    
    const char *params[6];
    int paramLengths[6];
    int paramFormats[6];
    
    params[0] = buf_round;
    params[1] = buf_user;
    paramLengths[0] = (int)strlen(buf_round);
    paramLengths[1] = (int)strlen(buf_user);
    paramFormats[0] = 0;  // text
    paramFormats[1] = 0;  // text
    
    if (answer == '\0') {
        // Timeout case - answer is NULL (use -1 for paramLengths to indicate NULL)
        params[2] = NULL;
        params[5] = NULL;  // time_left is NULL
        paramLengths[2] = -1;  // -1 indicates NULL
        paramLengths[5] = -1;  // -1 indicates NULL
        paramFormats[2] = 0;
        paramFormats[5] = 0;
    } else {
        // Normal answer
        buf_answer[0] = answer;
        buf_answer[1] = '\0';
        params[2] = buf_answer;
        paramLengths[2] = 1;
        paramFormats[2] = 0;
        
        snprintf(buf_time, sizeof(buf_time), "%.2f", time_left);
        params[5] = buf_time;
        paramLengths[5] = (int)strlen(buf_time);
        paramFormats[5] = 0;
    }
    
    params[3] = buf_correct;
    params[4] = buf_score;
    paramLengths[3] = (int)strlen(buf_correct);
    paramLengths[4] = (int)strlen(buf_score);
    paramFormats[3] = 0;
    paramFormats[4] = 0;

    PGresult *res = PQexecParams(conn, sql, 6, NULL, params, paramLengths, paramFormats, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "[DAO_ONEVN] save_player_answer error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }
    PQclear(res);
    return 0;
}

int dao_onevn_get_replay_details(int64_t session_id, void **json_replay) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    // Get session info
    int64_t room_id = 0, winner_id = 0;
    char status[32] = {0};
    char players_json[4096] = {0};
    if (dao_onevn_get_session(session_id, &room_id, &winner_id, status, players_json, sizeof(players_json)) != 0) {
        return -1;
    }

    // Get rounds with questions and answers
    const char *sql =
        "SELECT r.round_id, r.round_number, r.difficulty, r.started_at, r.ended_at, "
        "       r.question_id, q.content, q.\"opA\", q.\"opB\", q.\"opC\", q.\"opD\", q.correct_op, q.explanation "
        "FROM onevn_rounds r "
        "JOIN question q ON r.question_id = q.question_id "
        "WHERE r.session_id = $1 "
        "ORDER BY r.round_number ASC;";

    char buf_session[32];
    snprintf(buf_session, sizeof(buf_session), "%ld", session_id);
    const char *params[1] = { buf_session };

    PGresult *res = PQexecParams(conn, sql, 1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "[DAO_ONEVN] get_replay_details error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    int round_rows = PQntuples(res);
    size_t cap = 4096;
    size_t used = 0;
    char *out = malloc(cap);
    if (!out) { PQclear(res); return -1; }

    // Start JSON object
    char buf_winner[32];
    if (winner_id > 0) {
        snprintf(buf_winner, sizeof(buf_winner), "%ld", winner_id);
    }
    used += snprintf(out + used, cap - used,
        "{\"session_id\":%ld,\"room_id\":%ld,\"winner_id\":%s,\"status\":\"%s\","
        "\"players\":%s,\"rounds\":[",
        session_id, room_id, winner_id > 0 ? buf_winner : "null", status, players_json);

    for (int i = 0; i < round_rows; ++i) {
        const char *round_id = PQgetvalue(res, i, 0);
        const char *round_number = PQgetvalue(res, i, 1);
        const char *difficulty = PQgetvalue(res, i, 2);
        const char *started_at = PQgetvalue(res, i, 3);
        const char *ended_at = PQgetvalue(res, i, 4);
        const char *question_id = PQgetvalue(res, i, 5);
        const char *content = PQgetvalue(res, i, 6);
        const char *opA = PQgetvalue(res, i, 7);
        const char *opB = PQgetvalue(res, i, 8);
        const char *opC = PQgetvalue(res, i, 9);
        const char *opD = PQgetvalue(res, i, 10);
        const char *correct_op = PQgetvalue(res, i, 11);
        const char *explanation = PQgetvalue(res, i, 12);

        // Escape strings
        char *esc_difficulty = util_json_escape(difficulty ? difficulty : "");
        char *esc_content = util_json_escape(content ? content : "");
        char *esc_opA = util_json_escape(opA ? opA : "");
        char *esc_opB = util_json_escape(opB ? opB : "");
        char *esc_opC = util_json_escape(opC ? opC : "");
        char *esc_opD = util_json_escape(opD ? opD : "");
        char *esc_correct_op = util_json_escape(correct_op ? correct_op : "");
        char *esc_explanation = util_json_escape(explanation ? explanation : "");
        char *esc_started = util_json_escape(started_at ? started_at : "");
        char *esc_ended = util_json_escape(ended_at ? ended_at : "");

        // Get answers for this round
        const char *sql_answers =
            "SELECT user_id, answer, is_correct, score_gained, time_left, answered_at "
            "FROM onevn_player_answers "
            "WHERE round_id = $1 "
            "ORDER BY answered_at ASC;";

        char buf_round[32];
        snprintf(buf_round, sizeof(buf_round), "%s", round_id);
        const char *params_answers[1] = { buf_round };
        PGresult *res_answers = PQexecParams(conn, sql_answers, 1, NULL, params_answers, NULL, NULL, 0);

        size_t answers_json_cap = 512;
        size_t answers_json_used = 0;
        char *answers_json = malloc(answers_json_cap);
        if (!answers_json) answers_json_used = 0;
        else answers_json[answers_json_used++] = '[';

        if (res_answers && PQresultStatus(res_answers) == PGRES_TUPLES_OK) {
            int answer_rows = PQntuples(res_answers);
            for (int j = 0; j < answer_rows; ++j) {
                const char *user_id = PQgetvalue(res_answers, j, 0);
                const char *answer = PQgetvalue(res_answers, j, 1);
                const char *is_correct = PQgetvalue(res_answers, j, 2);
                const char *score_gained = PQgetvalue(res_answers, j, 3);
                const char *time_left = PQgetvalue(res_answers, j, 4);
                const char *answered_at = PQgetvalue(res_answers, j, 5);

                // Handle NULL answer (timeout)
                const char *answer_val = answer && strlen(answer) > 0 ? answer : "";
                char *esc_answer = answer_val[0] != '\0' ? util_json_escape(answer_val) : strdup("\"\"");
                char *esc_answered_at = util_json_escape(answered_at ? answered_at : "");
                const char *answer_json = answer_val[0] != '\0' ? esc_answer : "null";
                const char *time_left_val = time_left && strlen(time_left) > 0 ? time_left : "null";

                int need_ans = snprintf(NULL, 0,
                    "{\"user_id\":%s,\"answer\":%s,\"is_correct\":%s,\"score_gained\":%s,\"time_left\":%s,\"answered_at\":\"%s\"}",
                    user_id, answer_json, is_correct, score_gained ? score_gained : "0", time_left_val, esc_answered_at);

                if (answers_json_used + (size_t)need_ans + 3 >= answers_json_cap) {
                    answers_json_cap = (answers_json_used + (size_t)need_ans + 3) * 2;
                    char *tmp = realloc(answers_json, answers_json_cap);
                    if (!tmp) { free(answers_json); free(esc_answer); free(esc_answered_at); answers_json = NULL; break; }
                    answers_json = tmp;
                }

                if (j > 0) answers_json[answers_json_used++] = ',';
                answers_json_used += snprintf(answers_json + answers_json_used, answers_json_cap - answers_json_used,
                    "{\"user_id\":%s,\"answer\":%s,\"is_correct\":%s,\"score_gained\":%s,\"time_left\":%s,\"answered_at\":\"%s\"}",
                    user_id, answer_json, is_correct, score_gained ? score_gained : "0", time_left_val, esc_answered_at);

                free(esc_answer);
                free(esc_answered_at);
            }
        }
        if (res_answers) PQclear(res_answers);
        if (answers_json) {
            if (answers_json_used + 2 >= answers_json_cap) {
                char *tmp = realloc(answers_json, answers_json_used + 2);
                if (tmp) { answers_json = tmp; answers_json_cap = answers_json_used + 2; }
            }
            answers_json[answers_json_used++] = ']';
            answers_json[answers_json_used] = '\0';
        } else {
            answers_json = strdup("[]");
        }

        int need = snprintf(NULL, 0,
            "{\"round_id\":%s,\"round_number\":%s,\"difficulty\":\"%s\","
            "\"question_id\":%s,\"content\":\"%s\",\"opA\":\"%s\",\"opB\":\"%s\",\"opC\":\"%s\",\"opD\":\"%s\","
            "\"correct_op\":\"%s\",\"explanation\":\"%s\",\"started_at\":\"%s\",\"ended_at\":\"%s\",\"answers\":%s}",
            round_id, round_number, esc_difficulty, question_id, esc_content,
            esc_opA, esc_opB, esc_opC, esc_opD, esc_correct_op, esc_explanation,
            esc_started, esc_ended, answers_json);

        if (used + (size_t)need + 3 >= cap) {
            cap = (used + (size_t)need + 3) * 2;
            char *tmp = realloc(out, cap);
            if (!tmp) {
                free(out); free(esc_difficulty); free(esc_content); free(esc_opA); free(esc_opB);
                free(esc_opC); free(esc_opD); free(esc_correct_op); free(esc_explanation);
                free(esc_started); free(esc_ended); free(answers_json); PQclear(res); return -1;
            }
            out = tmp;
        }

        if (i > 0) out[used++] = ',';
        used += snprintf(out + used, cap - used,
            "{\"round_id\":%s,\"round_number\":%s,\"difficulty\":\"%s\","
            "\"question_id\":%s,\"content\":\"%s\",\"opA\":\"%s\",\"opB\":\"%s\",\"opC\":\"%s\",\"opD\":\"%s\","
            "\"correct_op\":\"%s\",\"explanation\":\"%s\",\"started_at\":\"%s\",\"ended_at\":\"%s\",\"answers\":%s}",
            round_id, round_number, esc_difficulty, question_id, esc_content,
            esc_opA, esc_opB, esc_opC, esc_opD, esc_correct_op, esc_explanation,
            esc_started, esc_ended, answers_json);

        free(esc_difficulty); free(esc_content); free(esc_opA); free(esc_opB);
        free(esc_opC); free(esc_opD); free(esc_correct_op); free(esc_explanation);
        free(esc_started); free(esc_ended); free(answers_json);
    }

    if (used + 3 >= cap) {
        char *tmp = realloc(out, used + 3);
        if (!tmp) { free(out); PQclear(res); return -1; }
        out = tmp;
        cap = used + 3;
    }
    out[used++] = ']';
    out[used++] = '}';
    out[used] = '\0';
    *json_replay = out;
    PQclear(res);
    return 0;
}
