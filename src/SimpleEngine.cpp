#include "SimpleEngine.h"
#include <algorithm>
#include <iostream>

const int SimpleEngine::PAWN_TABLE[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
     5,  5, 10, 25, 25, 10,  5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5, -5,-10,  0,  0,-10, -5,  5,
     5, 10, 10,-20,-20, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0
};

const int SimpleEngine::KNIGHT_TABLE[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};

const int SimpleEngine::BISHOP_TABLE[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};

const int SimpleEngine::ROOK_TABLE[64] = {
      0,  0,  0,  0,  0,  0,  0,  0,
      5, 10, 10, 10, 10, 10, 10,  5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
      0,  0,  0,  5,  5,  0,  0,  0
};

const int SimpleEngine::QUEEN_TABLE[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};

const int SimpleEngine::KING_MG_TABLE[64] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20
};

const int SimpleEngine::KING_EG_TABLE[64] = {
    -50,-40,-30,-20,-20,-30,-40,-50,
    -30,-20,-10,  0,  0,-10,-20,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -50,-30,-30,-30,-30,-30,-30,-50
};

SimpleEngine::SimpleEngine() : maxDepth(4), stopSearch(false), nodesSearched(0) {}

SimpleEngine::SimpleEngine(int depth) : maxDepth(depth), stopSearch(false), nodesSearched(0) {}

void SimpleEngine::setDepth(int depth) {
    maxDepth = depth;
}

Move SimpleEngine::getBestMove(ChessBoard& board, Color color, long timeLimitMs) {
    nodesSearched = 0;
    stopSearch = false;
    auto startTime = std::chrono::steady_clock::now();

    if (timeLimitMs > 0) {
        searchEndTime = startTime + std::chrono::milliseconds(timeLimitMs);
    } else {
        searchEndTime = std::chrono::steady_clock::time_point::max();
    }

    Move bestMove;
    bool hasBestMove = false;
    int bestScore = 0;
    int completedDepth = 0;

    // Iterative Deepening
    for (int d = 1; d <= maxDepth; d++) {
        Move currentBest;
        bool hasCurrentBest = false;
        int maxScore = -999999;
        int alpha = -999999;
        int beta = 999999;

        try {
            std::vector<Move> rootMoves = board.getAllLegalMoves(color);

            if (rootMoves.empty()) {
                return Move(); // No moves (checkmate or stalemate)
            }

            // If this is depth 1 and we don't have a best move yet, grab the first legal move as safety net
            if (!hasBestMove && d == 1) {
                bestMove = rootMoves[0];
                hasBestMove = true;
            }

            orderMoves(rootMoves, board);

            for (Move& move : rootMoves) {
                board.makeMove(move);

                Color nextColor = (color == Color::WHITE) ? Color::BLACK : Color::WHITE;
                int score = -negamax(board, d - 1, -beta, -alpha, nextColor);

                board.undoMove(move);

                if (stopSearch) {
                    // Use partial result from this depth if it's better
                    if (hasCurrentBest && completedDepth == 0) {
                        bestMove = currentBest;
                        bestScore = maxScore;
                        hasBestMove = true;
                        completedDepth = d;
                    }
                    break;
                }

                if (score > maxScore) {
                    maxScore = score;
                    currentBest = move;
                    hasCurrentBest = true;
                }
                if (maxScore > alpha) {
                    alpha = maxScore;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "info string ERROR in search at depth " << d << ": " << e.what() << std::endl;
            break;
        } catch (...) {
            std::cerr << "info string UNKNOWN ERROR in search at depth " << d << std::endl;
            break;
        }

        if (stopSearch) break;

        if (hasCurrentBest) {
            bestMove = currentBest;
            bestScore = maxScore;
            hasBestMove = true;
            completedDepth = d;
            auto now = std::chrono::steady_clock::now();
            long timeMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
            std::cout << "info depth " << d << " score cp " << bestScore
                      << " nodes " << nodesSearched << " time " << timeMs
                      << " pv " << bestMove.toUCI() << std::endl;
        }

        if (std::chrono::steady_clock::now() >= searchEndTime) {
            break;
        }
    }

    // Fallback: only print if search was interrupted before completing any depth
    if (hasBestMove && completedDepth == 0) {
        auto now = std::chrono::steady_clock::now();
        long timeMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
        std::cout << "info depth 1 score cp " << bestScore
                  << " nodes " << nodesSearched << " time " << timeMs
                  << " pv " << bestMove.toUCI() << std::endl;
    }

    return bestMove;
}

int SimpleEngine::negamax(ChessBoard& board, int depth, int alpha, int beta, Color color) {
    nodesSearched++;

    // Periodic time check
    if ((nodesSearched & 2047) == 0) {
        if (std::chrono::steady_clock::now() >= searchEndTime) {
            stopSearch = true;
        }
    }
    if (stopSearch) return 0;

    if (depth == 0) {
        return quiescence(board, alpha, beta, color);
    }

    std::vector<Move> moves = board.getAllLegalMoves(color);

    if (moves.empty()) {
        if (board.isInCheck(color)) {
            return -999999 + (maxDepth - depth);
        }
        return 0; // Stalemate
    }

    orderMoves(moves, board);

    int maxScore = -999999;

    for (Move& move : moves) {
        board.makeMove(move);

        Color nextColor = (color == Color::WHITE) ? Color::BLACK : Color::WHITE;
        int score = -negamax(board, depth - 1, -beta, -alpha, nextColor);

        board.undoMove(move);

        if (stopSearch) return 0;

        if (score > maxScore) {
            maxScore = score;
        }
        if (maxScore > alpha) {
            alpha = maxScore;
        }
        if (alpha >= beta) {
            break; // Beta Cutoff
        }
    }

    return maxScore;
}

int SimpleEngine::quiescence(ChessBoard& board, int alpha, int beta, Color color) {
    nodesSearched++;

    // Periodic time check
    if ((nodesSearched & 2047) == 0) {
        if (std::chrono::steady_clock::now() >= searchEndTime) {
            stopSearch = true;
        }
    }
    if (stopSearch) return 0;

    // 1. Stand Pat - evaluate the current position
    int standPat = evaluate(board, color);

    // If standing pat already beats beta, opponent won't allow this position
    if (standPat >= beta) {
        return beta;
    }

    // Delta Pruning: if even capturing a queen can't raise alpha, skip
    const int DELTA_MARGIN = QUEEN_VAL + 200; // Queen + safety margin for promotions
    if (standPat + DELTA_MARGIN < alpha) {
        return alpha;
    }

    // Raise alpha to stand pat score if it's better
    if (standPat > alpha) {
        alpha = standPat;
    }

    // 2. Generate ONLY capture and promotion moves
    std::vector<Move> captureMoves;
    board.generateCaptureMoves(color, captureMoves);

    // 3. Move ordering (MVV-LVA is critical here)
    orderMoves(captureMoves, board);

    // 4. Search each capture
    for (Move& move : captureMoves) {
        board.makeMove(move);

        Color nextColor = (color == Color::WHITE) ? Color::BLACK : Color::WHITE;
        int score = -quiescence(board, -beta, -alpha, nextColor);

        board.undoMove(move);

        if (stopSearch) return 0;

        if (score >= beta) {
            return beta; // Beta cutoff - fail high
        }
        if (score > alpha) {
            alpha = score; // New best move
        }
    }

    return alpha;
}

int SimpleEngine::evaluate(ChessBoard& board, Color activeColor) {
    int wScore = 0;
    int bScore = 0;
    bool endgame = isEndgame(board);

    for (int sq = 21; sq <= 98; sq++) {
        if (isOffBoard(sq)) continue;

        Piece* p = board.getPiece(Position::get(sq));
        if (p != nullptr) {
            int val = getBasicValue(p->getType());
            int psq;

            // Use endgame king table when appropriate
            if (p->getType() == PieceType::KING && endgame) {
                int row = (sq - 21) / 10;
                int col = (sq - 21) % 10;
                int idx = row * 8 + col;
                if (idx < 0 || idx > 63) { psq = 0; }
                else {
                    if (p->getColor() == Color::BLACK) idx = 63 - idx;
                    psq = KING_EG_TABLE[idx];
                }
            } else {
                psq = getPsqBonus(p, sq);
            }

            if (p->getColor() == Color::WHITE) {
                wScore += val + psq;
            } else {
                bScore += val + psq;
            }
        }
    }

    return (activeColor == Color::WHITE) ? (wScore - bScore) : (bScore - wScore);
}

bool SimpleEngine::isOffBoard(int sq) const {
    return (sq % 10) == 0 || (sq % 10) == 9;
}

int SimpleEngine::getBasicValue(PieceType t) const {
    switch (t) {
        case PieceType::PAWN:   return PAWN_VAL;
        case PieceType::KNIGHT: return KNIGHT_VAL;
        case PieceType::BISHOP: return BISHOP_VAL;
        case PieceType::ROOK:   return ROOK_VAL;
        case PieceType::QUEEN:  return QUEEN_VAL;
        case PieceType::KING:   return KING_VAL;
        default: return 0;
    }
}

int SimpleEngine::getPsqBonus(Piece* p, int sq) const {
    int row = (sq - 21) / 10;
    int col = (sq - 21) % 10;
    int idx = row * 8 + col;

    if (idx < 0 || idx > 63) return 0;

    if (p->getColor() == Color::BLACK) {
        idx = 63 - idx;
    }

    switch (p->getType()) {
        case PieceType::PAWN:   return PAWN_TABLE[idx];
        case PieceType::KNIGHT: return KNIGHT_TABLE[idx];
        case PieceType::BISHOP: return BISHOP_TABLE[idx];
        case PieceType::ROOK:   return ROOK_TABLE[idx];
        case PieceType::QUEEN:  return QUEEN_TABLE[idx];
        case PieceType::KING:   return KING_MG_TABLE[idx]; // default to middlegame
        default: return 0;
    }
}

bool SimpleEngine::isEndgame(ChessBoard& board) const {
    int wQueens = 0, bQueens = 0;
    int wMinor = 0, bMinor = 0;
    for (int sq = 21; sq <= 98; sq++) {
        if (isOffBoard(sq)) continue;
        Piece* p = board.getPiece(Position::get(sq));
        if (p == nullptr) continue;
        if (p->getType() == PieceType::QUEEN) {
            if (p->getColor() == Color::WHITE) wQueens++; else bQueens++;
        } else if (p->getType() == PieceType::KNIGHT || p->getType() == PieceType::BISHOP) {
            if (p->getColor() == Color::WHITE) wMinor++; else bMinor++;
        } else if (p->getType() == PieceType::ROOK) {
            if (p->getColor() == Color::WHITE) wMinor += 2; else bMinor += 2;
        }
    }
    // Endgame if both sides have no queen, or have queen but no other pieces
    bool wEndgame = (wQueens == 0) || (wQueens == 1 && wMinor <= 1);
    bool bEndgame = (bQueens == 0) || (bQueens == 1 && bMinor <= 1);
    return wEndgame && bEndgame;
}

void SimpleEngine::orderMoves(std::vector<Move>& moves, ChessBoard& board) {
    std::sort(moves.begin(), moves.end(),
        [this, &board](const Move& m1, const Move& m2) {
            return scoreMove(m1, board) > scoreMove(m2, board);
        }
    );
}

int SimpleEngine::scoreMove(const Move& m, ChessBoard& board) {
    int score = 0;

    // Check the board directly for a capture target — m.capturedPiece
    // is only populated after makeMove(), so it's null during ordering.
    Piece* victim = board.getPiece(m.to);
    if (victim != nullptr) {
        // MVV-LVA: prioritize capturing high-value pieces with low-value pieces
        score = 10 * getBasicValue(victim->getType());
        Piece* attacker = board.getPiece(m.from);
        if (attacker) {
            score -= getBasicValue(attacker->getType());
        }
    } else if (m.isEnPassant) {
        // En passant is a pawn capture — score it as PxP
        score = 10 * PAWN_VAL - PAWN_VAL;
    }

    if (m.promotionPiece != PieceType::NONE) {
        score += 900;
    }
    return score;
}

int64_t SimpleEngine::getNodesSearched() const {
    return nodesSearched;
}
