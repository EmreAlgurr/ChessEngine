#pragma once

#include "BitBoard.h"
#include "misc.h"
#include <string>
#include <cstdint>
#include <cstddef>

struct BookEntry {
    uint64_t key;
    uint16_t move;
    uint16_t weight;
    uint32_t learn;
};
static_assert(sizeof(BookEntry) == 16, "Polyglot BookEntry must be 16 bytes");

class PolyglotBook {
public:
    PolyglotBook();
    ~PolyglotBook();

    // Open and memory-map book file (zero-copy)
    bool open(const std::string& filename);
    void close();

    // Is a book currently loaded?
    bool isLoaded() const { return mappedEntries != nullptr && entryCount > 0; }

    // Enable/disable book
    void setEnabled(bool enabled) { bookEnabled = enabled; }
    bool isEnabled() const { return bookEnabled && isLoaded(); }

    // Get best or weighted-random book move for current position
    // Returns MOVE_NONE (0) if no book move found
    Move16 getBookMove(BoardState& board);

    // Compute standard Polyglot 64-bit Zobrist key for a position
    static uint64_t computePolyglotKey(const BoardState& board);

private:
    FD bookFd = FD_ERR;
    map_t bookMapping{};
    const BookEntry* mappedEntries = nullptr;
    size_t entryCount = 0;
    bool bookEnabled;
    std::string currentFile;

    // Convert raw 16-bit Polyglot move to our Move16 format (matching legal moves)
    Move16 convertMove(uint16_t rawMove, BoardState& board);
};

extern PolyglotBook openingBook;
