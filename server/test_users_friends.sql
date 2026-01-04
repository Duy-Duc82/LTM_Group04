-- Test Users and Friends Data for Network Testing
-- Usage: psql -U postgres -d ltm_group04 -f server/test_users_friends.sql

-- ============================================================
-- CREATE TEST USERS
-- ============================================================

-- User 1: testuser1
INSERT INTO users (username, password) 
VALUES ('testuser1', 'testpass1')
ON CONFLICT (username) DO NOTHING;

-- User 2: testuser2
INSERT INTO users (username, password) 
VALUES ('testuser2', 'testpass2')
ON CONFLICT (username) DO NOTHING;

-- User 3: testuser3
INSERT INTO users (username, password) 
VALUES ('testuser3', 'testpass3')
ON CONFLICT (username) DO NOTHING;

-- User 4: alice
INSERT INTO users (username, password) 
VALUES ('alice', 'alice123')
ON CONFLICT (username) DO UPDATE SET password = EXCLUDED.password;

-- User 5: bob
INSERT INTO users (username, password) 
VALUES ('bob', 'bob123')
ON CONFLICT (username) DO NOTHING;

-- User 6: charlie
INSERT INTO users (username, password) 
VALUES ('charlie', 'charlie123')
ON CONFLICT (username) DO NOTHING;

-- ============================================================
-- CREATE FRIEND RELATIONSHIPS
-- ============================================================

-- Get user IDs (assuming they exist or were just created)
-- Note: In PostgreSQL, we can use subqueries to get IDs

-- testuser1 sends friend request to testuser2 (PENDING)
-- This means testuser2 will receive a pending request from testuser1
INSERT INTO friend_relationships (user_id, peer_user_id, status, created_at)
SELECT u1.user_id, u2.user_id, 'PENDING', NOW()
FROM users u1, users u2
WHERE u1.username = 'testuser1' AND u2.username = 'testuser2'
ON CONFLICT (user_id, peer_user_id) DO UPDATE SET status = 'PENDING', responded_at = NULL;

-- testuser1 and alice are friends (ACCEPTED)
INSERT INTO friend_relationships (user_id, peer_user_id, status, created_at, responded_at)
SELECT u1.user_id, u2.user_id, 'ACCEPTED', NOW() - INTERVAL '1 day', NOW() - INTERVAL '1 day'
FROM users u1, users u2
WHERE u1.username = 'testuser1' AND u2.username = 'alice'
ON CONFLICT (user_id, peer_user_id) DO UPDATE SET status = 'ACCEPTED';

-- Also create reverse relationship (friendship is bidirectional)
INSERT INTO friend_relationships (user_id, peer_user_id, status, created_at, responded_at)
SELECT u1.user_id, u2.user_id, 'ACCEPTED', NOW() - INTERVAL '1 day', NOW() - INTERVAL '1 day'
FROM users u1, users u2
WHERE u1.username = 'alice' AND u2.username = 'testuser1'
ON CONFLICT (user_id, peer_user_id) DO UPDATE SET status = 'ACCEPTED';

-- testuser1 and bob are friends (ACCEPTED)
INSERT INTO friend_relationships (user_id, peer_user_id, status, created_at, responded_at)
SELECT u1.user_id, u2.user_id, 'ACCEPTED', NOW() - INTERVAL '2 days', NOW() - INTERVAL '2 days'
FROM users u1, users u2
WHERE u1.username = 'testuser1' AND u2.username = 'bob'
ON CONFLICT (user_id, peer_user_id) DO UPDATE SET status = 'ACCEPTED';

INSERT INTO friend_relationships (user_id, peer_user_id, status, created_at, responded_at)
SELECT u1.user_id, u2.user_id, 'ACCEPTED', NOW() - INTERVAL '2 days', NOW() - INTERVAL '2 days'
FROM users u1, users u2
WHERE u1.username = 'bob' AND u2.username = 'testuser1'
ON CONFLICT (user_id, peer_user_id) DO UPDATE SET status = 'ACCEPTED';

-- testuser2 sends friend request to testuser3 (PENDING - for testuser3 to accept)
INSERT INTO friend_relationships (user_id, peer_user_id, status, created_at)
SELECT u1.user_id, u2.user_id, 'PENDING', NOW()
FROM users u1, users u2
WHERE u1.username = 'testuser2' AND u2.username = 'testuser3'
ON CONFLICT (user_id, peer_user_id) DO UPDATE SET status = 'PENDING', responded_at = NULL;

-- ============================================================
-- VERIFY DATA
-- ============================================================

-- Show all users
SELECT '=== ALL USERS ===' AS info;
SELECT user_id, username FROM users ORDER BY user_id;

-- Show all friend relationships
SELECT '=== FRIEND RELATIONSHIPS ===' AS info;
SELECT 
    u1.username AS user1,
    u2.username AS user2,
    fr.status,
    fr.created_at,
    fr.responded_at
FROM friend_relationships fr
JOIN users u1 ON fr.user_id = u1.user_id
JOIN users u2 ON fr.peer_user_id = u2.user_id
ORDER BY fr.created_at DESC;

-- Show testuser1's friends
SELECT '=== TESTUSER1 FRIENDS ===' AS info;
SELECT 
    u.username AS friend_username,
    fr.status
FROM friend_relationships fr
JOIN users u ON u.user_id = fr.peer_user_id
JOIN users u1 ON fr.user_id = u1.user_id
WHERE u1.username = 'testuser1' AND fr.status = 'ACCEPTED';

-- Show pending requests for testuser1
SELECT '=== PENDING REQUESTS FOR TESTUSER1 ===' AS info;
SELECT 
    u.username AS from_username,
    fr.created_at
FROM friend_relationships fr
JOIN users u ON u.user_id = fr.user_id
JOIN users u1 ON fr.peer_user_id = u1.user_id
WHERE u1.username = 'testuser1' AND fr.status = 'PENDING';

-- ============================================================
-- CLEANUP (Optional - commented out)
-- ============================================================
-- Uncomment to remove test data:
-- DELETE FROM friend_relationships WHERE user_id IN (SELECT user_id FROM users WHERE username LIKE 'testuser%' OR username IN ('alice', 'bob', 'charlie'));
-- DELETE FROM users WHERE username LIKE 'testuser%' OR username IN ('alice', 'bob', 'charlie');

