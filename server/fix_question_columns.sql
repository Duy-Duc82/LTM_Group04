-- Fix question table columns to use quoted names
-- Run this if you get "column opA does not exist" error

-- Option 1: If table already exists with lowercase columns, rename them
DO $$
BEGIN
    -- Check if columns exist as lowercase
    IF EXISTS (
        SELECT 1 FROM information_schema.columns 
        WHERE table_name = 'question' AND column_name = 'opa'
    ) THEN
        -- Rename lowercase columns to quoted uppercase
        ALTER TABLE question RENAME COLUMN opa TO "opA";
        ALTER TABLE question RENAME COLUMN opb TO "opB";
        ALTER TABLE question RENAME COLUMN opc TO "opC";
        ALTER TABLE question RENAME COLUMN opd TO "opD";
        RAISE NOTICE 'Renamed columns from lowercase to quoted uppercase';
    ELSE
        RAISE NOTICE 'Columns already have correct names or table does not exist';
    END IF;
END $$;

-- Option 2: If you want to recreate the table (WARNING: This will delete all questions!)
-- Uncomment the following lines:
/*
DROP TABLE IF EXISTS question CASCADE;

CREATE TABLE question (
  question_id      BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  difficulty_level TEXT NOT NULL CHECK (difficulty_level IN ('EASY','MEDIUM','HARD')),
  content          TEXT NOT NULL,
  "opA"            VARCHAR(512) NOT NULL,
  "opB"            VARCHAR(512) NOT NULL,
  "opC"            VARCHAR(512) NOT NULL,
  "opD"            VARCHAR(512) NOT NULL,
  correct_op       CHAR(1) NOT NULL CHECK (correct_op IN ('A','B','C','D')),
  explanation      TEXT NULL
);

CREATE INDEX idx_question_diff ON question(difficulty_level);
*/
