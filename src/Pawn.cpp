#include "Pawn.h"
#include "ChessBoard.h"

Pawn::Pawn(Color color) : Piece(color, PieceType::PAWN) {}

char Pawn::getSymbol() const {
    return color == Color::WHITE ? 'P' : 'p';
}

std::vector<Move> Pawn::getPossibleMoves(const Position& pos, ChessBoard& board) {
    std::vector<Move> moves;
    int direction = (color == Color::WHITE) ? -10 : 10;

    // Forward move
    Position oneForward(pos.square + direction);
    if (oneForward.isValid() && board.getPiece(oneForward) == nullptr) {
        int promotionRow = (color == Color::WHITE) ? 0 : 7;
        if (oneForward.getRow() == promotionRow) {
            moves.emplace_back(pos, oneForward, PieceType::QUEEN);
            moves.emplace_back(pos, oneForward, PieceType::ROOK);
            moves.emplace_back(pos, oneForward, PieceType::BISHOP);
            moves.emplace_back(pos, oneForward, PieceType::KNIGHT);
        } else {
            moves.emplace_back(pos, oneForward);
            // Two squares forward from starting position
            int startRow = (color == Color::WHITE) ? 6 : 1;
            if (pos.getRow() == startRow) {
                Position twoForward(pos.square + 2 * direction);
                if (board.getPiece(twoForward) == nullptr) {
                    moves.emplace_back(pos, twoForward);
                }
            }
        }
    }

    // Captures (including promotions)
    int captureDirs[] = {direction - 1, direction + 1};
    for (int captureDir : captureDirs) {
        Position diag(pos.square + captureDir);
        if (diag.isValid()) {
            Piece* target = board.getPiece(diag);

            // Regular capture
            if (target != nullptr && isEnemyPiece(target)) {
                int promotionRow = (color == Color::WHITE) ? 0 : 7;
                if (diag.getRow() == promotionRow) {
                    moves.emplace_back(pos, diag, PieceType::QUEEN);
                    moves.emplace_back(pos, diag, PieceType::ROOK);
                    moves.emplace_back(pos, diag, PieceType::BISHOP);
                    moves.emplace_back(pos, diag, PieceType::KNIGHT);
                } else {
                    moves.emplace_back(pos, diag);
                }
            }
            // En passant
            else if (target == nullptr && board.getEnPassantTarget() != nullptr &&
                     *board.getEnPassantTarget() == diag) {
                Move enPassantMove(pos, diag);
                enPassantMove.isEnPassant = true;
                moves.push_back(enPassantMove);
            }
        }
    }
    return moves;
}

Piece* Pawn::clone() const {
    Pawn* p = new Pawn(color);
    p->hasMoved = this->hasMoved;
    return p;
}
