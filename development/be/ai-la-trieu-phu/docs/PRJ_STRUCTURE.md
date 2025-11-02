# Cấu trúc dự án Ai-la-triệu-phú

Tài liệu này mô tả mục đích của từng thư mục/tệp và liên hệ với các tiêu chí chấm điểm trong môn IT4062_161265.

## Thư mục gốc
- `README.md`: Giới thiệu dự án, hướng dẫn nhanh và thông tin tác giả (Tiêu chí: Hồ sơ dự án đầy đủ).

## server-c
- `Makefile`: Điểm khởi đầu cho quy trình build tự động (Tiêu chí: Tự động hóa biên dịch).
- `config.example.toml`: Mẫu cấu hình runtime của server (Tiêu chí: Khả năng cấu hình linh hoạt).
- `include/`: Nơi tập trung header để tái sử dụng giữa các mô-đun (Tiêu chí: Thiết kế module hóa).
- `src/`: Mã nguồn chính của server (Tiêu chí: Lập trình socket TCP và xử lý đồng thời).
  - `main.c`: Hàm `main` khởi tạo server, cấu hình và vào vòng lặp sự kiện.
  - `net/`: Lớp giao tiếp mạng với client.
    - `server.c`: Quản lý socket, `select()`/`epoll()` và phân phối sự kiện (Tiêu chí: Xử lý nhiều kết nối).
    - `protocol.c`: Định nghĩa quy tắc đóng gói/tháo gỡ thông điệp JSON (Tiêu chí: Thiết kế giao thức ứng dụng).
  - `auth/` với `auth.c`: Quản lý đăng nhập, đăng ký và phiên (Tiêu chí: An toàn truy cập và xác thực).
  - `room/` với `room.c`: Điều phối phòng chơi và lời mời (Tiêu chí: Quản lý phiên trò chơi nhiều người).
  - `game/`: Logic trò chơi.
    - `game.c`: Luồng chơi chính và trạng thái tiến trình.
    - `quiz.c`: Nạp câu hỏi, chấm điểm.
    - `state.c`: Lưu trạng thái và bộ đếm thời gian (Tiêu chí: Quản lý trạng thái và thời gian thực).
  - `store/`: Lớp truy xuất dữ liệu.
    - `users.c`: Dữ liệu tài khoản và mật khẩu.
    - `questions.c`: Bộ câu hỏi.
    - `matches.c`: Nhật ký trận và replay (Tiêu chí: Lưu vết và thống kê).
  - `utils/`: Công cụ chia sẻ.
    - `log.c`: Ghi log hệ thống (Tiêu chí: Theo dõi hoạt động).
    - `timer.c`: Hỗ trợ bộ đếm thời gian.
    - `common.h`: Định nghĩa chung để giảm lặp.
  - `config/` với `config.c`: Đọc cấu hình và kiểm tra hợp lệ.
  - `extra/` với `chat.c`: Nơi bổ sung tính năng nâng cao như chat (Tiêu chí cộng thêm).

## clients
- `cli-python/`, `cli-c/`, `web-react/`: Khung client mẫu cho các ngôn ngữ/giao diện khác nhau (Tiêu chí: Khả năng tương thích đa nền tảng). Mỗi thư mục chứa file mô tả hoặc điểm vào mẫu.

## scripts
- `test_clients.py`: Tự động hóa kiểm thử tải (Tiêu chí: Kiểm thử và đánh giá hiệu năng).
- `generate_questions.py`: Công cụ chuẩn bị dữ liệu (Tiêu chí: Hỗ trợ vận hành).
- `README.md`: Hướng dẫn cách sử dụng các script.

## assets
- `questions.csv`: Bộ dữ liệu câu hỏi mẫu (Tiêu chí: Chuẩn bị tài nguyên nội dung).

## data
- `users.db`: Kho lưu tài khoản dựa trên SQLite hoặc tệp tương tự.
- `matches/`, `replays/`: Lưu nhật ký và dữ liệu phát lại (Tiêu chí: Khả năng lưu trữ và phân tích trận).

## docs
- `API.md`: Mô tả giao thức và các endpoint giao tiếp giữa client và server (Tiêu chí: Đặc tả giao thức).
- `DESIGN.md`: Tài liệu kiến trúc và sơ đồ (Tiêu chí: Thiết kế hệ thống rõ ràng).
- `PRJ_STRUCTURE.md`: Chính tài liệu này, dùng cho báo cáo cấu trúc dự án.
