#include "Piece.h"

Piece::Piece(Color color, PieceType type)
    : color(color), type(type), hasMoved(false) {}

bool Piece::isValidPosition(const Position& pos) const {
    return pos.isValid();
}

bool Piece::isEnemyPiece(const Piece* piece) const {
    return piece != nullptr && piece->getColor() != this->color;
}

bool Piece::isFriendlyPiece(const Piece* piece) const {
    return piece != nullptr && piece->getColor() == this->color;
}
