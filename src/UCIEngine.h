#pragma once

#include "BitBoard.h"
#include "SimpleEngine.h"
#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

class UCIEngine {
public:
    UCIEngine();
    void run();
    void handleUCI();

private:
    static constexpr size_t MAX_UCI_TOKENS = 2048;

    BoardState board;
    SimpleEngine engine;
    bool debug;
    int currentColor;
    std::vector<uint64_t> gameKeys; // oyundaki tum pozisyonlarin Zobrist anahtarlari (tekrar tespiti)

    // Pre-allocated zero-copy line & token buffers
    std::string lineBuffer;
    std::array<std::string_view, MAX_UCI_TOKENS> tokenBuffer;

    void handleDebug(std::string_view arg);
    void handleIsReady();
    void handleSetOption(const std::string_view* tokens, size_t count);
    void handleUCINewGame();
    void handlePosition(const std::string_view* tokens, size_t count);
    void handleGo(const std::string_view* tokens, size_t count);
    void handleStop();
    void handleQuit();

    void makeUCIMove(std::string_view uciMove);
    int getCurrentPlayerFromBoard();

    // Zero-copy tokenizer: slices `line` into `outTokens` without allocating strings
    static size_t tokenizeZeroCopy(std::string_view line, std::string_view* outTokens, size_t maxTokens);
};
