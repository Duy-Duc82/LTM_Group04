-- =========================================================
-- BỔ SUNG DỮ LIỆU CHO BẢNG QUESTION
-- File: insert_questions.sql
-- =========================================================

INSERT INTO question (difficulty_level, content, "opA", "opB", "opC", "opD", correct_op, explanation)
VALUES

-- =========================
-- EASY QUESTIONS (20 câu)
-- =========================

('EASY', '2 + 2 = ?', '3', '4', '5', '6', 'B', '2 + 2 = 4'),
('EASY', 'Màu của lá cây thường là gì?', 'Đỏ', 'Xanh', 'Vàng', 'Trắng', 'B', 'Lá cây có màu xanh do chứa chất diệp lục.'),
('EASY', 'Con vật nào được gọi là "Chúa tể rừng xanh"?', 'Hổ', 'Sư tử', 'Báo', 'Gấu', 'B', 'Sư tử được mệnh danh là "Chúa tể rừng xanh".'),
('EASY', 'Ngày Quốc khánh Việt Nam là ngày nào?', '1/1', '30/4', '2/9', '20/10', 'C', 'Ngày Quốc khánh Việt Nam là 2/9/1945.'),
('EASY', 'Số nguyên tố nhỏ nhất là?', '0', '1', '2', '3', 'C', 'Số nguyên tố nhỏ nhất là 2.'),
('EASY', 'Hành tinh nào gần Mặt Trời nhất?', 'Sao Kim', 'Sao Thủy', 'Trái Đất', 'Sao Hỏa', 'B', 'Sao Thủy là hành tinh gần Mặt Trời nhất.'),
('EASY', 'Ai là tác giả của "Truyện Kiều"?', 'Nguyễn Du', 'Hồ Xuân Hương', 'Nguyễn Trãi', 'Lý Bạch', 'A', 'Nguyễn Du là tác giả của "Truyện Kiều".'),
('EASY', 'Nước nào có diện tích lớn nhất thế giới?', 'Trung Quốc', 'Mỹ', 'Nga', 'Canada', 'C', 'Nga có diện tích lớn nhất thế giới.'),
('EASY', 'Đơn vị đo nhiệt độ phổ biến nhất là?', 'Celsius', 'Fahrenheit', 'Kelvin', 'Rankine', 'A', 'Celsius là đơn vị đo nhiệt độ phổ biến nhất.'),
('EASY', 'Thành phố nào được gọi là "Thành phố không ngủ"?', 'Paris', 'New York', 'Tokyo', 'London', 'B', 'New York được gọi là "Thành phố không ngủ".'),
('EASY', 'Số Pi (π) xấp xỉ bằng?', '3.14', '2.71', '1.41', '1.73', 'A', 'Số Pi (π) xấp xỉ bằng 3.14.'),
('EASY', 'Loài động vật nào lớn nhất trên cạn?', 'Voi', 'Hươu cao cổ', 'Hà mã', 'Tê giác', 'A', 'Voi là loài động vật lớn nhất trên cạn.'),
('EASY', 'Năm 2024 là năm con gì theo lịch âm?', 'Mèo', 'Rồng', 'Rắn', 'Ngựa', 'B', 'Năm 2024 là năm Giáp Thìn (Rồng).'),
('EASY', 'Ai phát minh ra bóng đèn điện?', 'Edison', 'Tesla', 'Newton', 'Einstein', 'A', 'Thomas Edison phát minh ra bóng đèn điện.'),
('EASY', 'Thủ đô của Pháp là thành phố nào?', 'Lyon', 'Paris', 'Marseille', 'Nice', 'B', 'Thủ đô của Pháp là Paris.'),
('EASY', 'Sông nào dài nhất thế giới?', 'Sông Amazon', 'Sông Nile', 'Sông Mississippi', 'Sông Dương Tử', 'B', 'Sông Nile là sông dài nhất thế giới.'),
('EASY', 'Ngày Quốc tế Phụ nữ là ngày nào?', '8/3', '20/10', '14/2', '1/5', 'A', 'Ngày Quốc tế Phụ nữ là 8/3.'),
('EASY', 'Con vật nào là biểu tượng của Australia?', 'Kangaroo', 'Koala', 'Đà điểu', 'Gấu túi', 'A', 'Kangaroo là biểu tượng của Australia.'),
('EASY', 'Môn thể thao nào được gọi là "môn thể thao vua"?', 'Bóng đá', 'Bóng rổ', 'Tennis', 'Bơi lội', 'A', 'Bóng đá được gọi là "môn thể thao vua".'),
('EASY', 'Ai là người đầu tiên đặt chân lên Mặt Trăng?', 'Neil Armstrong', 'Buzz Aldrin', 'Yuri Gagarin', 'Alan Shepard', 'A', 'Neil Armstrong là người đầu tiên đặt chân lên Mặt Trăng.'),

-- =========================
-- MEDIUM QUESTIONS (20 câu)
-- =========================

('MEDIUM', 'Trong lập trình, từ khóa nào dùng để khai báo biến không đổi trong C?', 'const', 'final', 'static', 'readonly', 'A', 'Từ khóa "const" dùng để khai báo biến không đổi trong C.'),
('MEDIUM', 'HTTP status code 404 có nghĩa là gì?', 'OK', 'Not Found', 'Server Error', 'Forbidden', 'B', 'HTTP 404 có nghĩa là "Not Found" - không tìm thấy tài nguyên.'),
('MEDIUM', 'Trong Git, lệnh nào dùng để xem lịch sử commit?', 'git log', 'git history', 'git show', 'git list', 'A', 'Lệnh "git log" dùng để xem lịch sử commit.'),
('MEDIUM', 'Port mặc định của HTTP là?', '80', '443', '8080', '21', 'A', 'Port mặc định của HTTP là 80.'),
('MEDIUM', 'Trong SQL, từ khóa nào dùng để sắp xếp kết quả?', 'ORDER BY', 'SORT BY', 'GROUP BY', 'ARRANGE BY', 'A', 'Từ khóa "ORDER BY" dùng để sắp xếp kết quả trong SQL.'),
('MEDIUM', 'Ngôn ngữ lập trình nào được dùng để phát triển Android apps?', 'Swift', 'Kotlin', 'Objective-C', 'C#', 'B', 'Kotlin là ngôn ngữ chính thức để phát triển Android apps.'),
('MEDIUM', 'Trong mạng máy tính, địa chỉ IP 127.0.0.1 có nghĩa là gì?', 'Broadcast', 'Localhost', 'Gateway', 'Subnet mask', 'B', '127.0.0.1 là địa chỉ localhost (máy tính hiện tại).'),
('MEDIUM', 'Trong JavaScript, phương thức nào dùng để chuyển string thành số?', 'parseInt()', 'toNumber()', 'convert()', 'stringToInt()', 'A', 'parseInt() dùng để chuyển string thành số nguyên trong JavaScript.'),
('MEDIUM', 'Trong Python, từ khóa nào dùng để định nghĩa hàm?', 'function', 'def', 'define', 'func', 'B', 'Từ khóa "def" dùng để định nghĩa hàm trong Python.'),
('MEDIUM', 'Trong HTML, thẻ nào dùng để tạo danh sách không có thứ tự?', '<ol>', '<ul>', '<li>', '<list>', 'B', 'Thẻ <ul> dùng để tạo danh sách không có thứ tự (unordered list).'),
('MEDIUM', 'Trong CSS, thuộc tính nào dùng để đặt màu nền?', 'color', 'background-color', 'bgcolor', 'background', 'B', 'Thuộc tính "background-color" dùng để đặt màu nền trong CSS.'),
('MEDIUM', 'Trong database, ACID là viết tắt của?', 'Atomicity, Consistency, Isolation, Durability', 'Access, Control, Integrity, Data', 'Analysis, Code, Input, Design', 'Application, Client, Interface, Database', 'A', 'ACID là viết tắt của Atomicity, Consistency, Isolation, Durability.'),
('MEDIUM', 'Trong lập trình hướng đối tượng, tính chất nào cho phép một class kế thừa từ class khác?', 'Encapsulation', 'Inheritance', 'Polymorphism', 'Abstraction', 'B', 'Inheritance (Kế thừa) cho phép một class kế thừa từ class khác.'),
('MEDIUM', 'Trong Linux, lệnh nào dùng để xem nội dung file?', 'cat', 'read', 'view', 'show', 'A', 'Lệnh "cat" dùng để xem nội dung file trong Linux.'),
('MEDIUM', 'Trong mạng, giao thức nào dùng để gửi email?', 'HTTP', 'FTP', 'SMTP', 'POP3', 'C', 'SMTP (Simple Mail Transfer Protocol) dùng để gửi email.'),
('MEDIUM', 'Trong JavaScript, toán tử nào dùng để so sánh giá trị và kiểu dữ liệu?', '==', '===', '=', '!=', 'B', 'Toán tử "===" so sánh cả giá trị và kiểu dữ liệu.'),
('MEDIUM', 'Trong SQL, từ khóa nào dùng để lọc dữ liệu?', 'FILTER', 'WHERE', 'SELECT', 'FROM', 'B', 'Từ khóa "WHERE" dùng để lọc dữ liệu trong SQL.'),
('MEDIUM', 'Trong HTML5, thẻ nào dùng để chèn video?', '<video>', '<media>', '<movie>', '<clip>', 'A', 'Thẻ <video> dùng để chèn video trong HTML5.'),
('MEDIUM', 'Trong Python, cấu trúc dữ liệu nào là immutable (không thể thay đổi)?', 'List', 'Dictionary', 'Tuple', 'Set', 'C', 'Tuple là cấu trúc dữ liệu immutable trong Python.'),
('MEDIUM', 'Trong mạng, địa chỉ MAC có độ dài bao nhiêu bit?', '32', '48', '64', '128', 'B', 'Địa chỉ MAC có độ dài 48 bit (6 bytes).'),

-- =========================
-- HARD QUESTIONS (20 câu)
-- =========================

('HARD', 'Trong cơ sở dữ liệu, thuật toán nào được dùng trong B-tree index?', 'Binary Search', 'Hash Table', 'B-tree Traversal', 'Linear Search', 'C', 'B-tree index sử dụng thuật toán B-tree Traversal để tìm kiếm.'),
('HARD', 'Trong lập trình, design pattern nào đảm bảo chỉ có một instance của class?', 'Factory', 'Singleton', 'Observer', 'Strategy', 'B', 'Singleton pattern đảm bảo chỉ có một instance của class.'),
('HARD', 'Trong mạng, giao thức nào sử dụng port 443?', 'HTTP', 'HTTPS', 'FTP', 'SSH', 'B', 'HTTPS sử dụng port 443.'),
('HARD', 'Trong database, transaction isolation level nào cho phép đọc dữ liệu chưa commit?', 'READ UNCOMMITTED', 'READ COMMITTED', 'REPEATABLE READ', 'SERIALIZABLE', 'A', 'READ UNCOMMITTED cho phép đọc dữ liệu chưa commit (dirty read).'),
('HARD', 'Trong lập trình, Big O notation O(n log n) thường xuất hiện trong thuật toán nào?', 'Bubble Sort', 'Quick Sort', 'Linear Search', 'Binary Search', 'B', 'Quick Sort có độ phức tạp O(n log n) trong trường hợp tốt nhất.'),
('HARD', 'Trong mạng, giao thức nào hoạt động ở tầng Network của mô hình OSI?', 'TCP', 'IP', 'HTTP', 'Ethernet', 'B', 'IP (Internet Protocol) hoạt động ở tầng Network (Layer 3).'),
('HARD', 'Trong database, index nào cho phép tìm kiếm nhanh nhưng không hỗ trợ range queries?', 'B-tree', 'Hash Index', 'Bitmap Index', 'Full-text Index', 'B', 'Hash Index nhanh nhưng chỉ hỗ trợ equality queries, không hỗ trợ range queries.'),
('HARD', 'Trong lập trình, từ khóa "volatile" trong C/C++ có tác dụng gì?', 'Tối ưu biến', 'Ngăn compiler tối ưu', 'Làm biến không đổi', 'Tăng tốc độ truy cập', 'B', 'Từ khóa "volatile" ngăn compiler tối ưu biến, đảm bảo đọc/ghi từ memory mỗi lần.'),
('HARD', 'Trong mạng, giao thức nào dùng để phân giải tên miền thành địa chỉ IP?', 'DHCP', 'DNS', 'ARP', 'ICMP', 'B', 'DNS (Domain Name System) dùng để phân giải tên miền thành địa chỉ IP.'),
('HARD', 'Trong database, normal form nào yêu cầu không có partial dependency?', '1NF', '2NF', '3NF', 'BCNF', 'B', '2NF (Second Normal Form) yêu cầu không có partial dependency.'),
('HARD', 'Trong lập trình, thuật toán nào có độ phức tạp O(1) cho insert và delete?', 'Array', 'Linked List', 'Hash Table', 'Binary Tree', 'C', 'Hash Table có độ phức tạp O(1) cho insert và delete trong trường hợp tốt nhất.'),
('HARD', 'Trong mạng, giao thức nào dùng để tự động cấp phát địa chỉ IP?', 'DNS', 'DHCP', 'ARP', 'ICMP', 'B', 'DHCP (Dynamic Host Configuration Protocol) dùng để tự động cấp phát địa chỉ IP.'),
('HARD', 'Trong database, câu lệnh nào dùng để tạo transaction?', 'BEGIN TRANSACTION', 'START TRANSACTION', 'BEGIN', 'Cả A và B', 'D', 'Cả "BEGIN TRANSACTION" và "START TRANSACTION" đều dùng để tạo transaction.'),
('HARD', 'Trong lập trình, design pattern nào cho phép thay đổi thuật toán tại runtime?', 'Strategy', 'Factory', 'Observer', 'Singleton', 'A', 'Strategy pattern cho phép thay đổi thuật toán tại runtime.'),
('HARD', 'Trong mạng, giao thức nào dùng để trao đổi thông tin routing giữa các router?', 'OSPF', 'HTTP', 'FTP', 'SMTP', 'A', 'OSPF (Open Shortest Path First) là giao thức routing.'),
('HARD', 'Trong database, index nào phù hợp cho full-text search?', 'B-tree', 'Hash', 'Bitmap', 'Full-text Index', 'D', 'Full-text Index được thiết kế đặc biệt cho full-text search.'),
('HARD', 'Trong lập trình, từ khóa "async" trong JavaScript có tác dụng gì?', 'Tạo hàm đồng bộ', 'Tạo hàm bất đồng bộ', 'Tạo promise', 'Tạo callback', 'B', 'Từ khóa "async" tạo hàm bất đồng bộ, tự động trả về Promise.'),
('HARD', 'Trong mạng, giao thức nào dùng để mã hóa kết nối SSH?', 'AES', '', 'SSL/TLS', 'Cả A và C', 'D', 'SSH sử dụng cả AES (symmetric) và RSA (asymmetric) để mã hóa.'),
('HARD', 'Trong database, isolation level nào đảm bảo không có phantom reads?', 'READ COMMITTED', 'REPEATABLE READ', 'SERIALIZABLE', 'Cả B và C', 'C', 'SERIALIZABLE là isolation level cao nhất, đảm bảo không có phantom reads.'),
('HARD', 'Trong lập trình, thuật toán nào dùng để tìm đường đi ngắn nhất trong đồ thị có trọng số?', 'BFS', 'DFS', 'Dijkstra', 'A*', 'C', 'Dijkstra algorithm dùng để tìm đường đi ngắn nhất trong đồ thị có trọng số không âm.');