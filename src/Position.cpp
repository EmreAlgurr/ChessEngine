#include "Position.h"

// Static members - CACHE uses default constructor which sets square=0
// We'll properly initialize in initCache which is called from get()
Position Position::CACHE[120];
bool Position::cacheInitialized = false;

void Position::initCache() {
    if (cacheInitialized) return;
    cacheInitialized = true;
    for (int i = 0; i < 120; i++) {
        CACHE[i].square = i;
    }
}

Position::Position() : square(0) {}

Position::Position(int square) : square(square) {}

Position::Position(int row, int col) {
    this->square = 21 + row * 10 + col;
}

Position::Position(const std::string& notation) {
    int col = notation[0] - 'a';
    int row = 8 - (notation[1] - '0');
    this->square = 21 + row * 10 + col;
}

bool Position::isValid() const {
    return square >= 21 && square <= 98 && (square % 10) >= 1 && (square % 10) <= 8;
}

int Position::getRow() const {
    return (square - 21) / 10;
}

int Position::getCol() const {
    return (square - 21) % 10;
}

std::string Position::toNotation() const {
    int row = getRow();
    int col = getCol();
    std::string result;
    result += static_cast<char>('a' + col);
    result += static_cast<char>('0' + (8 - row));
    return result;
}

bool Position::operator==(const Position& other) const {
    return square == other.square;
}

bool Position::operator!=(const Position& other) const {
    return square != other.square;
}

const Position& Position::get(int square) {
    initCache();
    if (square >= 0 && square < 120) {
        return CACHE[square];
    }
    // Fallback - return a static default (shouldn't happen in normal use)
    static Position fallback(0);
    fallback.square = square;
    return fallback;
}

const Position& Position::get(int row, int col) {
    int sq = 21 + row * 10 + col;
    return get(sq);
}
