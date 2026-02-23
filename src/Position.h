#pragma once

#include <string>

// Position class for 120-square mailbox
class Position {
public:
    int square; // 0-119 mailbox index

    Position();
    explicit Position(int square);
    Position(int row, int col);
    explicit Position(const std::string& notation);

    bool isValid() const;
    int getRow() const;
    int getCol() const;
    std::string toNotation() const;

    bool operator==(const Position& other) const;
    bool operator!=(const Position& other) const;

    // Static cache access (replaces Java's Position.get())
    static const Position& get(int square);
    static const Position& get(int row, int col);

private:
    static Position CACHE[120];
    static bool cacheInitialized;
    static void initCache();
};
