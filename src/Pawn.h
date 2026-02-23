#pragma once
#include "Piece.h"

class Pawn : public Piece {
public:
    explicit Pawn(Color color);
    char getSymbol() const override;
    std::vector<Move> getPossibleMoves(const Position& pos, ChessBoard& board) override;
    Piece* clone() const override;
};
