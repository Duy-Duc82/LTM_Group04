# Trò chơi “Ai là triệu phú” trực tuyến — README

> Môn học: Thực hành lập trình mạng (IT4062_161265)  
> Đề tài: Trò chơi “Ai là triệu phú” trực tuyến  
> Server: **C (sockets)** · Client: **tùy chọn ngôn ngữ** (C/CPP, Python, JavaScript, v.v.)

## 1) Giới thiệu
Dự án xây dựng một hệ thống đố vui “Ai là triệu phú” theo mô hình **client–server**, cho phép người chơi đăng ký/đăng nhập, tạo hoặc tham gia phòng chơi trực tuyến, thi đấu trả lời câu hỏi theo thời gian, và xem kết quả/bảng xếp hạng. Mục tiêu là thực hành **lập trình mạng** với socket, quản lý kết nối đồng thời, thiết kế giao thức ứng dụng và xử lý trạng thái phiên chơi.

## 2) Tính năng
- **Quản lý người dùng**: Đăng ký, đăng nhập, hiển thị trạng thái trực tuyến.
- **Phòng chơi**: Tạo phòng, mời bạn, tham gia/thoát, chủ phòng bắt đầu trận.
- **Chế độ chơi**: Loại trực tiếp hoặc tính điểm; thời gian trả lời giới hạn.
- **Thống kê**: Điểm số, tỷ lệ đúng, **leaderboard**.
- **UI/UX**: Giao diện mô phỏng phong cách “Ai là triệu phú” (phần client tùy chọn công nghệ).
- **Mở rộng**: Chat trong phòng, bộ câu hỏi ngẫu nhiên, bot luyện tập.

## 3) Kiến trúc tổng quan
```
┌───────────┐        TCP/UDP (tuỳ chọn, khuyến nghị TCP)
│  Clients  │ <───────────────────────────────────────────► │  Server C  │
└───────────┘                                              └─────────────┘
      ▲                           CSDL (tùy chọn: MySQL/MongoDB)
      └─────────────►  REST-ish / JSON over TCP  ◄───────────────┘
```
- **Server (C)**: Quản lý kết nối, xác thực, tạo phòng/ghép trận, phát câu hỏi, chấm điểm, đồng bộ trạng thái; có thể sử dụng **epoll/kqueue/select/poll**.
- **Client (tuỳ chọn)**: CLI/GUI/Web. Ví dụ: 
  - CLI (Python/Java/C) để test nhanh giao thức.
  - Web (React + WebSocket proxy) nếu muốn giao diện rich UI.
- **Giao thức ứng dụng**: **JSON over TCP** (khuyến nghị). Mỗi thông điệp kết thúc bằng ký tự xuống dòng `\n` hoặc **prefixed-length**.

## 4) Luồng trò chơi cơ bản
1. Client `LOGIN` → Server xác thực → trả `LOGIN_OK`.
2. Client `CREATE_ROOM`/`JOIN_ROOM` → Server trả `ROOM_JOINED`.
3. Chủ phòng `START_GAME` → Server phát `QUESTION` (nội dung, 4 lựa chọn, thời gian).
4. Mỗi client gửi `ANSWER` (A/B/C/D) trong thời gian quy định.
5. Server chấm điểm, gửi `ROUND_RESULT` và/hoặc `ELIMINATED`.
6. Hết n vòng → Server gửi `GAME_RESULT` + `LEADERBOARD`.

## 5) Giao thức (đề xuất)
Tất cả đều là JSON một dòng (line-delimited).

**Đăng ký**  
```json
{"type":"REGISTER","username":"duy","password":"***"}
```
**Đăng nhập**  
```json
{"type":"LOGIN","username":"duy","password":"***"}
```
**Tạo/Tham gia phòng**  
```json
{"type":"CREATE_ROOM","mode":"POINTS","limit":4}
{"type":"JOIN_ROOM","room_id":"RM1234"}
{"type":"INVITE","room_id":"RM1234","friend":"hung"}
```
**Bắt đầu & câu hỏi**  
```json
{"type":"START_GAME","room_id":"RM1234"}
{"type":"QUESTION","qid":"Q001","content":"Thủ đô Pháp là?","choices":["Paris","Lyon","Nice","Lille"],"timeout":20,"round":1}
```
**Trả lời & chấm điểm**  
```json
{"type":"ANSWER","qid":"Q001","answer":"A","time_ms":5423}
{"type":"ROUND_RESULT","qid":"Q001","correct":"A","earned":10,"total":10}
```
**Kết thúc & bảng xếp hạng**  
```json
{"type":"GAME_RESULT","summary":[{"user":"duy","score":70},{"user":"hung","score":60}]}
{"type":"LEADERBOARD","items":[{"user":"duy","correct_rate":0.7,"score":70}]}
```

> **Gợi ý hiện thực:** sử dụng `
` làm delimiter; nếu cần nhị phân, dùng `uint32_t length-prefix` + payload JSON để tránh split packet.

## 6) Cấu trúc thư mục (đề xuất)
```
ai-la-trieu-phu/
├─ server-c/
│  ├─ src/
│  │  ├─ main.c
│  │  ├─ net/
│  │  │  ├─ server.c        # tạo socket, accept, event-loop (select/epoll)
│  │  │  └─ protocol.c      # parse/compose JSON, framing
│  │  ├─ game/
│  │  │  ├─ room.c          # quản lý phòng, ghép người chơi
│  │  │  ├─ quiz.c          # nạp & phát câu hỏi, chấm điểm
│  │  │  └─ state.c         # trạng thái phiên, đồng hồ đếm
│  │  └─ store/
│  │     ├─ users.c         # đăng ký/đăng nhập, băm mật khẩu
│  │     └─ db_mysql.c      # (tùy chọn) MySQL DAO
│  ├─ include/
│  ├─ CMakeLists.txt / Makefile
│  └─ config.example.toml
├─ clients/
│  ├─ cli-python/
│  ├─ cli-java/
│  └─ web-react/
├─ assets/                   # câu hỏi, ảnh, âm thanh
├─ docs/
│  ├─ API.md
│  └─ DESIGN.md
└─ README.md
```

## 7) Yêu cầu hệ thống
- **Server**: GCC/Clang, CMake/Make, Linux (khuyến nghị) / Windows (WSL/MinGW).  
- **Thư viện gợi ý**:  
  - JSON: cJSON / Jansson.  
  - Bảo mật: OpenSSL (nếu dùng TLS).  
  - DB (tùy chọn): libmysqlclient hoặc mongoc.
- **Client**: tuỳ ngôn ngữ (Python `socket`, Java `nio`, Node.js `net`/WebSocket…).

## 8) Build & Run (Server C)
### Linux/macOS
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y build-essential cmake libssl-dev libmysqlclient-dev

git clone <repo-url> ai-la-trieu-phu
cd ai-la-trieu-phu/server-c
cmake -S . -B build -DDB_MYSQL=ON -DUSE_OPENSSL=OFF
cmake --build build -j

./build/altp_server --host 0.0.0.0 --port 5050 --config ../config.example.toml
```
### Windows (MSYS2/MinGW)
```bash
pacman -S --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake
cd server-c
cmake -G "MinGW Makefiles" -S . -B build
cmake --build build -j
build\altp_server.exe --host 0.0.0.0 --port 5050
```

## 9) Thiết kế câu hỏi & CSDL (tuỳ chọn)
**Bảng `users`**
```
id (PK), username (unique), pass_hash, created_at, last_login
```
**Bảng `questions`**
```
id (PK), content, choice_a, choice_b, choice_c, choice_d, correct ('A'..'D'), category, difficulty
```
**Bảng `matches`, `match_players`, `answers`** để lưu lịch sử, tính tỷ lệ đúng, leaderboard.

> Có thể nạp câu hỏi từ `assets/questions.csv` → cache trên RAM khi khởi động.

## 10) Bảo mật & An toàn
- Hash mật khẩu (Argon2/bcrypt/scrypt). Không lưu plain text.
- Chống flood: giới hạn tốc độ (rate limit) theo kết nối/IP.
- Phòng ngừa “double answer” trong 1 vòng; đóng nhận bài khi hết thời gian.
- (Tuỳ chọn) **TLS**: `STARTTLS` hoặc TCP+TLS ngay từ đầu. 
- Sanitize đầu vào, kiểm tra framing chặt chẽ tránh “packet smuggling”.

## 11) Kiểm thử
- **Unit test**: parser JSON, chấm điểm, quản lý phòng.
- **Integration**: nhiều client ảo gửi/nhận theo kịch bản.
- **Load test**: 100–500 kết nối đồng thời, đo độ trễ phát câu hỏi & thu bài.

## 12) Roadmap
- [ ] Chat trong phòng, gửi emoji.
- [ ] Bộ câu hỏi theo chủ đề/độ khó, chế độ luyện tập offline.
- [ ] Web client (React) + hiệu ứng “Ai là triệu phú”.
- [ ] Admin tool nhập/xuất câu hỏi, phát hiện câu hỏi trùng lặp.
- [ ] Docker hoá server & DB.

## 13) Thành viên
- Dương Đức Duy — 20225828  
- Phan Sỹ Hùng — 20225631  
- Lục Minh Đức — 20225810

## 14) Giấy phép
Chọn một giấy phép phù hợp (ví dụ **MIT License**) cho mã nguồn.

---

### Phụ lục A — Tham số dòng lệnh server (ví dụ)
```
--host 0.0.0.0
--port 5050
--db "mysql://user:pass@localhost:3306/altp"
--questions ./assets/questions.csv
--tls-cert ./certs/server.crt
--tls-key  ./certs/server.key
--room-timeout-sec 30
--answer-timeout-sec 20
```
