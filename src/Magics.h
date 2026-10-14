#pragma once

#include "BitBoard.h"
#include <cstdint>

// ============================================================
//  Magic Bitboards for Sliding Pieces
// ============================================================

namespace Magics {

    // Magic Numbers and Shift Amounts for Rooks and Bishops
    extern uint64_t RookMagics[64];
    extern uint64_t BishopMagics[64];
    
    extern const int RookShifts[64];
    extern const int BishopShifts[64];

    // Offsets into the global attack tables
    extern const int RookOffsets[64];
    extern const int BishopOffsets[64];

    // Global Pre-Calculated Attack Tables
    // Size required: 102400 for rooks, 5248 for bishops
    // We allocate a single large array for each to hold all variations
    extern uint64_t RookAttacksTable[102400];
    extern uint64_t BishopAttacksTable[10240];

    // Masks for the relevant occupancy bits (excluding edges)
    extern uint64_t RookMasks[64];
    extern uint64_t BishopMasks[64];

    // ============================================================
    //  Initialization function
    //  MUST be called once at engine startup!
    // ============================================================
    void initMagics();

    // ============================================================
    //  O(1) Attack Lookups
    // ============================================================
    
    // Get rook attacks for a given square and current board occupancy
    inline uint64_t getRookAttacks(int sq, uint64_t occupancy) {
        occupancy &= RookMasks[sq];
        int index = (occupancy * RookMagics[sq]) >> RookShifts[sq];
        return RookAttacksTable[RookOffsets[sq] + index];
    }

    // Get bishop attacks for a given square and current board occupancy
    inline uint64_t getBishopAttacks(int sq, uint64_t occupancy) {
        occupancy &= BishopMasks[sq];
        int index = (occupancy * BishopMagics[sq]) >> BishopShifts[sq];
        return BishopAttacksTable[BishopOffsets[sq] + index];
    }

    // Get queen attacks (Queen = Rook + Bishop)
    inline uint64_t getQueenAttacks(int sq, uint64_t occupancy) {
        return getRookAttacks(sq, occupancy) | getBishopAttacks(sq, occupancy);
    }

} // namespace Magics
