#pragma once

#include "Types.h"
#include "Position.h"
#include <string>

class Piece; // Forward declaration

struct Move {
    Position from;
    Position to;
    Piece* capturedPiece = nullptr;
    bool isEnPassant = false;
    bool isCastling = false;
    PieceType promotionPiece = PieceType::NONE;
    Position prevEnPassantTarget;
    bool prevFromMoved = false;
    bool prevToMoved = false;
    Piece* promotedPiece = nullptr;
    bool prevRookMoved = false;
    int rookFromSquare = 0;

    Move();
    Move(const Position& from, const Position& to);
    Move(const Position& from, const Position& to, PieceType promotionPiece);
    Move(const std::string& fromNotation, const std::string& toNotation);

    std::string toString() const;
    std::string toUCI() const;
};
