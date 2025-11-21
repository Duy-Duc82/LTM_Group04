@echo off
echo ===== Compiling test_dao =====

REM ---- CHỈNH ĐƯỜNG DẪN POSTGRESQL TẠI ĐÂY ---
set PG_INC="C:\Program Files\PostgreSQL\16\include"
set PG_LIB="C:\Program Files\PostgreSQL\16\lib"

gcc -std=c11 -Wall -Wextra ^
    ..\src\db.c ^
    ..\src\dao\dao_users.c ^
    ..\src\dao\dao_sessions.c ^
    test_dao.c ^
    -I..\include ^
    -I..\include\dao ^
    -I%PG_INC% ^
    -L%PG_LIB% ^
    -lpq ^
    -o test_dao.exe

if %ERRORLEVEL% NEQ 0 (
    echo Compile FAILED
    pause
    exit /b 1
)

echo ===== Running test_dao =====
test_dao.exe
pause
