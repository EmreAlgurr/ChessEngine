#pragma once

#include "ChessBoard.h"
#include "SimpleEngine.h"
#include <string>

class UCIEngine {
public:
    UCIEngine();
    void run();

private:
    ChessBoard board;
    SimpleEngine engine;
    bool debug;
    Color currentColor;

    void handleUCI();
    void handleDebug(const std::string& arg);
    void handleIsReady();
    void handleSetOption(const std::vector<std::string>& tokens);
    void handleUCINewGame();
    void handlePosition(const std::vector<std::string>& tokens);
    void handleGo(const std::vector<std::string>& tokens);
    void handleStop();
    void handleQuit();

    void makeUCIMove(const std::string& uciMove);
    Color getCurrentPlayerFromBoard();
    std::string moveToUCI(const Move& move);

    // Split a line into tokens
    static std::vector<std::string> split(const std::string& line);
};
