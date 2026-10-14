#pragma once

#include "BitBoard.h"
#include "MoveEncoding.h"
#include "LeaperAttacks.h"
#include "Magics.h"

// ============================================================
//  Fast Stack-Allocated Move List
// ============================================================

struct MoveList {
    Move16 moves[256];
    int count = 0;

    inline void add(Move16 m) {
        moves[count++] = m;
    }
    
    // Range-for loop compatibility
    Move16* begin() { return moves; }
    Move16* end()   { return moves + count; }
    const Move16* begin() const { return moves; }
    const Move16* end() const { return moves + count; }
};

// ============================================================
//  Pseudo-Legal Move Generation
// ============================================================

namespace MoveGen {

    // Bitboard masks for ranks
    constexpr uint64_t RANK_1 = 0x00000000000000FFULL;
    constexpr uint64_t RANK_2 = 0x000000000000FF00ULL;
    constexpr uint64_t RANK_3 = 0x0000000000FF0000ULL;
    constexpr uint64_t RANK_4 = 0x00000000FF000000ULL;
    constexpr uint64_t RANK_5 = 0x000000FF00000000ULL;
    constexpr uint64_t RANK_6 = 0x0000FF0000000000ULL;
    constexpr uint64_t RANK_7 = 0x00FF000000000000ULL;
    constexpr uint64_t RANK_8 = 0xFF00000000000000ULL;

    // Generate all pseudo-legal moves for the current side to move
    void generateAllMoves(const BoardState& board, MoveList& moveList);
    
    // Generate only tactical (captures + promotions) pseudo-legal moves (for Quiescence Search)
    void generateTacticalMoves(const BoardState& board, MoveList& moveList);

} // namespace MoveGen
