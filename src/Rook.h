#pragma once
#include "Piece.h"

class Rook : public Piece {
public:
    static constexpr int ROOK_DIRECTIONS[] = {-10, -1, 1, 10};

    explicit Rook(Color color);
    char getSymbol() const override;
    std::vector<Move> getPossibleMoves(const Position& pos, ChessBoard& board) override;
    Piece* clone() const override;
};
