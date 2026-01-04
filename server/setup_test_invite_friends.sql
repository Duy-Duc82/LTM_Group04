-- Setup friend relationships for invite flow test
-- Make sure alice and bob are friends

-- Get alice and bob user_id
DO $$
DECLARE
    alice_id INT;
    bob_id INT;
BEGIN
    SELECT user_id INTO alice_id FROM users WHERE username = 'alice';
    SELECT user_id INTO bob_id FROM users WHERE username = 'bob';
    
    IF alice_id IS NULL OR bob_id IS NULL THEN
        RAISE EXCEPTION 'Alice or Bob not found in database';
    END IF;
    
    -- Insert friend relationship (both directions)
    INSERT INTO friend_relationships (user_id, peer_user_id, status, responded_at)
    VALUES 
        (alice_id, bob_id, 'ACCEPTED', NOW()),
        (bob_id, alice_id, 'ACCEPTED', NOW())
    ON CONFLICT (user_id, peer_user_id) 
    DO UPDATE SET status = 'ACCEPTED', responded_at = NOW();
    
    RAISE NOTICE 'Alice (%) and Bob (%) are now friends', alice_id, bob_id;
END $$;
