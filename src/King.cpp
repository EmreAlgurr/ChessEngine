#include "King.h"
#include "ChessBoard.h"
#include <cmath>

King::King(Color color) : Piece(color, PieceType::KING) {}

char King::getSymbol() const {
    return color == Color::WHITE ? 'K' : 'k';
}

std::vector<Move> King::getPossibleMoves(const Position& pos, ChessBoard& board) {
    return getPossibleMoves(pos, board, false);
}

std::vector<Move> King::getPossibleMoves(const Position& pos, ChessBoard& board, bool forAttack) {
    std::vector<Move> moves;

    for (int delta : KING_MOVES) {
        Position newPos(pos.square + delta);
        if (newPos.isValid()) {
            Piece* targetPiece = board.getPiece(newPos);
            if (targetPiece == nullptr || isEnemyPiece(targetPiece)) {
                moves.emplace_back(pos, newPos);
            }
        }
    }

    // Only generate castling moves if not for attack detection
    if (!forAttack && !hasMoved) {
        if (color == Color::WHITE) {
            if (board.canCastleWhiteKingside && canCastleBasic(board, pos, true, forAttack)) {
                moves.emplace_back(pos, Position(pos.square + 2));
            }
            if (board.canCastleWhiteQueenside && canCastleBasic(board, pos, false, forAttack)) {
                moves.emplace_back(pos, Position(pos.square - 2));
            }
        } else {
            if (board.canCastleBlackKingside && canCastleBasic(board, pos, true, forAttack)) {
                moves.emplace_back(pos, Position(pos.square + 2));
            }
            if (board.canCastleBlackQueenside && canCastleBasic(board, pos, false, forAttack)) {
                moves.emplace_back(pos, Position(pos.square - 2));
            }
        }
    }

    return moves;
}

bool King::canCastleBasic(ChessBoard& board, const Position& kingPos, bool kingside, bool forAttack) {
    if (forAttack) return false;

    int rookDelta = kingside ? 3 : -4;
    Position rookPos(kingPos.square + rookDelta);
    Piece* rook = board.getPiece(rookPos);

    if (rook == nullptr || rook->getType() != PieceType::ROOK ||
        rook->getColor() != color || rook->getHasMoved()) {
        return false;
    }

    if (board.isInCheck(color)) return false;

    // Check squares between king and rook are empty
    int step = kingside ? 1 : -1;
    for (int sq = kingPos.square + step;
         kingside ? sq < rookPos.square : sq > rookPos.square;
         sq += step) {
        if (board.getPiece(Position(sq)) != nullptr) return false;
    }

    // Check king doesn't pass through check
    for (int sq = kingPos.square;
         kingside ? sq <= kingPos.square + 2 : sq >= kingPos.square - 2;
         sq += step) {
        if (board.wouldBeInCheck(color, kingPos, Position(sq))) return false;
    }

    return true;
}

Piece* King::clone() const {
    King* k = new King(color);
    k->hasMoved = this->hasMoved;
    return k;
}
