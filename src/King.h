#pragma once
#include "Piece.h"

class King : public Piece {
public:
    static constexpr int KING_MOVES[] = {-11, -10, -9, -1, 1, 9, 10, 11};

    explicit King(Color color);
    char getSymbol() const override;
    std::vector<Move> getPossibleMoves(const Position& pos, ChessBoard& board) override;
    std::vector<Move> getPossibleMoves(const Position& pos, ChessBoard& board, bool forAttack);
    Piece* clone() const override;

private:
    bool canCastleBasic(ChessBoard& board, const Position& kingPos, bool kingside, bool forAttack);
};
