#include "Bishop.h"
#include "ChessBoard.h"

Bishop::Bishop(Color color) : Piece(color, PieceType::BISHOP) {}

char Bishop::getSymbol() const {
    return color == Color::WHITE ? 'B' : 'b';
}

std::vector<Move> Bishop::getPossibleMoves(const Position& pos, ChessBoard& board) {
    std::vector<Move> moves;

    for (int direction : BISHOP_DIRECTIONS) {
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

Piece* Bishop::clone() const {
    Bishop* b = new Bishop(color);
    b->hasMoved = this->hasMoved;
    return b;
}
