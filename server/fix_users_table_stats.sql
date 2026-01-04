-- Script to ensure users table has stats columns
-- Run this if users table doesn't have quickmode_games, quickmode_wins, onevn_games, onevn_wins columns

-- Check if columns exist, if not add them
DO $$
BEGIN
    -- Add quickmode_games if not exists
    IF NOT EXISTS (
        SELECT 1 FROM information_schema.columns 
        WHERE table_name = 'users' AND column_name = 'quickmode_games'
    ) THEN
        ALTER TABLE users ADD COLUMN quickmode_games INT NOT NULL DEFAULT 0;
        RAISE NOTICE 'Added column quickmode_games';
    END IF;

    -- Add quickmode_wins if not exists
    IF NOT EXISTS (
        SELECT 1 FROM information_schema.columns 
        WHERE table_name = 'users' AND column_name = 'quickmode_wins'
    ) THEN
        ALTER TABLE users ADD COLUMN quickmode_wins INT NOT NULL DEFAULT 0;
        RAISE NOTICE 'Added column quickmode_wins';
    END IF;

    -- Add onevn_games if not exists
    IF NOT EXISTS (
        SELECT 1 FROM information_schema.columns 
        WHERE table_name = 'users' AND column_name = 'onevn_games'
    ) THEN
        ALTER TABLE users ADD COLUMN onevn_games INT NOT NULL DEFAULT 0;
        RAISE NOTICE 'Added column onevn_games';
    END IF;

    -- Add onevn_wins if not exists
    IF NOT EXISTS (
        SELECT 1 FROM information_schema.columns 
        WHERE table_name = 'users' AND column_name = 'onevn_wins'
    ) THEN
        ALTER TABLE users ADD COLUMN onevn_wins INT NOT NULL DEFAULT 0;
        RAISE NOTICE 'Added column onevn_wins';
    END IF;
END $$;

-- Verify columns exist
SELECT 
    column_name, 
    data_type, 
    column_default
FROM information_schema.columns 
WHERE table_name = 'users' 
  AND column_name IN ('quickmode_games', 'quickmode_wins', 'onevn_games', 'onevn_wins')
ORDER BY column_name;

