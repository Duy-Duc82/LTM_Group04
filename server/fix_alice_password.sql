-- Fix alice password to alice123
-- Usage: psql -U postgres -d ltm_group04 -f server/fix_alice_password.sql

-- Update alice's password to alice123
UPDATE users 
SET password = 'alice123'
WHERE username = 'alice';

-- Verify the update
SELECT '=== ALICE ACCOUNT ===' AS info;
SELECT user_id, username, password 
FROM users 
WHERE username = 'alice';

-- If there are multiple alice accounts (shouldn't happen due to unique constraint),
-- you might need to delete the old one first:
-- DELETE FROM users WHERE username = 'alice' AND password = '123';

