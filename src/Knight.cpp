#include "Knight.h"
#include "ChessBoard.h"

Knight::Knight(Color color) : Piece(color, PieceType::KNIGHT) {}

char Knight::getSymbol() const {
    return color == Color::WHITE ? 'N' : 'n';
}

std::vector<Move> Knight::getPossibleMoves(const Position& pos, ChessBoard& board) {
    std::vector<Move> moves;

    for (int delta : KNIGHT_MOVES) {
        Position newPos(pos.square + delta);
        if (newPos.isValid()) {
            Piece* targetPiece = board.getPiece(newPos);
            if (targetPiece == nullptr || isEnemyPiece(targetPiece)) {
                moves.emplace_back(pos, newPos);
            }
        }
    }
    return moves;
}

Piece* Knight::clone() const {
    Knight* n = new Knight(color);
    n->hasMoved = this->hasMoved;
    return n;
}
