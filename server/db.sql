-- =========================================================
-- Schema: Millionaire Game (PostgreSQL) - Updated Version
-- =========================================================

DROP DATABASE IF EXISTS ltm_group04;
CREATE DATABASE ltm_group04;
\c ltm_group04;

-- =========================
-- USERS (gộp stats vào đây)
-- =========================
CREATE TABLE users (
  user_id         BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  username        VARCHAR(32)  NOT NULL UNIQUE,
  password        VARCHAR(255) NOT NULL,
  avatar_img      VARCHAR(512) NULL,
  quickmode_games INT NOT NULL DEFAULT 0,
  quickmode_wins  INT NOT NULL DEFAULT 0,
  onevn_games     INT NOT NULL DEFAULT 0,
  onevn_wins      INT NOT NULL DEFAULT 0,
  created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- =========================
-- USER SESSIONS
-- =========================
CREATE TABLE user_sessions (
  id              BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  user_id         BIGINT NOT NULL,
  access_token    CHAR(64) NOT NULL UNIQUE,
  last_heartbeat  TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  expires_at      TIMESTAMPTZ NOT NULL,
  CONSTRAINT fk_user_sessions_user
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE
);

CREATE INDEX idx_user_sessions_user ON user_sessions(user_id);
CREATE INDEX idx_user_sessions_exp  ON user_sessions(expires_at);
CREATE INDEX idx_user_sessions_heartbeat ON user_sessions(last_heartbeat);

-- =========================
-- MESSAGES
-- =========================
CREATE TABLE messages (
  id            BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  sender_id     BIGINT NOT NULL,
  receiver_id   BIGINT NULL,  -- NULL nếu là tin nhắn phòng
  room_id       BIGINT NULL,  -- NULL nếu là chat 1-1
  message       TEXT NOT NULL,
  created_at    TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  is_delivered  BOOLEAN NOT NULL DEFAULT FALSE,
  delivered_at  TIMESTAMPTZ NULL,
  is_read       BOOLEAN NOT NULL DEFAULT FALSE,
  read_at       TIMESTAMPTZ NULL,
  CONSTRAINT fk_messages_sender   FOREIGN KEY (sender_id)   REFERENCES users(user_id) ON DELETE CASCADE,
  CONSTRAINT fk_messages_receiver FOREIGN KEY (receiver_id) REFERENCES users(user_id) ON DELETE CASCADE,
  CONSTRAINT chk_messages_type CHECK (
    (receiver_id IS NOT NULL AND room_id IS NULL) OR
    (receiver_id IS NULL AND room_id IS NOT NULL)
  )
);

CREATE INDEX idx_messages_pair ON messages (sender_id, receiver_id, created_at);
CREATE INDEX idx_messages_receiver ON messages (receiver_id, is_delivered, is_read);
CREATE INDEX idx_messages_room ON messages (room_id, created_at);

-- =========================
-- FRIEND RELATIONSHIPS
-- =========================
CREATE TABLE friend_relationships (
  id            BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  user_id       BIGINT NOT NULL,
  peer_user_id  BIGINT NOT NULL,
  status        TEXT NOT NULL CHECK (status IN ('PENDING','ACCEPTED','DECLINED','BLOCKED')),
  created_at    TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  responded_at  TIMESTAMPTZ NULL,
  CONSTRAINT fk_fr_user FOREIGN KEY (user_id)      REFERENCES users(user_id) ON DELETE CASCADE,
  CONSTRAINT fk_fr_peer FOREIGN KEY (peer_user_id) REFERENCES users(user_id) ON DELETE CASCADE,
  CONSTRAINT chk_fr_not_self CHECK (user_id <> peer_user_id)
);

CREATE INDEX idx_fr_user ON friend_relationships(user_id, status);
CREATE INDEX idx_fr_peer ON friend_relationships(peer_user_id, status);
CREATE UNIQUE INDEX uq_fr_pair ON friend_relationships(user_id, peer_user_id);

-- =========================
-- QUESTIONS
-- =========================
CREATE TABLE question (
  question_id      BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  difficulty_level TEXT NOT NULL CHECK (difficulty_level IN ('EASY','MEDIUM','HARD')),
  content          TEXT NOT NULL,
  "opA"            VARCHAR(512) NOT NULL,
  "opB"            VARCHAR(512) NOT NULL,
  "opC"            VARCHAR(512) NOT NULL,
  "opD"            VARCHAR(512) NOT NULL,
  correct_op       CHAR(1) NOT NULL CHECK (correct_op IN ('A','B','C','D')),
  explanation      TEXT NULL
);

CREATE INDEX idx_question_diff ON question(difficulty_level);

-- =========================
-- ROOMS
-- =========================
CREATE TABLE room (
  room_id                BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  owner_id               BIGINT NOT NULL,
  status                 TEXT NOT NULL CHECK (status IN ('WAITING','STARTING','IN_PROGRESS','FINISHED')),
  max_number_players     INT  NOT NULL DEFAULT 8,
  created_at             TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  started_at             TIMESTAMPTZ NULL,
  ended_at               TIMESTAMPTZ NULL,
  CONSTRAINT fk_room_owner FOREIGN KEY (owner_id) REFERENCES users(user_id)
);

CREATE INDEX idx_room_status ON room(status);
CREATE OR REPLACE VIEW rooms AS SELECT * FROM room;

CREATE TABLE room_members (
  room_id   BIGINT NOT NULL,
  user_id   BIGINT NOT NULL,
  joined_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  PRIMARY KEY (room_id, user_id),
  CONSTRAINT fk_rmem_room FOREIGN KEY (room_id) REFERENCES room(room_id) ON DELETE CASCADE,
  CONSTRAINT fk_rmem_user FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE
);

-- Bổ sung FK cho messages.room_id
ALTER TABLE messages
  ADD CONSTRAINT fk_messages_room
  FOREIGN KEY (room_id) REFERENCES room(room_id) ON DELETE CASCADE;

-- =========================
-- 1vN SESSIONS (chỉ lưu kết quả cuối)
-- =========================
CREATE TABLE onevn_sessions (
  session_id   BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  room_id      BIGINT NOT NULL,
  status       TEXT NOT NULL CHECK (status IN ('IN_PROGRESS','FINISHED','ABORTED')),
  winner_id    BIGINT NULL,
  started_at   TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  ended_at     TIMESTAMPTZ NULL,
  players      JSONB NOT NULL DEFAULT '[]'::jsonb,  -- [{ user_id, score, final_rank, eliminated }]
  CONSTRAINT fk_1s_room   FOREIGN KEY (room_id)   REFERENCES room(room_id)   ON DELETE CASCADE,
  CONSTRAINT fk_1s_winner FOREIGN KEY (winner_id) REFERENCES users(user_id)
);

CREATE UNIQUE INDEX uq_onevn_room_active
  ON onevn_sessions(room_id, status)
  WHERE status = 'IN_PROGRESS';

CREATE INDEX idx_onevn_players_jsonb ON onevn_sessions USING GIN (players);

-- =========================
-- SAMPLE DATA
-- =========================
INSERT INTO question (difficulty_level, content, "opA", "opB", "opC", "opD", correct_op, explanation)
VALUES
('EASY', 'Thủ đô của Việt Nam là thành phố nào?', 'Hồ Chí Minh', 'Hà Nội', 'Đà Nẵng', 'Huế', 'B', 'Thủ đô của Việt Nam hiện nay là Hà Nội.'),
('EASY', '"AI" trong "Trí tuệ nhân tạo (AI)" là viết tắt của cụm từ tiếng Anh nào?', 'Artificial Intelligence', 'Advanced Internet', 'Automatic Interaction', 'Algorithm Integration', 'A', '"AI" là viết tắt của "Artificial Intelligence" (Trí tuệ nhân tạo).'),
('MEDIUM', 'Ngôn ngữ lập trình nào sau đây thường được dùng để lập trình hệ điều hành Linux?', 'Python', 'Java', 'C', 'HTML', 'C', 'Nhân Linux được viết chủ yếu bằng ngôn ngữ C.'),
('MEDIUM', 'Giao thức nào sau đây hoạt động ở tầng Transport trong mô hình TCP/IP?', 'IP', 'TCP', 'Ethernet', 'ARP', 'B', 'TCP (Transmission Control Protocol) là giao thức tầng Transport.'),
('HARD', 'Trong cơ sở dữ liệu quan hệ, ràng buộc đảm bảo giá trị một cột là duy nhất và không NULL được gọi là gì?', 'FOREIGN KEY', 'PRIMARY KEY', 'UNIQUE KEY', 'CHECK', 'B', 'PRIMARY KEY yêu cầu giá trị duy nhất và không được NULL.'),
('HARD', 'Trong mô hình OSI, giao thức HTTP chủ yếu tương ứng với tầng nào?', 'Tầng Mạng (Network)', 'Tầng Vật lý (Physical)', 'Tầng Giao vận (Transport)', 'Tầng Ứng dụng (Application)', 'D', 'HTTP là giao thức tầng Ứng dụng trong mô hình OSI.');
