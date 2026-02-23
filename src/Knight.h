#pragma once
#include "Piece.h"

class Knight : public Piece {
public:
    static constexpr int KNIGHT_MOVES[] = {-21, -19, -12, -8, 8, 12, 19, 21};

    explicit Knight(Color color);
    char getSymbol() const override;
    std::vector<Move> getPossibleMoves(const Position& pos, ChessBoard& board) override;
    Piece* clone() const override;
};
