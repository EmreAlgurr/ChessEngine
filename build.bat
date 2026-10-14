@echo off
echo ========================================
echo   Chess Engine C++ Build Script
echo ========================================

REM Kullanim: build.bat [cikti_adi.exe]   (varsayilan: ChessEngine.exe)
set OUT=%~1
if "%OUT%"=="" set OUT=ChessEngine.exe

where g++ >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo [INFO] Using g++ compiler...
    g++ -std=c++17 -O2 -o %OUT% ^
        src/BitBoard.cpp ^
        src/Magics.cpp ^
        src/Zobrist.cpp ^
        src/TranspositionTable.cpp ^
        src/MoveGenerator.cpp ^
        src/SimpleEngine.cpp ^
        src/EvalParams.cpp ^
        src/Tuning.cpp ^
        src/Book.cpp ^
        src/UCIEngine.cpp ^
        src/TerminalGame.cpp ^
        src/main.cpp
    if %ERRORLEVEL% EQU 0 (
        echo [SUCCESS] Built %OUT%
    ) else (
        echo [ERROR] Build failed!
    )
    goto :eof
)

where cl >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo [INFO] Using MSVC compiler...
    cl /std:c++17 /EHsc /O2 /Fe:%OUT% ^
        src/BitBoard.cpp ^
        src/Magics.cpp ^
        src/MoveGenerator.cpp ^
        src/Zobrist.cpp ^
        src/TranspositionTable.cpp ^
        src/SimpleEngine.cpp ^
        src/EvalParams.cpp ^
        src/Tuning.cpp ^
        src/Book.cpp ^
        src/UCIEngine.cpp ^
        src/TerminalGame.cpp ^
        src/main.cpp
    if %ERRORLEVEL% EQU 0 (
        echo [SUCCESS] Built %OUT%
    ) else (
        echo [ERROR] Build failed!
    )
    goto :eof
)

echo [ERROR] No C++ compiler found!
echo Please install one of the following:
echo   - MinGW-w64 (g++): https://www.mingw-w64.org/
echo   - MSVC (Visual Studio): https://visualstudio.microsoft.com/
echo   - Or use CMake with: cmake -B build ^&^& cmake --build build
