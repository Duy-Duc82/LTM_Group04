#include <stdio.h>
#include "db.h"
#include "dao/dao_chat.h"
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
                printf("Receiver logged in: user_id=%lld token=%s\n", (long long)recv_sess.user_id, recv_sess.access_token);
            } else {
                printf("Receiver login failed (code=%d) — proceeding as offline recipient id=2\n", ar);
            }
        }
    }

    // Interactive: read message content from stdin and send as DM from user 1 -> user 2
    char buf[1024];
    while (1) {
        printf("Type message to send from 1->2 (q to quit): ");
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
        if (dao_chat_send_dm(sender_id, receiver_id, buf) == 0) {
            printf("Send DM OK\n");
        } else {
            printf("Send DM FAILED\n");
        }
        // If receiver is logged in, display message for receiver side
        if (receiver_logged_in) {
            printf("[RECEIVER VIEW] %lld received: %s\n", (long long)receiver_id, buf);
            // show any offline messages for the recipient as well
            void *json_msgs2 = NULL;
            if (dao_chat_fetch_offline(receiver_id, &json_msgs2) == 0) {
                printf("Receiver offline messages: %s\n", (char *)json_msgs2);
                free(json_msgs2);
            }
        }
    }

    void *json_msgs = NULL;
    if (dao_chat_fetch_offline(2, &json_msgs) == 0) {
        printf("Offline messages: %s\n", (char *)json_msgs);
        free(json_msgs);
    } else {
        printf("fetch_offline FAILED\n");
    }

    db_close();
    return 0;
}
