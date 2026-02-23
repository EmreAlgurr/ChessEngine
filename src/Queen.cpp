#include "Queen.h"
#include "ChessBoard.h"

Queen::Queen(Color color) : Piece(color, PieceType::QUEEN) {}

char Queen::getSymbol() const {
    return color == Color::WHITE ? 'Q' : 'q';
}

std::vector<Move> Queen::getPossibleMoves(const Position& pos, ChessBoard& board) {
    std::vector<Move> moves;

    for (int direction : QUEEN_DIRECTIONS) {
        for (int sq = pos.square + direction; ; sq += direction) {
            Position newPos(sq);
            if (!newPos.isValid()) break;

            Piece* targetPiece = board.getPiece(newPos);
            if (targetPiece == nullptr) {
                moves.emplace_back(pos, newPos);
            } else {
                if (isEnemyPiece(targetPiece)) {
                    moves.emplace_back(pos, newPos);
                }
                break;
            }
        }
    }
    return moves;
}

Piece* Queen::clone() const {
    Queen* q = new Queen(color);
    q->hasMoved = this->hasMoved;
    return q;
}
