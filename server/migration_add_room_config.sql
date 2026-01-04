-- Migration: Add room configuration for 1vN mode
-- Adds fields to store question count per difficulty level

-- Add columns if not exist
ALTER TABLE room
ADD COLUMN IF NOT EXISTS easy_count INT NOT NULL DEFAULT 0,
ADD COLUMN IF NOT EXISTS medium_count INT NOT NULL DEFAULT 0,
ADD COLUMN IF NOT EXISTS hard_count INT NOT NULL DEFAULT 0;

-- Add constraint if not exists
DO $$
BEGIN
    IF NOT EXISTS (
        SELECT 1 FROM pg_constraint 
        WHERE conname = 'chk_room_question_total' 
        AND conrelid = 'room'::regclass
    ) THEN
        ALTER TABLE room
        ADD CONSTRAINT chk_room_question_total 
        CHECK (easy_count + medium_count + hard_count <= 30);
    END IF;
END $$;

-- Update existing rooms to have default values
UPDATE room SET easy_count = 5, medium_count = 5, hard_count = 5 WHERE easy_count = 0 AND medium_count = 0 AND hard_count = 0;

-- =========================================================
-- Create user_stats table if not exists (for stats tracking)
-- =========================================================
CREATE TABLE IF NOT EXISTS user_stats (
    user_id         BIGINT PRIMARY KEY,
    quickmode_games BIGINT NOT NULL DEFAULT 0,
    onevn_games     BIGINT NOT NULL DEFAULT 0,
    quickmode_wins  BIGINT NOT NULL DEFAULT 0,
    onevn_wins      BIGINT NOT NULL DEFAULT 0,
    CONSTRAINT fk_user_stats_user
        FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE
);

-- Migrate existing stats from users table to user_stats table (if stats columns exist in users table)
DO $$
BEGIN
    -- Check if stats columns exist in users table
    IF EXISTS (
        SELECT 1 FROM information_schema.columns 
        WHERE table_name = 'users' 
        AND column_name = 'quickmode_games'
    ) THEN
        -- Migrate stats from users to user_stats
        INSERT INTO user_stats (user_id, quickmode_games, quickmode_wins, onevn_games, onevn_wins)
        SELECT user_id, 
               COALESCE(quickmode_games, 0), 
               COALESCE(quickmode_wins, 0), 
               COALESCE(onevn_games, 0), 
               COALESCE(onevn_wins, 0)
        FROM users
        WHERE user_id NOT IN (SELECT user_id FROM user_stats)
        ON CONFLICT (user_id) DO NOTHING;
    ELSE
        -- If columns don't exist, just create empty entries for existing users
        INSERT INTO user_stats (user_id)
        SELECT user_id
        FROM users
        WHERE user_id NOT IN (SELECT user_id FROM user_stats)
        ON CONFLICT (user_id) DO NOTHING;
    END IF;
END $$;

