#pragma once

enum class Color {
    WHITE,
    BLACK
};

enum class PieceType {
    KING,
    QUEEN,
    ROOK,
    BISHOP,
    KNIGHT,
    PAWN,
    NONE
};

// Helper to get ordinal (0-5), matching Java's ordinal()
inline int pieceTypeOrdinal(PieceType t) {
    switch (t) {
        case PieceType::KING:   return 0;
        case PieceType::QUEEN:  return 1;
        case PieceType::ROOK:   return 2;
        case PieceType::BISHOP: return 3;
        case PieceType::KNIGHT: return 4;
        case PieceType::PAWN:   return 5;
        default:                return -1;
    }
}
