#pragma once

#include "BitBoard.h"

// ============================================================
//  Pre-calculated Attack Masks for Leaping Pieces
//  Completely computed at compile-time via constexpr
// ============================================================

namespace Attacks {

    // --- File Masks ---
    // Using bitboards to mask out illegal wraps across the board edges
    constexpr uint64_t FILE_A = 0x0101010101010101ULL;
    constexpr uint64_t FILE_B = 0x0202020202020202ULL;
    constexpr uint64_t FILE_G = 0x4040404040404040ULL;
    constexpr uint64_t FILE_H = 0x8080808080808080ULL;

    constexpr uint64_t NOT_A  = ~FILE_A;
    constexpr uint64_t NOT_H  = ~FILE_H;
    constexpr uint64_t NOT_AB = ~(FILE_A | FILE_B);
    constexpr uint64_t NOT_GH = ~(FILE_G | FILE_H);

    // ============================================================
    //  Compile-time Generators
    // ============================================================

    constexpr uint64_t computeKnightAttacks(int sq) {
        uint64_t bb = sqBB(sq);
        uint64_t attacks = 0;
        
        // Up directions
        attacks |= (bb << 17) & NOT_A;   // Up 2, Right 1
        attacks |= (bb << 15) & NOT_H;   // Up 2, Left 1
        attacks |= (bb << 10) & NOT_AB;  // Up 1, Right 2
        attacks |= (bb <<  6) & NOT_GH;  // Up 1, Left 2
        
        // Down directions
        attacks |= (bb >> 17) & NOT_H;   // Down 2, Left 1
        attacks |= (bb >> 15) & NOT_A;   // Down 2, Right 1
        attacks |= (bb >> 10) & NOT_GH;  // Down 1, Left 2
        attacks |= (bb >>  6) & NOT_AB;  // Down 1, Right 2

        return attacks;
    }

    constexpr uint64_t computeKingAttacks(int sq) {
        uint64_t bb = sqBB(sq);
        uint64_t attacks = 0;
        
        // 1 step Left & Right
        attacks |= (bb << 1) & NOT_A;
        attacks |= (bb >> 1) & NOT_H;

        // Base pattern (King + left/right)
        uint64_t w = bb | ((bb << 1) & NOT_A) | ((bb >> 1) & NOT_H);
        
        // Up & Down rows
        attacks |= (w << 8); // Up
        attacks |= (w >> 8); // Down

        return attacks;
    }

    constexpr uint64_t computePawnAttacks(int side, int sq) {
        uint64_t bb = sqBB(sq);
        uint64_t attacks = 0;

        if (side == WHITE_SIDE) {
            attacks |= (bb << 9) & NOT_A;  // Up-Right
            attacks |= (bb << 7) & NOT_H;  // Up-Left
        } else {
            attacks |= (bb >> 7) & NOT_A;  // Down-Right
            attacks |= (bb >> 9) & NOT_H;  // Down-Left
        }
        return attacks;
    }

    // ============================================================
    //  Storage Array populated at Compile Time
    // ============================================================
    struct LeaperAttacksData {
        uint64_t knight[64];
        uint64_t king[64];
        uint64_t pawn[2][64]; // [side][square]

        constexpr LeaperAttacksData() : knight{0}, king{0}, pawn{{0}, {0}} {
            for (int sq = 0; sq < 64; ++sq) {
                knight[sq] = computeKnightAttacks(sq);
                king[sq] = computeKingAttacks(sq);
                pawn[WHITE_SIDE][sq] = computePawnAttacks(WHITE_SIDE, sq);
                pawn[BLACK_SIDE][sq] = computePawnAttacks(BLACK_SIDE, sq);
            }
        }
    };

    // Global inline constexpr instance (C++17 feature)
    // The compiler will evaluate all 256 masks during compilation
    // and store them in the binary data section directly.
    inline constexpr LeaperAttacksData leaper = LeaperAttacksData();

    // ============================================================
    //  O(1) Accessors
    // ============================================================
    inline constexpr uint64_t getKnightAttacks(int sq) { 
        return leaper.knight[sq]; 
    }
    
    inline constexpr uint64_t getKingAttacks(int sq) { 
        return leaper.king[sq]; 
    }
    
    inline constexpr uint64_t getPawnAttacks(int side, int sq) { 
        return leaper.pawn[side][sq]; 
    }

} // namespace Attacks
