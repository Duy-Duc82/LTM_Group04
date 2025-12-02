#include <stdio.h>
#include "db.h"
#include "dao/dao_rooms.h"

int main(void) {
    if (db_init() != 0) {
        fprintf(stderr, "DB init failed\n");
        return 1;
    }

    int64_t room_id;
    if (dao_rooms_create(1, ROOM_MODE_ONEVN, &room_id) == 0) {
        printf("Create room OK, id = %ld\n", room_id);
    } else {
        printf("Create room FAILED\n");
    }

    if (dao_rooms_join(room_id, 2, 0) == 0) {
        printf("User 2 joined room\n");
    }

    db_close();
    return 0;
}
