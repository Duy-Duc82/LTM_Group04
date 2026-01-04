-- Add friends for account "duc"
-- Usage: psql -U postgres -d ltm_group04 -f server/add_friends_for_duc.sql

-- ============================================================
-- CREATE TEST USERS (if they don't exist)
-- ============================================================

-- Create some test users to be friends with "duc"
-- Use INSERT ... ON CONFLICT UPDATE to ensure password is correct
INSERT INTO users (username, password) 
VALUES ('alice', 'alice123')
ON CONFLICT (username) DO UPDATE SET password = EXCLUDED.password;

INSERT INTO users (username, password) 
VALUES ('bob', 'bob123')
ON CONFLICT (username) DO NOTHING;

INSERT INTO users (username, password) 
VALUES ('charlie', 'charlie123')
ON CONFLICT (username) DO NOTHING;

INSERT INTO users (username, password) 
VALUES ('david', 'david123')
ON CONFLICT (username) DO NOTHING;

INSERT INTO users (username, password) 
VALUES ('eve', 'eve123')
ON CONFLICT (username) DO NOTHING;

-- ============================================================
-- ADD FRIEND RELATIONSHIPS FOR "duc"
-- ============================================================

-- duc and alice are friends (ACCEPTED) - bidirectional
INSERT INTO friend_relationships (user_id, peer_user_id, status, created_at, responded_at)
SELECT u1.user_id, u2.user_id, 'ACCEPTED', NOW() - INTERVAL '5 days', NOW() - INTERVAL '5 days'
FROM users u1, users u2
WHERE u1.username = 'duc' AND u2.username = 'alice'
ON CONFLICT (user_id, peer_user_id) DO UPDATE SET status = 'ACCEPTED';

INSERT INTO friend_relationships (user_id, peer_user_id, status, created_at, responded_at)
SELECT u1.user_id, u2.user_id, 'ACCEPTED', NOW() - INTERVAL '5 days', NOW() - INTERVAL '5 days'
FROM users u1, users u2
WHERE u1.username = 'alice' AND u2.username = 'duc'
ON CONFLICT (user_id, peer_user_id) DO UPDATE SET status = 'ACCEPTED';

-- duc and bob are friends (ACCEPTED) - bidirectional
INSERT INTO friend_relationships (user_id, peer_user_id, status, created_at, responded_at)
SELECT u1.user_id, u2.user_id, 'ACCEPTED', NOW() - INTERVAL '3 days', NOW() - INTERVAL '3 days'
FROM users u1, users u2
WHERE u1.username = 'duc' AND u2.username = 'bob'
ON CONFLICT (user_id, peer_user_id) DO UPDATE SET status = 'ACCEPTED';

INSERT INTO friend_relationships (user_id, peer_user_id, status, created_at, responded_at)
SELECT u1.user_id, u2.user_id, 'ACCEPTED', NOW() - INTERVAL '3 days', NOW() - INTERVAL '3 days'
FROM users u1, users u2
WHERE u1.username = 'bob' AND u2.username = 'duc'
ON CONFLICT (user_id, peer_user_id) DO UPDATE SET status = 'ACCEPTED';

-- duc and charlie are friends (ACCEPTED) - bidirectional
INSERT INTO friend_relationships (user_id, peer_user_id, status, created_at, responded_at)
SELECT u1.user_id, u2.user_id, 'ACCEPTED', NOW() - INTERVAL '2 days', NOW() - INTERVAL '2 days'
FROM users u1, users u2
WHERE u1.username = 'duc' AND u2.username = 'charlie'
ON CONFLICT (user_id, peer_user_id) DO UPDATE SET status = 'ACCEPTED';

INSERT INTO friend_relationships (user_id, peer_user_id, status, created_at, responded_at)
SELECT u1.user_id, u2.user_id, 'ACCEPTED', NOW() - INTERVAL '2 days', NOW() - INTERVAL '2 days'
FROM users u1, users u2
WHERE u1.username = 'charlie' AND u2.username = 'duc'
ON CONFLICT (user_id, peer_user_id) DO UPDATE SET status = 'ACCEPTED';

-- duc and david are friends (ACCEPTED) - bidirectional
INSERT INTO friend_relationships (user_id, peer_user_id, status, created_at, responded_at)
SELECT u1.user_id, u2.user_id, 'ACCEPTED', NOW() - INTERVAL '1 day', NOW() - INTERVAL '1 day'
FROM users u1, users u2
WHERE u1.username = 'duc' AND u2.username = 'david'
ON CONFLICT (user_id, peer_user_id) DO UPDATE SET status = 'ACCEPTED';

INSERT INTO friend_relationships (user_id, peer_user_id, status, created_at, responded_at)
SELECT u1.user_id, u2.user_id, 'ACCEPTED', NOW() - INTERVAL '1 day', NOW() - INTERVAL '1 day'
FROM users u1, users u2
WHERE u1.username = 'david' AND u2.username = 'duc'
ON CONFLICT (user_id, peer_user_id) DO UPDATE SET status = 'ACCEPTED';

-- duc and eve are friends (ACCEPTED) - bidirectional
INSERT INTO friend_relationships (user_id, peer_user_id, status, created_at, responded_at)
SELECT u1.user_id, u2.user_id, 'ACCEPTED', NOW() - INTERVAL '6 hours', NOW() - INTERVAL '6 hours'
FROM users u1, users u2
WHERE u1.username = 'duc' AND u2.username = 'eve'
ON CONFLICT (user_id, peer_user_id) DO UPDATE SET status = 'ACCEPTED';

INSERT INTO friend_relationships (user_id, peer_user_id, status, created_at, responded_at)
SELECT u1.user_id, u2.user_id, 'ACCEPTED', NOW() - INTERVAL '6 hours', NOW() - INTERVAL '6 hours'
FROM users u1, users u2
WHERE u1.username = 'eve' AND u2.username = 'duc'
ON CONFLICT (user_id, peer_user_id) DO UPDATE SET status = 'ACCEPTED';

-- ============================================================
-- VERIFY DATA
-- ============================================================

-- Show duc's friends
SELECT '=== DUC FRIENDS ===' AS info;
SELECT 
    u.username AS friend_username,
    fr.status,
    fr.created_at
FROM friend_relationships fr
JOIN users u ON u.user_id = fr.peer_user_id
JOIN users u1 ON fr.user_id = u1.user_id
WHERE u1.username = 'duc' AND fr.status = 'ACCEPTED'
ORDER BY fr.created_at DESC;

-- Show all friend relationships involving duc
SELECT '=== ALL RELATIONSHIPS WITH DUC ===' AS info;
SELECT 
    u1.username AS user1,
    u2.username AS user2,
    fr.status,
    fr.created_at
FROM friend_relationships fr
JOIN users u1 ON fr.user_id = u1.user_id
JOIN users u2 ON fr.peer_user_id = u2.user_id
WHERE u1.username = 'duc' OR u2.username = 'duc'
ORDER BY fr.created_at DESC;

