-- =========================================================
-- Schema: Millionaire Game (PostgreSQL)
-- =========================================================

-- =========================
-- USERS & SESSIONS & STATS
-- =========================

DROP DATABASE IF EXISTS ltm_group04;
CREATE DATABASE ltm_group04;
\c ltm_group04;

CREATE TABLE IF NOT EXISTS users (
  user_id       BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  username      VARCHAR(32)  NOT NULL UNIQUE,
  password      VARCHAR(255) NOT NULL,
  avatar_img    VARCHAR(512),
  created_at    TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS user_stats (
  user_id         BIGINT PRIMARY KEY,
  quickmode_games BIGINT NOT NULL DEFAULT 0,
  onevn_games     BIGINT NOT NULL DEFAULT 0,
  quickmode_wins  BIGINT NOT NULL DEFAULT 0,
  onevn_wins      BIGINT NOT NULL DEFAULT 0,
  CONSTRAINT fk_user_stats_user
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS user_sessions (
  id              BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  user_id         BIGINT NOT NULL,
  access_token    CHAR(64) NOT NULL UNIQUE,
  last_heartbeat  TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  expires_at      TIMESTAMPTZ NOT NULL,
  CONSTRAINT fk_user_sessions_user
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_user_sessions_user ON user_sessions(user_id);
CREATE INDEX IF NOT EXISTS idx_user_sessions_exp  ON user_sessions(expires_at);

-- =================
-- MESSAGES (unified table for direct & room messages)
-- =================
CREATE TABLE IF NOT EXISTS messages (
  msg_id       BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  sender_id    BIGINT NOT NULL,
  receiver_id  BIGINT NULL,
  room_id      BIGINT NULL,
  content      TEXT   NOT NULL,
  is_delivered BOOLEAN NOT NULL DEFAULT FALSE,
  delivered_at TIMESTAMPTZ NULL,
  is_read      BOOLEAN NOT NULL DEFAULT FALSE,
  read_at      TIMESTAMPTZ NULL,
  created_at   TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  CONSTRAINT fk_messages_sender   FOREIGN KEY (sender_id)   REFERENCES users(user_id) ON DELETE CASCADE,
  CONSTRAINT fk_messages_receiver FOREIGN KEY (receiver_id) REFERENCES users(user_id) ON DELETE CASCADE
  -- room_id FK added later after room table exists
);

CREATE INDEX IF NOT EXISTS idx_messages_pair     ON messages (sender_id, receiver_id, created_at);
CREATE INDEX IF NOT EXISTS idx_messages_receiver ON messages (receiver_id, is_delivered, is_read);
CREATE INDEX IF NOT EXISTS idx_messages_room     ON messages (room_id, created_at);

-- =========
-- FRIENDS
-- =========
CREATE TABLE IF NOT EXISTS friends (
  user_id    BIGINT NOT NULL,
  friend_id  BIGINT NOT NULL,
  status     TEXT   NOT NULL CHECK (status IN ('PENDING','ACCEPTED','BLOCKED')),
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  PRIMARY KEY (user_id, friend_id),
  CONSTRAINT fk_friends_u1 FOREIGN KEY (user_id)   REFERENCES users(user_id) ON DELETE CASCADE,
  CONSTRAINT fk_friends_u2 FOREIGN KEY (friend_id) REFERENCES users(user_id) ON DELETE CASCADE,
  CONSTRAINT chk_friends_not_self CHECK (user_id <> friend_id)
);

CREATE INDEX IF NOT EXISTS idx_friends_status ON friends(status);

-- ================
-- FRIEND REQUESTS
-- ================
CREATE TABLE IF NOT EXISTS friend_request (
  rq_id         BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  from_user_id  BIGINT NOT NULL,
  to_user_id    BIGINT NOT NULL,
  status        TEXT   NOT NULL CHECK (status IN ('SENT','ACCEPTED','DECLINED','EXPIRED')),
  created_at    TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  responded_at  TIMESTAMPTZ NULL,
  CONSTRAINT fk_fr_from FOREIGN KEY (from_user_id) REFERENCES users(user_id) ON DELETE CASCADE,
  CONSTRAINT fk_fr_to   FOREIGN KEY (to_user_id)   REFERENCES users(user_id) ON DELETE CASCADE,
  CONSTRAINT chk_fr_not_self CHECK (from_user_id <> to_user_id)
);

CREATE INDEX IF NOT EXISTS idx_fr_to_status ON friend_request(to_user_id, status);

-- ===========
-- QUESTIONS
-- ===========
CREATE TABLE IF NOT EXISTS question (
  question_id      BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  difficulty_level TEXT NOT NULL CHECK (difficulty_level IN ('EASY','MEDIUM','HARD')),
  content          TEXT NOT NULL,
  op_a             VARCHAR(512) NOT NULL,
  op_b             VARCHAR(512) NOT NULL,
  op_c             VARCHAR(512) NOT NULL,
  op_d             VARCHAR(512) NOT NULL,
  correct_op       TEXT NOT NULL CHECK (correct_op IN ('A','B','C','D')),
  explanation      TEXT
);

CREATE INDEX IF NOT EXISTS idx_question_diff ON question(difficulty_level);

-- ======================
-- QUICK MODE (Solo 15Q)
-- ======================
CREATE TABLE IF NOT EXISTS quickmode_sessions (
  session_id     BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  user_id        BIGINT NOT NULL,
  status         TEXT NOT NULL CHECK (status IN ('IN_PROGRESS','FINISHED','FAILED')),
  current_round  INT  NOT NULL DEFAULT 1,
  total_correct  INT  NOT NULL DEFAULT 0,
  started_at     TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  ended_at       TIMESTAMPTZ NULL,
  CONSTRAINT fk_qs_user FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_qs_user_status ON quickmode_sessions(user_id, status);

CREATE TABLE IF NOT EXISTS quickmode_rounds (
  round_id         BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  session_id       BIGINT NOT NULL,
  round_number     INT    NOT NULL,
  question_id      BIGINT NOT NULL,
  difficulty_level TEXT   NOT NULL CHECK (difficulty_level IN ('EASY','MEDIUM','HARD')),
  time_limit       INT    NOT NULL DEFAULT 30,      -- seconds
  start_time       TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  CONSTRAINT fk_qr_session  FOREIGN KEY (session_id)  REFERENCES quickmode_sessions(session_id) ON DELETE CASCADE,
  CONSTRAINT fk_qr_question FOREIGN KEY (question_id) REFERENCES question(question_id)
);
CREATE UNIQUE INDEX IF NOT EXISTS uq_qr_session_round ON quickmode_rounds(session_id, round_number);

CREATE TABLE IF NOT EXISTS round_answers (
  id            BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  round_id      BIGINT NOT NULL,
  user_id       BIGINT NOT NULL,
  selected_op   TEXT   NOT NULL CHECK (selected_op IN ('A','B','C','D')),
  is_correct    BOOLEAN NOT NULL,
  answered_at   TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  CONSTRAINT fk_ra_round FOREIGN KEY (round_id) REFERENCES quickmode_rounds(round_id) ON DELETE CASCADE,
  CONSTRAINT fk_ra_user  FOREIGN KEY (user_id)  REFERENCES users(user_id) ON DELETE CASCADE
);
CREATE UNIQUE INDEX IF NOT EXISTS uq_ra_once ON round_answers(round_id, user_id);

-- 50:50 lifeline cho Quick Mode
CREATE TABLE IF NOT EXISTS lifeline_5050 (
  id               BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  session_id       BIGINT NOT NULL,
  round_id         BIGINT NOT NULL,
  user_id          BIGINT NOT NULL,
  removed_options  TEXT[] NOT NULL,
  is_used          BOOLEAN NOT NULL DEFAULT TRUE,
  used_at          TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  CONSTRAINT fk_ll_session FOREIGN KEY (session_id) REFERENCES quickmode_sessions(session_id) ON DELETE CASCADE,
  CONSTRAINT fk_ll_round   FOREIGN KEY (round_id)   REFERENCES quickmode_rounds(round_id)   ON DELETE CASCADE,
  CONSTRAINT fk_ll_user    FOREIGN KEY (user_id)    REFERENCES users(user_id)               ON DELETE CASCADE,
  CONSTRAINT chk_ll_removed
    CHECK (
      cardinality(removed_options) = 2
      AND removed_options <@ ARRAY['A','B','C','D']::TEXT[]
    )
);

-- ============
-- MATCH HISTORY
-- generic match history table used by stats/leaderboard endpoints
-- ============
CREATE TABLE IF NOT EXISTS match_history (
  match_id      BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  user_id       BIGINT NOT NULL,
  mode          TEXT NOT NULL,
  score         INT NOT NULL DEFAULT 0,
  is_win        BOOLEAN NOT NULL DEFAULT FALSE,
  total_correct INT NOT NULL DEFAULT 0,
  avg_answer_ms INT NOT NULL DEFAULT 0,
  played_at     TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  CONSTRAINT fk_mh_user FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_mh_user_played ON match_history(user_id, played_at DESC);

-- ============
-- 1vN ROOMS
-- ============
CREATE TABLE IF NOT EXISTS room (
  room_id                  BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  owner_id                 BIGINT NOT NULL,
  mode                     TEXT NOT NULL CHECK (mode IN ('BASIC','ONEVN')) DEFAULT 'ONEVN',
  status                   TEXT NOT NULL CHECK (status IN ('WAITING','PLAYING','FINISHED')),
  current_number_players   INT  NOT NULL DEFAULT 0,
  max_number_players       INT  NOT NULL DEFAULT 8,
  created_at               TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  started_at               TIMESTAMPTZ NULL,
  ended_at                 TIMESTAMPTZ NULL,
  CONSTRAINT fk_room_owner FOREIGN KEY (owner_id) REFERENCES users(user_id)
);

CREATE INDEX IF NOT EXISTS idx_room_status ON room(status);

-- Create a read/write view named 'rooms' so code referencing plural 'rooms' works too
CREATE OR REPLACE VIEW rooms AS SELECT * FROM room;

CREATE TABLE IF NOT EXISTS room_invites (
  id            BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  room_id       BIGINT NOT NULL,
  from_user_id  BIGINT NOT NULL,
  to_user_id    BIGINT NOT NULL,
  status        TEXT NOT NULL CHECK (status IN ('SENT','ACCEPTED','DECLINED','EXPIRED')),
  created_at    TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  responded_at  TIMESTAMPTZ NULL,
  CONSTRAINT fk_rinv_room FOREIGN KEY (room_id)      REFERENCES room(room_id)      ON DELETE CASCADE,
  CONSTRAINT fk_rinv_from FOREIGN KEY (from_user_id) REFERENCES users(user_id)     ON DELETE CASCADE,
  CONSTRAINT fk_rinv_to   FOREIGN KEY (to_user_id)   REFERENCES users(user_id)     ON DELETE CASCADE,
  CONSTRAINT chk_rinv_not_self CHECK (from_user_id <> to_user_id)
);

CREATE INDEX IF NOT EXISTS idx_rinv_to_status ON room_invites(to_user_id, status);

CREATE TABLE IF NOT EXISTS room_members (
  room_id   BIGINT NOT NULL,
  user_id   BIGINT NOT NULL,
  role      TEXT NOT NULL DEFAULT 'PLAYER',
  joined_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  PRIMARY KEY (room_id, user_id),
  CONSTRAINT fk_rmem_room FOREIGN KEY (room_id) REFERENCES room(room_id) ON DELETE CASCADE,
  CONSTRAINT fk_rmem_user FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE
);

-- Bổ sung FK cho messages.room_id (messages table was created earlier)
ALTER TABLE messages
  ADD CONSTRAINT fk_messages_room
  FOREIGN KEY (room_id) REFERENCES room(room_id) ON DELETE CASCADE;

-- ===============
-- 1vN SESSIONS
-- ===============
CREATE TABLE IF NOT EXISTS onevn_sessions (
  session_id   BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  room_id      BIGINT NOT NULL,
  winner_id    BIGINT NULL,
  status       TEXT NOT NULL CHECK (status IN ('IN_PROGRESS','FINISHED','ABORTED')),
  started_at   TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  ended_at     TIMESTAMPTZ NULL,
  CONSTRAINT fk_1s_room   FOREIGN KEY (room_id)   REFERENCES room(room_id)   ON DELETE CASCADE,
  CONSTRAINT fk_1s_winner FOREIGN KEY (winner_id) REFERENCES users(user_id)
);

CREATE UNIQUE INDEX IF NOT EXISTS uq_onevn_room_active
  ON onevn_sessions(room_id, status)
  WHERE status = 'IN_PROGRESS';

CREATE TABLE IF NOT EXISTS onevn_players (
  user_id       BIGINT NOT NULL,
  session_id    BIGINT NOT NULL,
  score         INT    NOT NULL DEFAULT 0,
  is_eliminated BOOLEAN NOT NULL DEFAULT FALSE,
  final_rank    INT NULL,
  PRIMARY KEY (session_id, user_id),
  CONSTRAINT fk_1p_user    FOREIGN KEY (user_id)    REFERENCES users(user_id)             ON DELETE CASCADE,
  CONSTRAINT fk_1p_session FOREIGN KEY (session_id) REFERENCES onevn_sessions(session_id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS onevn_rounds (
  round_id     BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  session_id   BIGINT NOT NULL,
  round_no     INT NOT NULL,
  question_id  BIGINT NOT NULL,
  difficulty   TEXT NOT NULL CHECK (difficulty IN ('EASY','MEDIUM','HARD')),
  time_limit   INT NOT NULL DEFAULT 20,    -- seconds
  start_time   TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  CONSTRAINT fk_1r_session  FOREIGN KEY (session_id)  REFERENCES onevn_sessions(session_id) ON DELETE CASCADE,
  CONSTRAINT fk_1r_question FOREIGN KEY (question_id) REFERENCES question(question_id)
);
CREATE UNIQUE INDEX IF NOT EXISTS uq_1r_session_no ON onevn_rounds(session_id, round_no);

CREATE TABLE IF NOT EXISTS onevn_answers (
  id              BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  round_id        BIGINT NOT NULL,
  user_id         BIGINT NOT NULL,
  selected_op     TEXT NOT NULL CHECK (selected_op IN ('A','B','C','D')),
  is_correct      BOOLEAN NOT NULL,
  point_awarded   INT NOT NULL DEFAULT 0,
  answered_at     TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  CONSTRAINT fk_1a_round FOREIGN KEY (round_id) REFERENCES onevn_rounds(round_id) ON DELETE CASCADE,
  CONSTRAINT fk_1a_user  FOREIGN KEY (user_id)  REFERENCES users(user_id)       ON DELETE CASCADE
);
CREATE UNIQUE INDEX IF NOT EXISTS uq_1a_once ON onevn_answers(round_id, user_id);



INSERT INTO question
  (difficulty_level, content, op_a, op_b, op_c, op_d, correct_op, explanation)
VALUES
-- EASY
('EASY',
 'Thủ đô của Việt Nam là thành phố nào?',
 'Hồ Chí Minh',
 'Hà Nội',
 'Đà Nẵng',
 'Huế',
 'B',
 'Thủ đô của Việt Nam hiện nay là Hà Nội.'),
('EASY',
 '"AI" trong "Trí tuệ nhân tạo (AI)" là viết tắt của cụm từ tiếng Anh nào?',
 'Artificial Intelligence',
 'Advanced Internet',
 'Automatic Interaction',
 'Algorithm Integration',
 'A',
 '"AI" là viết tắt của "Artificial Intelligence" (Trí tuệ nhân tạo).'),

-- MEDIUM
('MEDIUM',
 'Ngôn ngữ lập trình nào sau đây thường được dùng để lập trình hệ điều hành Linux?',
 'Python',
 'Java',
 'C',
 'HTML',
 'C',
 'Nhân Linux được viết chủ yếu bằng ngôn ngữ C.'),
('MEDIUM',
 'Giao thức nào sau đây hoạt động ở tầng Transport trong mô hình TCP/IP?',
 'IP',
 'TCP',
 'Ethernet',
 'ARP',
 'B',
 'TCP (Transmission Control Protocol) là giao thức tầng Transport.'),

-- HARD
('HARD',
 'Trong cơ sở dữ liệu quan hệ, ràng buộc đảm bảo giá trị một cột là duy nhất và không NULL được gọi là gì?',
 'FOREIGN KEY',
 'PRIMARY KEY',
 'UNIQUE KEY',
 'CHECK',
 'B',
 'PRIMARY KEY yêu cầu giá trị duy nhất và không được NULL.'),
('HARD',
 'Trong mô hình OSI, giao thức HTTP chủ yếu tương ứng với tầng nào?',
 'Tầng Mạng (Network)',
 'Tầng Vật lý (Physical)',
 'Tầng Giao vận (Transport)',
 'Tầng Ứng dụng (Application)',
 'D',
 'HTTP là giao thức tầng Ứng dụng trong mô hình OSI.');
