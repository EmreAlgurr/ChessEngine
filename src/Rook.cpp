#include "Rook.h"
#include "ChessBoard.h"

Rook::Rook(Color color) : Piece(color, PieceType::ROOK) {}

char Rook::getSymbol() const {
    return color == Color::WHITE ? 'R' : 'r';
}

std::vector<Move> Rook::getPossibleMoves(const Position& pos, ChessBoard& board) {
    std::vector<Move> moves;

    for (int direction : ROOK_DIRECTIONS) {
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

Piece* Rook::clone() const {
    Rook* r = new Rook(color);
    r->hasMoved = this->hasMoved;
    return r;
}
