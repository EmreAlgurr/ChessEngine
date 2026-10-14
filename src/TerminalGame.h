#pragma once

#include "BitBoard.h"
#include "SimpleEngine.h"
#include <string>
#include <vector>

class TerminalGame {
public:
    TerminalGame();

    // Main game loop
    void run();

    // Perft testing
    static void runPerftSuite();

private:
    BoardState board;
    SimpleEngine engine;
    int humanColor;
    int currentTurn;
    int engineDepth;

    void showWelcome();
    void showBoard();
    void newGame();
    void handleEngineMove();
    bool handlePlayerMove(const std::string& input);
    bool tryMakeMove(const std::string& moveStr);
    bool checkGameEnd();
    void showLegalMoves();
    void handleSetDepth(const std::string& input);
    void handleSetColor(const std::string& input);
    void handlePerft(const std::string& input);

    // Perft helpers
    static uint64_t perft(BoardState& b, int depth, int color);
    static void perftDivide(BoardState& b, int depth, int color);

    // Utility
    static std::string trim(const std::string& s);
};
