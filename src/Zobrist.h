#pragma once

#include <cstdint>
#include "BitBoard.h"

// ============================================================
//  Zobrist Hashing
//  Used to uniquely identify a board state for the Transposition Table.
// ============================================================

namespace Zobrist {

    extern uint64_t pieceKeys[SIDE_COUNT][PIECE_COUNT][64];
    extern uint64_t sideToMoveKey;
    extern uint64_t castlingKeys[16];
    extern uint64_t enPassantKeys[8]; // One for each file (A-H)

    // MUST be called once at engine startup
    void init();

    // Compute the full hash from scratch (useful for FEN loading)
    uint64_t computeKey(const BoardState& board);

} // namespace Zobrist
