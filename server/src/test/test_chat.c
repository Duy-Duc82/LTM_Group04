#include <stdio.h>
#include "db.h"
#include "dao/dao_chat.h"

int main(void) {
    if (db_init() != 0) {
        fprintf(stderr, "DB init failed\n");
        return 1;
    }

    if (dao_chat_send_dm(1, 2, "Hello from test_chat") == 0) {
        printf("Send DM OK\n");
    } else {
        printf("Send DM FAILED\n");
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
