-- =========================================================
-- TEST SCRIPT: Friend Request Flow
-- Mục đích: Test luồng gửi lời mời và chấp nhận kết bạn
-- =========================================================

\c ltm_group04;

-- =========================================================
-- BƯỚC 1: Cleanup - Xóa dữ liệu test cũ (nếu có)
-- =========================================================
DELETE FROM friend_relationships 
WHERE user_id IN (SELECT user_id FROM users WHERE username IN ('test_user_a', 'test_user_b'))
   OR peer_user_id IN (SELECT user_id FROM users WHERE username IN ('test_user_a', 'test_user_b'));

DELETE FROM user_sessions 
WHERE user_id IN (SELECT user_id FROM users WHERE username IN ('test_user_a', 'test_user_b'));

DELETE FROM users WHERE username IN ('test_user_a', 'test_user_b');

-- =========================================================
-- BƯỚC 2: Tạo 2 người chơi mới
-- =========================================================
INSERT INTO users (username, password, avatar_img)
VALUES 
    ('test_user_a', '$2b$10$dummyhashforpassword123456789012345678901234567890', NULL),
    ('test_user_b', '$2b$10$dummyhashforpassword123456789012345678901234567890', NULL)
RETURNING user_id, username;

-- Lưu user_id vào biến (PostgreSQL không hỗ trợ biến trực tiếp, nên ta sẽ query lại)
-- Hoặc sử dụng CTE để lấy user_id

-- =========================================================
-- BƯỚC 3: Lấy user_id của 2 user vừa tạo
-- =========================================================
DO $$
DECLARE
    user_a_id BIGINT;
    user_b_id BIGINT;
BEGIN
    -- Lấy user_id của test_user_a
    SELECT user_id INTO user_a_id 
    FROM users 
    WHERE username = 'test_user_a';
    
    -- Lấy user_id của test_user_b
    SELECT user_id INTO user_b_id 
    FROM users 
    WHERE username = 'test_user_b';
    
    RAISE NOTICE 'User A ID: %, User B ID: %', user_a_id, user_b_id;
    
    -- =========================================================
    -- BƯỚC 4: User A gửi lời mời kết bạn cho User B
    -- =========================================================
    INSERT INTO friend_relationships (user_id, peer_user_id, status)
    VALUES (user_a_id, user_b_id, 'PENDING')
    ON CONFLICT (user_id, peer_user_id) DO UPDATE 
        SET status = 'PENDING', responded_at = NULL;
    
    RAISE NOTICE 'Step 4: User A sent friend request to User B';
    
    -- =========================================================
    -- BƯỚC 5: Kiểm tra request đã được tạo
    -- =========================================================
    RAISE NOTICE 'Step 5: Checking friend request...';
    RAISE NOTICE 'Friend request status: %', 
        (SELECT status FROM friend_relationships 
         WHERE user_id = user_a_id AND peer_user_id = user_b_id);
    
    -- =========================================================
    -- BƯỚC 6: User B chấp nhận lời mời
    -- =========================================================
    -- 6.1: Update request từ A → B thành ACCEPTED
    UPDATE friend_relationships 
    SET status = 'ACCEPTED', responded_at = NOW()
    WHERE user_id = user_a_id AND peer_user_id = user_b_id AND status = 'PENDING';
    
    RAISE NOTICE 'Step 6.1: Updated A→B relationship to ACCEPTED';
    
    -- 6.2: Tạo reverse relationship B → A với status ACCEPTED
    INSERT INTO friend_relationships (user_id, peer_user_id, status, responded_at)
    VALUES (user_b_id, user_a_id, 'ACCEPTED', NOW())
    ON CONFLICT (user_id, peer_user_id) DO UPDATE 
        SET status = 'ACCEPTED', responded_at = NOW();
    
    RAISE NOTICE 'Step 6.2: Created reverse relationship B→A with ACCEPTED status';
    
    -- =========================================================
    -- BƯỚC 7: Verify kết quả
    -- =========================================================
    RAISE NOTICE 'Step 7: Verifying results...';
    
    -- Kiểm tra relationship A → B
    RAISE NOTICE 'A → B relationship: %', 
        (SELECT status FROM friend_relationships 
         WHERE user_id = user_a_id AND peer_user_id = user_b_id);
    
    -- Kiểm tra relationship B → A
    RAISE NOTICE 'B → A relationship: %', 
        (SELECT status FROM friend_relationships 
         WHERE user_id = user_b_id AND peer_user_id = user_a_id);
    
    -- Kiểm tra cả 2 đều là ACCEPTED
    IF (SELECT COUNT(*) FROM friend_relationships 
        WHERE ((user_id = user_a_id AND peer_user_id = user_b_id) 
            OR (user_id = user_b_id AND peer_user_id = user_a_id))
        AND status = 'ACCEPTED') = 2 THEN
        RAISE NOTICE 'SUCCESS: Both relationships are ACCEPTED!';
    ELSE
        RAISE WARNING 'FAILED: Not both relationships are ACCEPTED!';
    END IF;
    
END $$;

-- =========================================================
-- BƯỚC 8: Hiển thị kết quả cuối cùng
-- =========================================================
SELECT 
    '=== FINAL RESULTS ===' as info;

SELECT 
    u1.username as from_user,
    u2.username as to_user,
    fr.status,
    fr.created_at,
    fr.responded_at
FROM friend_relationships fr
JOIN users u1 ON fr.user_id = u1.user_id
JOIN users u2 ON fr.peer_user_id = u2.user_id
WHERE u1.username IN ('test_user_a', 'test_user_b')
   OR u2.username IN ('test_user_a', 'test_user_b')
ORDER BY fr.created_at;

-- =========================================================
-- BƯỚC 9: Test query để kiểm tra 2 user có là bạn bè
-- =========================================================
SELECT 
    CASE 
        WHEN COUNT(*) = 2 THEN 'YES - They are friends!'
        ELSE 'NO - They are not friends yet'
    END as are_friends
FROM friend_relationships
WHERE ((user_id = (SELECT user_id FROM users WHERE username = 'test_user_a') 
        AND peer_user_id = (SELECT user_id FROM users WHERE username = 'test_user_b'))
    OR (user_id = (SELECT user_id FROM users WHERE username = 'test_user_b') 
        AND peer_user_id = (SELECT user_id FROM users WHERE username = 'test_user_a')))
AND status = 'ACCEPTED';

-- =========================================================
-- KẾT THÚC TEST
-- =========================================================
\echo ''
\echo '========================================'
\echo 'TEST COMPLETED!'
\echo 'Check the results above to verify:'
\echo '1. User A sent request to User B (PENDING)'
\echo '2. User B accepted (both relationships are ACCEPTED)'
\echo '========================================'

