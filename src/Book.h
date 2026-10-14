#pragma once

#include "BitBoard.h"
#include <string>
#include <vector>
#include <cstdint>

struct BookEntry {
    uint64_t key;
    uint16_t move;
    uint16_t weight;
    uint32_t learn;
};

class PolyglotBook {
public:
    PolyglotBook();
    ~PolyglotBook();

    // Open and load book file into memory
    bool open(const std::string& filename);
    void close();

    // Is a book currently loaded?
    bool isLoaded() const { return !entries.empty(); }

    // Enable/disable book
    void setEnabled(bool enabled) { bookEnabled = enabled; }
    bool isEnabled() const { return bookEnabled && isLoaded(); }

    // Get best or weighted-random book move for current position
    // Returns MOVE_NONE (0) if no book move found
    Move16 getBookMove(BoardState& board);

    // Compute standard Polyglot 64-bit Zobrist key for a position
    static uint64_t computePolyglotKey(const BoardState& board);

private:
    std::vector<BookEntry> entries;
    bool bookEnabled;
    std::string currentFile;

    // Convert raw 16-bit Polyglot move to our Move16 format (matching legal moves)
    Move16 convertMove(uint16_t rawMove, BoardState& board);
};

extern PolyglotBook openingBook;
