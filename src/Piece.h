#pragma once

#include "Types.h"
#include "Position.h"
#include "Move.h"
#include <vector>

class ChessBoard; // Forward declaration

// Abstract base class for all chess pieces
class Piece {
public:
    Color color;
    PieceType type;
    bool hasMoved;

    Piece(Color color, PieceType type);
    virtual ~Piece() = default;

    Color getColor() const { return color; }
    PieceType getType() const { return type; }
    bool getHasMoved() const { return hasMoved; }
    void setMoved() { hasMoved = true; }

    virtual std::vector<Move> getPossibleMoves(const Position& pos, ChessBoard& board) = 0;
    virtual char getSymbol() const = 0;

    // Clone piece (for undo or promotion)
    virtual Piece* clone() const = 0;

protected:
    bool isValidPosition(const Position& pos) const;
    bool isEnemyPiece(const Piece* piece) const;
    bool isFriendlyPiece(const Piece* piece) const;
};
