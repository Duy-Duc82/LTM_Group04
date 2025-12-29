#include <stdio.h>
#include <stdlib.h>
#include "db.h"
#include "dao/dao_users.h"
#include "dao/dao_friends.h"

int main(void) {
    if (db_init() != 0) {
        fprintf(stderr, "DB init failed\n");
        return 1;
    }

    int64_t u1 = 1; // giả sử đã có user_id 1, 2
    int64_t u2 = 2;

    if (dao_friends_send_request(u1, u2) == 0) {
        printf("Send friend request OK\n");
    } else {
        printf("Send friend request FAILED\n");
    }

    if (dao_friends_respond_request(u1, u2, 1) == 0) {
        printf("Accept friend OK\n");
    } else {
        printf("Accept friend FAILED\n");
    }

    void *json_list = NULL;
    if (dao_friends_list(u1, &json_list) == 0) {
        printf("Friends list: %s\n", (char *)json_list);
        free(json_list);
    } else {
        printf("Friends list FAILED\n");
    }

    db_close();
    return 0;
}
