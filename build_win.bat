@echo off
:: BLUSK Windows build script
:: Uses the default CMake generator configured on this machine.

set BUILD_DIR=build_win

echo [BLUSK] Configuring...
cmake -S . -B %BUILD_DIR% ^
    -DCMAKE_BUILD_TYPE=Release

if %errorlevel% neq 0 (
    echo [BLUSK] CMake configure failed.
    pause
    exit /b 1
)

echo [BLUSK] Building...
cmake --build %BUILD_DIR% --config Release --parallel

if %errorlevel% neq 0 (
    echo [BLUSK] Build failed.
    pause
    exit /b 1
)

echo.
echo [BLUSK] Build succeeded!
echo   Executable: %BUILD_DIR%\blusk.exe
echo.
echo Usage:
echo   blusk.exe examples\01_basic.blusk
echo   blusk.exe --repl
echo   blusk.exe --version
pause
