#pragma once

#include "BitBoard.h"
#include "SimpleEngine.h"
#include <string>
#include <vector>

class UCIEngine {
public:
    UCIEngine();
    void run();

private:
    BoardState board;
    SimpleEngine engine;
    bool debug;
    int currentColor;
    std::vector<uint64_t> gameKeys; // oyundaki tum pozisyonlarin Zobrist anahtarlari (tekrar tespiti)

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
    int getCurrentPlayerFromBoard();

    // Split a line into tokens
    static std::vector<std::string> split(const std::string& line);
};
