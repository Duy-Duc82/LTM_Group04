-- Migration: Add eliminated column to room_members
-- This allows us to mark players as eliminated without removing them from the table
-- during an in-progress game

ALTER TABLE room_members ADD COLUMN eliminated BOOLEAN DEFAULT FALSE;

-- Verify
SELECT column_name, data_type FROM information_schema.columns 
WHERE table_name='room_members' ORDER BY ordinal_position;
