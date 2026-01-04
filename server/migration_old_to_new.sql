-- =========================================================
-- MIGRATION SCRIPT: Old Schema → New Schema
-- =========================================================
-- Script này giúp migrate dữ liệu từ schema cũ sang schema mới
-- Chạy script này nếu bạn đã có dữ liệu trong database cũ

-- =========================================================
-- BƯỚC 1: Backup dữ liệu (nếu cần)
-- =========================================================
-- Trước khi chạy migration, nên backup database:
-- pg_dump -U postgres ltm_group04 > backup_before_migration.sql

-- =========================================================
-- BƯỚC 2: Tạo các bảng mới tạm thời để chứa dữ liệu
-- =========================================================

-- Backup users (nếu có user_stats riêng)
CREATE TABLE IF NOT EXISTS users_backup AS 
SELECT * FROM users;

-- Backup messages
CREATE TABLE IF NOT EXISTS messages_backup AS 
SELECT * FROM messages;

-- Backup friends (nếu có bảng friends cũ)
CREATE TABLE IF NOT EXISTS friends_backup AS 
SELECT * FROM friends;

-- Backup friend_request (nếu có)
CREATE TABLE IF NOT EXISTS friend_request_backup AS 
SELECT * FROM friend_request;

-- Backup questions
CREATE TABLE IF NOT EXISTS question_backup AS 
SELECT * FROM question;

-- Backup rooms
CREATE TABLE IF NOT EXISTS room_backup AS 
SELECT * FROM room;

-- Backup room_members
CREATE TABLE IF NOT EXISTS room_members_backup AS 
SELECT * FROM room_members;

-- =========================================================
-- BƯỚC 3: Drop các bảng cũ và constraints
-- =========================================================

-- Drop foreign keys trước
ALTER TABLE IF EXISTS messages DROP CONSTRAINT IF EXISTS fk_messages_room;
ALTER TABLE IF EXISTS user_sessions DROP CONSTRAINT IF EXISTS fk_user_sessions_user;
ALTER TABLE IF EXISTS friends DROP CONSTRAINT IF EXISTS fk_friends_u1;
ALTER TABLE IF EXISTS friends DROP CONSTRAINT IF EXISTS fk_friends_u2;
ALTER TABLE IF EXISTS friend_request DROP CONSTRAINT IF EXISTS fk_fr_from;
ALTER TABLE IF EXISTS friend_request DROP CONSTRAINT IF EXISTS fk_fr_to;
ALTER TABLE IF EXISTS room DROP CONSTRAINT IF EXISTS fk_room_owner;
ALTER TABLE IF EXISTS room_members DROP CONSTRAINT IF EXISTS fk_rmem_room;
ALTER TABLE IF EXISTS room_members DROP CONSTRAINT IF EXISTS fk_rmem_user;

-- Drop các bảng cũ (nếu tồn tại)
DROP TABLE IF EXISTS onevn_answers CASCADE;
DROP TABLE IF EXISTS onevn_rounds CASCADE;
DROP TABLE IF EXISTS onevn_players CASCADE;
DROP TABLE IF EXISTS onevn_sessions CASCADE;
DROP TABLE IF EXISTS quickmode_answers CASCADE;
DROP TABLE IF EXISTS quickmode_rounds CASCADE;
DROP TABLE IF EXISTS lifeline_5050 CASCADE;
DROP TABLE IF EXISTS quickmode_sessions CASCADE;
DROP TABLE IF EXISTS match_history CASCADE;
DROP TABLE IF EXISTS room_invites CASCADE;
DROP TABLE IF EXISTS friend_request CASCADE;
DROP TABLE IF EXISTS friends CASCADE;
DROP TABLE IF EXISTS user_stats CASCADE;
DROP TABLE IF EXISTS messages CASCADE;
DROP TABLE IF EXISTS question CASCADE;
DROP TABLE IF EXISTS room_members CASCADE;
DROP TABLE IF EXISTS room CASCADE;
DROP TABLE IF EXISTS user_sessions CASCADE;
DROP TABLE IF EXISTS users CASCADE;

-- =========================================================
-- BƯỚC 4: Tạo schema mới (từ db.sql)
-- =========================================================

-- Users (gộp stats)
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

-- User Sessions
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

-- Messages
CREATE TABLE messages (
  id            BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  sender_id     BIGINT NOT NULL,
  receiver_id   BIGINT NULL,
  room_id       BIGINT NULL,
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

-- Friend Relationships
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

-- Questions
CREATE TABLE question (
  question_id      BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  difficulty_level TEXT NOT NULL CHECK (difficulty_level IN ('EASY','MEDIUM','HARD')),
  content          TEXT NOT NULL,
  opA              VARCHAR(512) NOT NULL,
  opB              VARCHAR(512) NOT NULL,
  opC              VARCHAR(512) NOT NULL,
  opD              VARCHAR(512) NOT NULL,
  correct_op       CHAR(1) NOT NULL CHECK (correct_op IN ('A','B','C','D')),
  explanation      TEXT NULL
);

CREATE INDEX idx_question_diff ON question(difficulty_level);

-- Rooms
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

ALTER TABLE messages
  ADD CONSTRAINT fk_messages_room
  FOREIGN KEY (room_id) REFERENCES room(room_id) ON DELETE CASCADE;

-- 1vN Sessions
CREATE TABLE onevn_sessions (
  session_id   BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  room_id      BIGINT NOT NULL,
  status       TEXT NOT NULL CHECK (status IN ('IN_PROGRESS','FINISHED','ABORTED')),
  winner_id    BIGINT NULL,
  started_at   TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  ended_at     TIMESTAMPTZ NULL,
  players      JSONB NOT NULL DEFAULT '[]'::jsonb,
  CONSTRAINT fk_1s_room   FOREIGN KEY (room_id)   REFERENCES room(room_id)   ON DELETE CASCADE,
  CONSTRAINT fk_1s_winner FOREIGN KEY (winner_id) REFERENCES users(user_id)
);

CREATE UNIQUE INDEX uq_onevn_room_active
  ON onevn_sessions(room_id, status)
  WHERE status = 'IN_PROGRESS';

CREATE INDEX idx_onevn_players_jsonb ON onevn_sessions USING GIN (players);

-- =========================================================
-- BƯỚC 5: Migrate dữ liệu từ backup
-- =========================================================

-- Migrate users (gộp stats nếu có)
INSERT INTO users (user_id, username, password, avatar_img, quickmode_games, quickmode_wins, onevn_games, onevn_wins, created_at)
SELECT 
    u.user_id,
    u.username,
    u.password,
    COALESCE(u.avatar_img, ''),
    COALESCE(us.quickmode_games, 0),
    COALESCE(us.quickmode_wins, 0),
    COALESCE(us.onevn_games, 0),
    COALESCE(us.onevn_wins, 0),
    u.created_at
FROM users_backup u
LEFT JOIN user_stats us ON u.user_id = us.user_id
ON CONFLICT (user_id) DO NOTHING;

-- Reset sequence để tránh conflict
SELECT setval('users_user_id_seq', (SELECT MAX(user_id) FROM users));

-- Migrate user_sessions
INSERT INTO user_sessions (id, user_id, access_token, last_heartbeat, expires_at)
SELECT 
    id,
    user_id,
    access_token,
    COALESCE(last_heartbeat, NOW()),
    expires_at
FROM user_sessions_backup
ON CONFLICT (id) DO NOTHING;

SELECT setval('user_sessions_id_seq', (SELECT MAX(id) FROM user_sessions));

-- Migrate messages (đổi content → message, msg_id → id)
INSERT INTO messages (id, sender_id, receiver_id, room_id, message, created_at, is_delivered, delivered_at, is_read, read_at)
SELECT 
    msg_id,
    sender_id,
    receiver_id,
    room_id,
    content,  -- content cũ → message mới
    created_at,
    COALESCE(is_delivered, FALSE),
    delivered_at,
    COALESCE(is_read, FALSE),
    read_at
FROM messages_backup
ON CONFLICT (id) DO NOTHING;

SELECT setval('messages_id_seq', (SELECT MAX(id) FROM messages));

-- Migrate friends → friend_relationships
-- Nếu có bảng friends cũ với composite PK
INSERT INTO friend_relationships (user_id, peer_user_id, status, created_at, responded_at)
SELECT 
    user_id,
    friend_id,  -- friend_id cũ → peer_user_id mới
    status,
    created_at,
    NULL  -- responded_at mới, chưa có trong schema cũ
FROM friends_backup
WHERE status IN ('PENDING', 'ACCEPTED', 'BLOCKED')
ON CONFLICT (user_id, peer_user_id) DO NOTHING;

-- Nếu có bảng friend_request riêng
INSERT INTO friend_relationships (user_id, peer_user_id, status, created_at, responded_at)
SELECT 
    from_user_id,
    to_user_id,
    CASE 
        WHEN status = 'SENT' THEN 'PENDING'
        WHEN status = 'ACCEPTED' THEN 'ACCEPTED'
        WHEN status = 'DECLINED' THEN 'DECLINED'
        WHEN status = 'EXPIRED' THEN 'PENDING'
        ELSE 'PENDING'
    END,
    created_at,
    responded_at
FROM friend_request_backup
WHERE NOT EXISTS (
    SELECT 1 FROM friend_relationships fr 
    WHERE fr.user_id = friend_request_backup.from_user_id 
    AND fr.peer_user_id = friend_request_backup.to_user_id
)
ON CONFLICT (user_id, peer_user_id) DO NOTHING;

-- Migrate questions (đổi op_a → opA, etc.)
INSERT INTO question (question_id, difficulty_level, content, "opA", "opB", "opC", "opD", correct_op, explanation)
SELECT 
    question_id,
    difficulty_level,
    content,
    op_a,  -- op_a cũ → opA mới
    op_b,
    op_c,
    op_d,
    correct_op,
    COALESCE(explanation, '')
FROM question_backup
ON CONFLICT (question_id) DO NOTHING;

SELECT setval('question_question_id_seq', (SELECT MAX(question_id) FROM question));

-- Migrate rooms (bỏ mode, current_number_players)
INSERT INTO room (room_id, owner_id, status, max_number_players, created_at, started_at, ended_at)
SELECT 
    room_id,
    owner_id,
    CASE 
        WHEN status = 'PLAYING' THEN 'IN_PROGRESS'
        ELSE status
    END,
    COALESCE(max_number_players, 8),
    created_at,
    started_at,
    ended_at
FROM room_backup
ON CONFLICT (room_id) DO NOTHING;

SELECT setval('room_room_id_seq', (SELECT MAX(room_id) FROM room));

-- Migrate room_members (bỏ role)
INSERT INTO room_members (room_id, user_id, joined_at)
SELECT 
    room_id,
    user_id,
    COALESCE(joined_at, NOW())
FROM room_members_backup
ON CONFLICT (room_id, user_id) DO NOTHING;

-- Migrate onevn_sessions (nếu có dữ liệu cũ)
-- Chuyển từ normalized tables sang JSONB
INSERT INTO onevn_sessions (session_id, room_id, status, winner_id, started_at, ended_at, players)
SELECT 
    s.session_id,
    s.room_id,
    s.status,
    s.winner_id,
    s.started_at,
    s.ended_at,
    COALESCE(
        (
            SELECT jsonb_agg(
                jsonb_build_object(
                    'user_id', p.user_id,
                    'score', p.score,
                    'final_rank', p.final_rank,
                    'eliminated', p.is_eliminated
                )
            )
            FROM onevn_players_backup p
            WHERE p.session_id = s.session_id
        ),
        '[]'::jsonb
    ) as players
FROM onevn_sessions_backup s
ON CONFLICT (session_id) DO NOTHING;

SELECT setval('onevn_sessions_session_id_seq', (SELECT MAX(session_id) FROM onevn_sessions));

-- =========================================================
-- BƯỚC 6: Cleanup backup tables (sau khi verify)
-- =========================================================
-- CHỈ CHẠY SAU KHI ĐÃ VERIFY DỮ LIỆU MỚI ĐÚNG!

-- DROP TABLE IF EXISTS users_backup;
-- DROP TABLE IF EXISTS messages_backup;
-- DROP TABLE IF EXISTS friends_backup;
-- DROP TABLE IF EXISTS friend_request_backup;
-- DROP TABLE IF EXISTS question_backup;
-- DROP TABLE IF EXISTS room_backup;
-- DROP TABLE IF EXISTS room_members_backup;
-- DROP TABLE IF EXISTS onevn_sessions_backup;
-- DROP TABLE IF EXISTS onevn_players_backup;

-- =========================================================
-- VERIFICATION QUERIES
-- =========================================================

-- Kiểm tra số lượng records
-- SELECT 'users' as table_name, COUNT(*) FROM users
-- UNION ALL
-- SELECT 'user_sessions', COUNT(*) FROM user_sessions
-- UNION ALL
-- SELECT 'messages', COUNT(*) FROM messages
-- UNION ALL
-- SELECT 'friend_relationships', COUNT(*) FROM friend_relationships
-- UNION ALL
-- SELECT 'question', COUNT(*) FROM question
-- UNION ALL
-- SELECT 'room', COUNT(*) FROM room
-- UNION ALL
-- SELECT 'room_members', COUNT(*) FROM room_members
-- UNION ALL
-- SELECT 'onevn_sessions', COUNT(*) FROM onevn_sessions;

