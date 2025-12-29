#include <stdio.h>
#include "db.h"
#include "dao/dao_chat.h"
#include "utils/json.h"
#include "dao/dao_users.h"
#include "service/auth_service.h"
#include <stdlib.h>
#include <string.h>


int main(void) {
    if (db_init() != 0) {
        fprintf(stderr, "DB init failed\n");
        return 1;
    }

    // Optionally login as receiver so we can display messages on the receiver side
    int64_t sender_id = 1;
    int64_t receiver_id = 2;
    char sender_name[64] = "user1";
    char receiver_name[64] = "user2";

    // try to resolve names
    User u;
    if (dao_users_find_by_id(sender_id, &u) == 0) {
        strncpy(sender_name, u.username, sizeof(sender_name)-1);
    }
    if (dao_users_find_by_id(receiver_id, &u) == 0) {
        strncpy(receiver_name, u.username, sizeof(receiver_name)-1);
    }
    UserSession recv_sess;
    int receiver_logged_in = 0;

    printf("Log in as receiver? (y/n): ");
    char yn[8];
    if (fgets(yn, sizeof(yn), stdin)) {
        if (yn[0] == 'y' || yn[0] == 'Y') {
            char uname[128];
            char pwd[128];
            printf("Receiver username: ");
            if (fgets(uname, sizeof(uname), stdin)) {
                if (uname[strlen(uname)-1] == '\n') uname[strlen(uname)-1] = '\0';
            }
            printf("Receiver password: ");
            if (fgets(pwd, sizeof(pwd), stdin)) {
                if (pwd[strlen(pwd)-1] == '\n') pwd[strlen(pwd)-1] = '\0';
            }
            AuthResult ar = auth_login(uname, pwd, &recv_sess);
            if (ar == AUTH_OK) {
                receiver_logged_in = 1;
                receiver_id = recv_sess.user_id;
                // we logged in with username from input; record as receiver_name
                // uname variable still exists in the outer scope? ensure we kept it
                // We'll capture it above in uname variable
                // find canonical username from DB for display
                if (dao_users_find_by_id(receiver_id, &u) == 0) strncpy(receiver_name, u.username, sizeof(receiver_name)-1);
                printf("Receiver logged in: user_id=%lld token=%s\n", (long long)recv_sess.user_id, recv_sess.access_token);
                // show any pending messages from sender to receiver
                void *msgs_from_sender = NULL;
                if (dao_chat_fetch_offline_from_sender(receiver_id, sender_id, &msgs_from_sender) == 0) {
                    if (msgs_from_sender) {
                        // print only the content fields from returned JSON
                        const char *p = (const char *)msgs_from_sender;
                        const char *key;
                        while ((key = strstr(p, "\"content\"")) != NULL) {
                            char *c = util_json_get_string(key, "content");
                            if (!c) break;
                            printf("[RECEIVER OFFLINE FROM %s] %s\n", sender_name, c);
                            free(c);
                            p = key + strlen("\"content\"");
                        }
                        free(msgs_from_sender);
                    }
                }
            } else {
                printf("Receiver login failed (code=%d) — proceeding as offline recipient id=2\n", ar);
            }
        }
    }

    // Interactive: read message content from stdin and send as DM from user 1 -> user 2
    char buf[1024];
    while (1) {
        // decide who is currently sending: normally sender_id -> receiver_id,
        // but if the receiver logged into this terminal, flip roles so the logged-in
        // user is the sender (can reply)
        int64_t from_id = sender_id;
        int64_t to_id = receiver_id;
        const char *from_name = sender_name;
        const char *to_name = receiver_name;
        if (receiver_logged_in) {
            from_id = receiver_id;
            to_id = sender_id;
            from_name = receiver_name;
            to_name = sender_name;
        }

        printf("Type message to send from %s(%lld) -> %s(%lld) (q to quit): ", from_name, (long long)from_id, to_name, (long long)to_id);
        if (!fgets(buf, sizeof(buf), stdin)) break;
        // strip newline
        size_t len = strlen(buf);
        if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';
        if (strcmp(buf, "q") == 0 || strcmp(buf, "Q") == 0) {
            printf("Quit sending messages\n");
            break;
        }
        if (strlen(buf) == 0) {
            printf("Empty message — not sent\n");
            continue;
        }
        if (dao_chat_send_dm(from_id, to_id, buf) == 0) {
            printf("Send DM OK\n");
        } else {
            printf("Send DM FAILED\n");
        }
        // Show the result as recipient view (the recipient is 'to_id')
        printf("[%s RECEIVED] %s(%lld) -> %s(%lld): %s\n",
               "RECEIVED",
               from_name, (long long)from_id, to_name, (long long)to_id, buf);

        // If the current terminal is logged-in receiver and they were sending (roles flipped),
        // also show any pending messages for the receiver (logged-in user) from the other party.
        if (receiver_logged_in) {
            // receiver is from_id — show any offline messages for receiver FROM the original sender
            void *json_msgs2 = NULL;
            if (dao_chat_fetch_offline_from_sender(receiver_id, sender_id, &json_msgs2) == 0) {
                if (json_msgs2) {
                    const char *p2 = (const char *)json_msgs2;
                    const char *key2;
                    while ((key2 = strstr(p2, "\"content\"")) != NULL) {
                        char *c2 = util_json_get_string(key2, "content");
                        if (!c2) break;
                        printf("[RECEIVER OFFLINE FROM %s] %s\n", sender_name, c2);
                        free(c2);
                        p2 = key2 + strlen("\"content\"");
                    }
                }
                free(json_msgs2);
            }
        }
    }

    void *json_msgs = NULL;
    if (dao_chat_fetch_offline(receiver_id, &json_msgs) == 0) {
        if (json_msgs) {
            const char *p3 = (const char *)json_msgs;
            const char *key3;
            while ((key3 = strstr(p3, "\"content\"")) != NULL) {
                char *c3 = util_json_get_string(key3, "content");
                if (!c3) break;
                printf("[OFFLINE] %s\n", c3);
                free(c3);
                p3 = key3 + strlen("\"content\"");
            }
        }
        free(json_msgs);
    } else {
        printf("fetch_offline FAILED\n");
    }

    db_close();
    return 0;
}
