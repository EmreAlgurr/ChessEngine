#pragma once

#include <cstdint>
#include <string>
#include <iostream>
#include "MoveEncoding.h"

// ============================================================
//  Bitboard-Based Board Representation
//  LSB = a1 (bit 0)  ...  MSB = h8 (bit 63)
//  Square index: sq = rank * 8 + file
//    rank 0 = rank 1, file 0 = file a
// ============================================================

// --- Square enum (0..63) ---
enum Square : int {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
    SQ_NONE = 64
};

// --- Side / Color ---
enum Side : int {
    WHITE_SIDE = 0,
    BLACK_SIDE = 1,
    SIDE_COUNT = 2
};

// --- Piece indices for bitboard arrays ---
enum PieceIndex : int {
    PAWN_IDX   = 0,
    KNIGHT_IDX = 1,
    BISHOP_IDX = 2,
    ROOK_IDX   = 3,
    QUEEN_IDX  = 4,
    KING_IDX   = 5,
    PIECE_COUNT = 6
};

// --- Castling rights bitmask ---
enum CastlingRights : int {
    CASTLE_NONE          = 0,
    CASTLE_WHITE_KING    = 1,  // K
    CASTLE_WHITE_QUEEN   = 2,  // Q
    CASTLE_BLACK_KING    = 4,  // k
    CASTLE_BLACK_QUEEN   = 8,  // q
    CASTLE_ALL           = 15
};

// ============================================================
//  Inline Bit Manipulation Utilities
// ============================================================

// Single bit mask for a given square
inline constexpr uint64_t sqBB(int sq) {
    return 1ULL << sq;
}

// Set a bit
inline void setBit(uint64_t& bb, int sq) {
    bb |= sqBB(sq);
}

// Clear a bit
inline void clearBit(uint64_t& bb, int sq) {
    bb &= ~sqBB(sq);
}

// Test a bit
inline bool testBit(uint64_t bb, int sq) {
    return (bb & sqBB(sq)) != 0;
}

// Population count (number of set bits)
inline int popCount(uint64_t bb) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcountll(bb);
#elif defined(_MSC_VER)
    return static_cast<int>(__popcnt64(bb));
#else
    // Fallback: Kernighan's method
    int count = 0;
    while (bb) { bb &= bb - 1; count++; }
    return count;
#endif
}

// Bit scan forward (index of least significant set bit)
// Undefined behaviour if bb == 0
inline int lsb(uint64_t bb) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_ctzll(bb);
#elif defined(_MSC_VER)
    unsigned long idx;
    _BitScanForward64(&idx, bb);
    return static_cast<int>(idx);
#else
    // Fallback: de Bruijn
    static const int index64[64] = {
         0,  1, 48,  2, 57, 49, 28,  3,
        61, 58, 50, 42, 38, 29, 17,  4,
        62, 55, 59, 36, 53, 51, 43, 22,
        45, 39, 33, 30, 24, 18, 12,  5,
        63, 47, 56, 27, 60, 41, 37, 16,
        54, 35, 52, 21, 44, 32, 23, 11,
        46, 26, 40, 15, 34, 20, 31, 10,
        25, 14, 19,  9, 13,  8,  7,  6
    };
    const uint64_t debruijn = 0x03f79d71b4cb0a89ULL;
    return index64[((bb & (uint64_t)(-(int64_t)bb)) * debruijn) >> 58];
#endif
}

// Pop LSB: return index of LSB and clear it
inline int popLsb(uint64_t& bb) {
    int sq = lsb(bb);
    bb &= bb - 1;
    return sq;
}

// Rank and file from square index
inline constexpr int rankOf(int sq) { return sq >> 3; }    // sq / 8
inline constexpr int fileOf(int sq) { return sq & 7; }     // sq % 8

// Square from rank and file
inline constexpr int makeSquare(int rank, int file) { return (rank << 3) | file; }

// ============================================================
//  Undo State — required to fast-rollback non-reversible moves
// ============================================================
struct UndoState {
    int castlingRights;
    int enPassantSq;
    int halfMoveClock;
    int capturedPiece; // PieceIndex (0..5), or -1 if no capture
    uint64_t zobristKey;
};

// ============================================================
//  BoardState  — the pure-data board structure
// ============================================================

struct BoardState {
    // --- 12 piece bitboards [side][piece] ---
    uint64_t pieces[SIDE_COUNT][PIECE_COUNT];

    // --- Aggregate occupancy ---
    uint64_t occupancy[SIDE_COUNT]; // all pieces of one side
    uint64_t allOccupancy;          // all pieces on the board

    // --- Game state ---
    int      sideToMove;     // WHITE_SIDE or BLACK_SIDE
    int      castlingRights; // bitmask (CastlingRights)
    int      enPassantSq;    // SQ_NONE if no en-passant target
    int      halfMoveClock;  // for 50-move rule
    int      fullMoveNumber; // starts at 1, incremented after Black's move
    uint64_t zobristKey;     // Zobrist hash of the current position

    // --- State Mutation ---
    bool makeMove(Move16 m, UndoState& undo);
    void undoMove(Move16 m, const UndoState& undo);
    
    void makeNullMove(UndoState& undo);
    void undoNullMove(const UndoState& undo);
    
    bool hasNonPawnMaterial(int side) const;
    int getKingSquare(int side) const;
    
    // Check if the current side's king is in check (used for verifying pseudo-legality)
    bool isInCheck() const;
    bool isSquareAttacked(int sq, int attackerSide) const;

    // Static Exchange Evaluation & Attackers
    uint64_t getAttackersToSquare(int sq, uint64_t occ) const;
    int getSEE(Move16 m) const;

    // --- Initialisation ---
    void clear();

    // --- Occupancy recalculation ---
    void updateOccupancy();

    // --- Piece query helpers ---
    int  pieceOnSquare(int sq) const;   // returns PieceIndex or -1
    int  colorOnSquare(int sq) const;   // returns Side or -1

    // --- FEN I/O ---
    void setFromFEN(const std::string& fen);
    std::string toFEN() const;

    // --- Debug display ---
    void display() const;
};
