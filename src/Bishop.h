#pragma once
#include "Piece.h"

class Bishop : public Piece {
public:
    static constexpr int BISHOP_DIRECTIONS[] = {-11, -9, 9, 11};

    explicit Bishop(Color color);
    char getSymbol() const override;
    std::vector<Move> getPossibleMoves(const Position& pos, ChessBoard& board) override;
    Piece* clone() const override;
};
