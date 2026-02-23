#pragma once
#include "Piece.h"

class Queen : public Piece {
public:
    static constexpr int QUEEN_DIRECTIONS[] = {-11, -10, -9, -1, 1, 9, 10, 11};

    explicit Queen(Color color);
    char getSymbol() const override;
    std::vector<Move> getPossibleMoves(const Position& pos, ChessBoard& board) override;
    Piece* clone() const override;
};
