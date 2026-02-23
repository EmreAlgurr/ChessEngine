#include "Move.h"

Move::Move() : from(0), to(0) {}

Move::Move(const Position& from, const Position& to)
    : from(from), to(to) {}

Move::Move(const Position& from, const Position& to, PieceType promotionPiece)
    : from(from), to(to), promotionPiece(promotionPiece) {}

Move::Move(const std::string& fromNotation, const std::string& toNotation)
    : from(Position(fromNotation)), to(Position(toNotation)) {}

std::string Move::toString() const {
    return from.toNotation() + "-" + to.toNotation();
}

std::string Move::toUCI() const {
    std::string uciMove = from.toNotation() + to.toNotation();

    if (promotionPiece != PieceType::NONE) {
        switch (promotionPiece) {
            case PieceType::QUEEN:  uciMove += "q"; break;
            case PieceType::ROOK:   uciMove += "r"; break;
            case PieceType::BISHOP: uciMove += "b"; break;
            case PieceType::KNIGHT: uciMove += "n"; break;
            default: break;
        }
    }

    return uciMove;
}
