@echo off
echo ========================================
echo   Chess Engine C++ Build Script
echo ========================================

where g++ >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo [INFO] Using g++ compiler...
    g++ -std=c++17 -O2 -o ChessEngine.exe ^
        src/Position.cpp ^
        src/Move.cpp ^
        src/Piece.cpp ^
        src/King.cpp ^
        src/Queen.cpp ^
        src/Rook.cpp ^
        src/Bishop.cpp ^
        src/Knight.cpp ^
        src/Pawn.cpp ^
        src/ChessBoard.cpp ^
        src/SimpleEngine.cpp ^
        src/UCIEngine.cpp ^
        src/TerminalGame.cpp ^
        src/main.cpp
    if %ERRORLEVEL% EQU 0 (
        echo [SUCCESS] Built ChessEngine.exe
    ) else (
        echo [ERROR] Build failed!
    )
    goto :eof
)

where cl >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo [INFO] Using MSVC compiler...
    cl /std:c++17 /EHsc /O2 /Fe:ChessEngine.exe ^
        src/Position.cpp ^
        src/Move.cpp ^
        src/Piece.cpp ^
        src/King.cpp ^
        src/Queen.cpp ^
        src/Rook.cpp ^
        src/Bishop.cpp ^
        src/Knight.cpp ^
        src/Pawn.cpp ^
        src/ChessBoard.cpp ^
        src/SimpleEngine.cpp ^
        src/UCIEngine.cpp ^
        src/TerminalGame.cpp ^
        src/main.cpp
    if %ERRORLEVEL% EQU 0 (
        echo [SUCCESS] Built ChessEngine.exe
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
