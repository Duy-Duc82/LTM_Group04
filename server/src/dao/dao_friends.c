#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>
#include "../include/db.h"
#include "dao/dao_friends.h"
#include "utils/json.h"

// Convert enum -> text
static const char *friend_status_to_str(friend_status_t st) {
    switch (st) {
        case FRIEND_STATUS_PENDING:  return "PENDING";
        case FRIEND_STATUS_ACCEPTED: return "ACCEPTED";
        case FRIEND_STATUS_BLOCKED:  return "BLOCKED";
        default: return "PENDING";
    }
}

// Add DECLINED status
#define FRIEND_STATUS_DECLINED 3

int dao_friends_send_request(int64_t from_user, int64_t to_user) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql =
        "INSERT INTO friend_relationships (user_id, peer_user_id, status) "
        "VALUES ($1, $2, 'PENDING') "
        "ON CONFLICT (user_id, peer_user_id) DO UPDATE SET status = 'PENDING', responded_at = NULL;";

    const char *params[2];
    char buf1[32], buf2[32];
    snprintf(buf1, sizeof(buf1), "%ld", from_user);
    snprintf(buf2, sizeof(buf2), "%ld", to_user);
    params[0] = buf1;
    params[1] = buf2;

    PGresult *res = PQexecParams(conn, sql, 2, NULL, params, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "[DAO_FRIENDS] send_request error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }
    PQclear(res);
    return 0;
}

int dao_friends_respond_request(int64_t from_user, int64_t to_user, bool accept) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql =
        "UPDATE friend_relationships "
        "SET status = $3, responded_at = NOW() "
        "WHERE user_id = $1 AND peer_user_id = $2 AND status = 'PENDING';";

    const char *params[3];
    char buf1[32], buf2[32];
    snprintf(buf1, sizeof(buf1), "%ld", from_user);
    snprintf(buf2, sizeof(buf2), "%ld", to_user);
    params[0] = buf1;
    params[1] = buf2;
    params[2] = accept ? "ACCEPTED" : "DECLINED";

    PGresult *res = PQexecParams(conn, sql, 3, NULL, params, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "[DAO_FRIENDS] respond_request error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }
    int affected = atoi(PQcmdTuples(res));
    PQclear(res);
    return (affected > 0) ? 0 : -1;
}

// Ở đây mình giả sử bạn dùng JSON/Jansson phía service.
// DAO có thể trả về raw PGresult, nhưng để đơn giản ta trả về void* chứa JSON string.
int dao_friends_list(int64_t user_id, void **result_json) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql =
        "SELECT u.user_id, u.username, f.status "
        "FROM friend_relationships f "
        "JOIN users u ON u.user_id = f.peer_user_id "
        "WHERE f.user_id = $1 AND f.status = 'ACCEPTED';";

    char buf[32];
    snprintf(buf, sizeof(buf), "%ld", user_id);
    const char *params[1] = { buf };

    PGresult *res = PQexecParams(conn, sql, 1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "[DAO_FRIENDS] list error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    int rows = PQntuples(res);
    // Build JSON array
    size_t cap = 256;
    size_t used = 0;
    char *out = malloc(cap);
    if (!out) { PQclear(res); return -1; }
    out[used++] = '[';

    for (int i = 0; i < rows; ++i) {
        const char *uid = PQgetvalue(res, i, 0);
        const char *uname = PQgetvalue(res, i, 1);
        const char *status = PQgetvalue(res, i, 2);

        char *esc_name = util_json_escape(uname);
        if (!esc_name) esc_name = strdup("");

        // estimate needed
        int need = snprintf(NULL, 0, "{\"user_id\": %s, \"username\": \"%s\", \"status\": \"%s\"}", uid, esc_name, status);
        if (used + (size_t)need + 3 >= cap) {
            cap = (used + (size_t)need + 3) * 2;
            char *tmp = realloc(out, cap);
            if (!tmp) { free(out); free(esc_name); PQclear(res); return -1; }
            out = tmp;
        }

        if (i > 0) out[used++] = ',';
        used += snprintf(out + used, cap - used, "{\"user_id\": %s, \"username\": \"%s\", \"status\": \"%s\"}", uid, esc_name, status);

        free(esc_name);
    }

    if (used + 2 >= cap) {
        char *tmp = realloc(out, used + 2);
        if (!tmp) { free(out); PQclear(res); return -1; }
        out = tmp;
        cap = used + 2;
    }
    out[used++] = ']';
    out[used] = '\0';

    *result_json = out;
    PQclear(res);
    return 0;
}

int dao_friends_update_status(int64_t user_id, int64_t friend_id, friend_status_t status) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql =
        "UPDATE friend_relationships SET status = $3 "
        "WHERE user_id = $1 AND peer_user_id = $2;";

    char buf1[32], buf2[32];
    snprintf(buf1, sizeof(buf1), "%ld", user_id);
    snprintf(buf2, sizeof(buf2), "%ld", friend_id);

    const char *params[3] = { buf1, buf2, friend_status_to_str(status) };

    PGresult *res = PQexecParams(conn, sql, 3, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "[DAO_FRIENDS] update_status error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }
    PQclear(res);
    return 0;
}

int dao_friends_get_info(int64_t user_id, int64_t friend_id, void **result_json) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;
    
    const char *sql =
        "SELECT u.user_id, u.username, COALESCE(u.avatar_img, '') as avatar_img, f.status "
        "FROM friend_relationships f "
        "JOIN users u ON u.user_id = f.peer_user_id "
        "WHERE f.user_id = $1 AND f.peer_user_id = $2 AND f.status = 'ACCEPTED';";
    
    char buf1[32], buf2[32];
    snprintf(buf1, sizeof(buf1), "%ld", user_id);
    snprintf(buf2, sizeof(buf2), "%ld", friend_id);
    const char *params[2] = { buf1, buf2 };
    
    PGresult *res = PQexecParams(conn, sql, 2, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "[DAO_FRIENDS] get_info error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }
    
    if (PQntuples(res) == 0) {
        PQclear(res);
        return -1;
    }
    
    const char *uid = PQgetvalue(res, 0, 0);
    const char *uname = PQgetvalue(res, 0, 1);
    const char *avatar = PQgetvalue(res, 0, 2);
    const char *status = PQgetvalue(res, 0, 3);
    
    char *esc_name = util_json_escape(uname);
    char *esc_avatar = util_json_escape(avatar);
    if (!esc_name) esc_name = strdup("");
    if (!esc_avatar) esc_avatar = strdup("");
    
    char *out = malloc(512);
    if (!out) { free(esc_name); free(esc_avatar); PQclear(res); return -1; }
    
    snprintf(out, 512, "{\"user_id\": %s, \"username\": \"%s\", \"avatar_img\": \"%s\", \"status\": \"%s\"}", 
             uid, esc_name, esc_avatar, status);
    
    free(esc_name);
    free(esc_avatar);
    *result_json = out;
    PQclear(res);
    return 0;
}

int dao_friends_get_pending_requests(int64_t user_id, void **result_json) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;
    
    const char *sql =
        "SELECT u.user_id, u.username, COALESCE(u.avatar_img, '') as avatar_img "
        "FROM friend_relationships f "
        "JOIN users u ON u.user_id = f.user_id "
        "WHERE f.peer_user_id = $1 AND f.status = 'PENDING';";
    
    char buf[32];
    snprintf(buf, sizeof(buf), "%ld", user_id);
    const char *params[1] = { buf };
    
    PGresult *res = PQexecParams(conn, sql, 1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "[DAO_FRIENDS] get_pending_requests error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }
    
    int rows = PQntuples(res);
    size_t cap = 512;
    size_t used = 0;
    char *out = malloc(cap);
    if (!out) { PQclear(res); return -1; }
    out[used++] = '[';
    
    for (int i = 0; i < rows; i++) {
        const char *uid = PQgetvalue(res, i, 0);
        const char *uname = PQgetvalue(res, i, 1);
        const char *avatar = PQgetvalue(res, i, 2);
        
        char *esc_name = util_json_escape(uname);
        char *esc_avatar = util_json_escape(avatar);
        if (!esc_name) esc_name = strdup("");
        if (!esc_avatar) esc_avatar = strdup("");
        
        int need = snprintf(NULL, 0, "{\"user_id\": %s, \"username\": \"%s\", \"avatar_img\": \"%s\"}", 
                          uid, esc_name, esc_avatar);
        if (used + (size_t)need + 3 >= cap) {
            cap = (used + (size_t)need + 3) * 2;
            char *tmp = realloc(out, cap);
            if (!tmp) { free(out); free(esc_name); free(esc_avatar); PQclear(res); return -1; }
            out = tmp;
        }
        
        if (i > 0) out[used++] = ',';
        used += snprintf(out + used, cap - used, 
                        "{\"user_id\": %s, \"username\": \"%s\", \"avatar_img\": \"%s\"}", 
                        uid, esc_name, esc_avatar);
        
        free(esc_name);
        free(esc_avatar);
    }
    
    if (used + 2 >= cap) {
        char *tmp = realloc(out, used + 2);
        if (!tmp) { free(out); PQclear(res); return -1; }
        out = tmp;
    }
    out[used++] = ']';
    out[used] = '\0';
    
    *result_json = out;
    PQclear(res);
    return 0;
}

int dao_friends_are_friends(int64_t user_id1, int64_t user_id2, bool *are_friends) {
    PGconn *conn = db_get_conn();
    if (!conn || !are_friends) return -1;
    
    const char *sql =
        "SELECT COUNT(*) FROM friend_relationships "
        "WHERE ((user_id = $1 AND peer_user_id = $2) OR (user_id = $2 AND peer_user_id = $1)) "
        "AND status = 'ACCEPTED';";
    
    char buf1[32], buf2[32];
    snprintf(buf1, sizeof(buf1), "%ld", user_id1);
    snprintf(buf2, sizeof(buf2), "%ld", user_id2);
    const char *params[2] = { buf1, buf2 };
    
    PGresult *res = PQexecParams(conn, sql, 2, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "[DAO_FRIENDS] are_friends error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }
    
    int count = atoi(PQgetvalue(res, 0, 0));
    *are_friends = (count > 0);
    
    PQclear(res);
    return 0;
}

int dao_friends_remove(int64_t user_id, int64_t friend_id) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;
    
    // Delete both directions of the friendship relationship
    const char *sql =
        "DELETE FROM friend_relationships "
        "WHERE (user_id = $1 AND peer_user_id = $2) OR (user_id = $2 AND peer_user_id = $1);";
    
    char buf1[32], buf2[32];
    snprintf(buf1, sizeof(buf1), "%ld", user_id);
    snprintf(buf2, sizeof(buf2), "%ld", friend_id);
    const char *params[2] = { buf1, buf2 };
    
    PGresult *res = PQexecParams(conn, sql, 2, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "[DAO_FRIENDS] remove error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }
    PQclear(res);
    return 0;
}