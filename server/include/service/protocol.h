// header + mã lệnh + JSON structure
#include <stdint.h>


typedef struct {
    uint16_t cmd;        // CommandType: 0x0101, 0x0103, ...
    uint16_t user_id;    // 0 nếu chưa login
    uint32_t length;     // độ dài payload (bytes)
} PacketHeader;

typedef struct {
    PacketHeader header;
    char payload[];      // dữ liệu theo định dạng JSON
} Packet;


