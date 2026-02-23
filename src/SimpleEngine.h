#pragma once

#include "ChessBoard.h"
#include "Move.h"
#include "Types.h"
#include <vector>
#include <cstdint>
#include <chrono>

class SimpleEngine {
public:
    SimpleEngine();
    explicit SimpleEngine(int depth);

    void setDepth(int depth);
    Move getBestMove(ChessBoard& board, Color color, long timeLimitMs);
    int64_t getNodesSearched() const;

private:
    int maxDepth;
    static constexpr int MAX_PLY = 128;
    std::chrono::steady_clock::time_point searchEndTime;
    bool stopSearch;
    int64_t nodesSearched;

    // Piece Values
    static constexpr int PAWN_VAL   = 100;
    static constexpr int KNIGHT_VAL = 320;
    static constexpr int BISHOP_VAL = 330;
    static constexpr int ROOK_VAL   = 500;
    static constexpr int QUEEN_VAL  = 900;
    static constexpr int KING_VAL   = 20000;

    // Piece-Square Tables
    static const int PAWN_TABLE[64];
    static const int KNIGHT_TABLE[64];
    static const int BISHOP_TABLE[64];
    static const int ROOK_TABLE[64];
    static const int QUEEN_TABLE[64];
    static const int KING_MG_TABLE[64];  // middlegame
    static const int KING_EG_TABLE[64];  // endgame

    bool isEndgame(ChessBoard& board) const;

    int negamax(ChessBoard& board, int depth, int alpha, int beta, Color color);
    int quiescence(ChessBoard& board, int alpha, int beta, Color color);
    int evaluate(ChessBoard& board, Color activeColor);

    bool isOffBoard(int sq) const;
    int getBasicValue(PieceType t) const;
    int getPsqBonus(Piece* p, int sq) const;

    void orderMoves(std::vector<Move>& moves, ChessBoard& board);
    int scoreMove(const Move& m, ChessBoard& board);
};
