#include "SimpleEngine.h"
#include "EvalParams.h"
#include "Book.h"
#include "nnue.h"
#include <algorithm>
#include <iostream>
#include <cmath>

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

static int lmrTable[128][64];

static bool initLmrDone = []() {
    for (int d = 1; d < 128; d++) {
        for (int m = 1; m < 64; m++) {
            lmrTable[d][m] = static_cast<int>(0.75 + std::log(d) * std::log(m) / 2.25);
        }
    }
    return true;
}();

SimpleEngine::SimpleEngine() : maxDepth(4), stopSearch(false), nodesSearched(0), tt(16) {
    for (int i = 0; i < MAX_PLY + 32; i++) {
        nnueStack[i].data.accumulator.computedAccumulation = 0;
        nnueStack[i].data.dirtyPiece.dirtyNum = 0;
    }
}

SimpleEngine::SimpleEngine(int depth) : maxDepth(depth), stopSearch(false), nodesSearched(0), tt(16) {
    for (int i = 0; i < MAX_PLY + 32; i++) {
        nnueStack[i].data.accumulator.computedAccumulation = 0;
        nnueStack[i].data.dirtyPiece.dirtyNum = 0;
    }
}

void SimpleEngine::setDepth(int depth) { maxDepth = depth; }

void SimpleEngine::clearHistoryAndKillers() {
    for (int i = 0; i < MAX_PLY; i++) {
        killerMoves[i][0] = 0;
        killerMoves[i][1] = 0;
    }
    for (int c = 0; c < 2; c++) {
        for (int f = 0; f < 64; f++) {
            for (int t = 0; t < 64; t++) {
                history[c][f][t] = 0;
            }
        }
    }
}

Move16 SimpleEngine::getBestMove(BoardState& board, int color, long timeLimitMs) {
    nodesSearched = 0;
    stopSearch = false;
    tt.newSearch();
    clearHistoryAndKillers();
    auto startTime = std::chrono::steady_clock::now();

    // Reset all plies of nnueStack to ensure zero leakage from previous searches
    for (int i = 0; i < MAX_PLY + 32; i++) {
        nnueStack[i].data.accumulator.computedAccumulation = 0;
        nnueStack[i].data.dirtyPiece.dirtyNum = 0;
    }
    evaluate(board, color, 0);

    // --- Tekrar tespiti icin anahtar yigini ---
    // gameKeys'in son elemani kok pozisyon olmali; degilse (or. TerminalGame)
    // sadece koku koyariz, oyun gecmisi olmadan calisir.
    {
        int n = static_cast<int>(gameKeys.size());
        if (n == 0 || gameKeys.back() != board.zobristKey) {
            keyStack[0] = board.zobristKey;
            rootIndex = 0;
        } else {
            int start = (n > MAX_GAME_KEYS) ? n - MAX_GAME_KEYS : 0;
            for (int i = start; i < n; i++) keyStack[i - start] = gameKeys[i];
            rootIndex = n - 1 - start;
        }
    }

    if (timeLimitMs > 0) {
        long softLimitMs = static_cast<long>(timeLimitMs * 0.60);
        if (softLimitMs < 5) softLimitMs = std::max(1L, timeLimitMs - 5);
        searchSoftEndTime = startTime + std::chrono::milliseconds(softLimitMs);
        searchHardEndTime = startTime + std::chrono::milliseconds(timeLimitMs);
    } else {
        searchSoftEndTime = std::chrono::steady_clock::time_point::max();
        searchHardEndTime = std::chrono::steady_clock::time_point::max();
    }

    // 0. Açılış Kitabı (Book move varsa 0 ms'de anında oyna)
    Move16 bookMove = openingBook.getBookMove(board);
    if (bookMove != MOVE_NONE) {
        std::cout << "info depth 1 score cp 0 pv " << moveToUCI(bookMove) << std::endl;
        return bookMove;
    }

    Move16 bestMove = MOVE_NONE;
    bool hasBestMove = false;
    int bestScore = 0;
    int completedDepth = 0;

    for (int d = 1; d <= maxDepth; d++) {
        Move16 currentBest = MOVE_NONE;
        bool hasCurrentBest = false;
        int maxScore = -999999;
        int alpha = -999999;
        int beta = 999999;

        try {
            MoveList rootMoves;
            MoveGen::generateAllMoves(board, rootMoves);

            if (rootMoves.count == 0) return MOVE_NONE;

            // Sort moves: Bir önceki derinlikte bulunan en iyi hamleyi (bestMove)
            // TT move olarak en başa yerleştir.
            orderMoves(rootMoves, board, bestMove, 0, color);

            // Aspiration Windows:
            // Derinlik 4 ve üzerinde, bir önceki iterasyonun skoru etrafında
            // dar bir pencere (örn. [-50, +50]) açıyoruz.
            // Skor pencerenin dışına çıkarsa (fail-low veya fail-high), pencereyi
            // genişleterek tekrar arıyoruz (widening).
            int delta = 40;
            if (d >= 4 && hasBestMove && std::abs(bestScore) < 900000) {
                alpha = std::max(-999999, bestScore - delta);
                beta  = std::min( 999999, bestScore + delta);
            }

            while (true) {
                int searchAlpha = alpha;
                int searchBeta = beta;
                maxScore = -999999;
                currentBest = MOVE_NONE;
                hasCurrentBest = false;
                int legalMovesSeen = 0;

                for (Move16 move : rootMoves) {
                    int pt = board.pieceOnSquare(moveFrom(move));
                    UndoState undo;
                    if (!board.makeMove(move, undo)) continue;
                    legalMovesSeen++;

                    recordMoveNNUE(1, move, pt, color, undo);

                    if (!hasBestMove && d == 1 && legalMovesSeen == 1) {
                        bestMove = move;
                        hasBestMove = true;
                    }

                    int nextColor = color ^ 1;
                    int score = -negamax(board, d - 1, 1, -searchBeta, -searchAlpha, nextColor);

                    board.undoMove(move, undo);

                    if (stopSearch) break;

                    if (score > maxScore) {
                        maxScore = score;
                        currentBest = move;
                        hasCurrentBest = true;
                    }
                    if (maxScore > searchAlpha) {
                        searchAlpha = maxScore;
                    }
                    if (searchAlpha >= searchBeta) {
                        break; // Beta cutoff at root
                    }
                }

                if (legalMovesSeen == 0) return MOVE_NONE;
                if (stopSearch) break;

                // Aspiration Window kontrolü:
                // Skor pencere içinde kaldıysa (alpha < maxScore < beta) arama başarılı!
                if (d < 4 || !hasBestMove || std::abs(bestScore) >= 900000 ||
                    (maxScore > alpha && maxScore < beta)) {
                    break;
                }

                // Fail-low: Beklenenden daha kötü skor -> alpha'yı aşağı genişlet
                if (maxScore <= alpha) {
                    alpha = std::max(-999999, alpha - delta);
                    delta += delta / 2;
                }
                // Fail-high: Beklenenden daha iyi skor -> beta'yı yukarı genişlet
                else if (maxScore >= beta) {
                    beta = std::min(999999, beta + delta);
                    delta += delta / 2;
                }

                // Pencere çok genişlediyse sonsuz pencereye dön
                if (delta > 1000) {
                    alpha = -999999;
                    beta  =  999999;
                }
            }

        } catch (...) {
            std::cerr << "info string UNKNOWN ERROR in search at depth " << d << std::endl;
            break;
        }

        if (stopSearch) {
            if (hasCurrentBest && completedDepth == 0) {
                bestMove = currentBest;
                bestScore = maxScore;
                hasBestMove = true;
                completedDepth = d;
            }
            break;
        }

        if (hasCurrentBest) {
            bestMove = currentBest;
            bestScore = maxScore;
            hasBestMove = true;
            completedDepth = d;
            long timeMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();
            std::cout << "info depth " << d << " score cp " << bestScore
                      << " nodes " << nodesSearched << " time " << timeMs
                      << " pv " << moveToUCI(bestMove) << std::endl;
        }

        // Yumuşak Zaman Sınırı (Soft Time Limit):
        // Sürenin %60'ından fazlası harcandıysa, bir sonraki derinlik kesinlikle yarım kalacaktır.
        // Bu yüzden yeni derinliğe girmeden mevcut en iyi tamamlanmış hamleyle çık!
        if (std::chrono::steady_clock::now() >= searchSoftEndTime) break;
    }

    if (hasBestMove && completedDepth == 0) {
        long timeMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();
        std::cout << "info depth 1 score cp " << bestScore
                  << " nodes " << nodesSearched << " time " << timeMs
                  << " pv " << moveToUCI(bestMove) << std::endl;
    }

    return bestMove;
}

// ============================================================
//  Tekrar tespiti
//  - Dugum anahtarini keyStack'e yazar, sonra geriye tarar.
//  - Sadece ayni taraf hamle sirasindaki pozisyonlar (2'ser adim) ve
//    sadece son geri donulemez hamleden (halfMoveClock) sonrasi taranir.
//    Pratikte birkac karsilastirma -> maliyeti ihmal edilebilir.
//  - Arama yolunda (kok dahil) 1 tekrar = beraberlik (2-fold).
//    Kokten onceki oyun gecmisinde ise gercek 3-fold aranir.
// ============================================================
bool SimpleEngine::isRepetition(const BoardState& board, int ply) {
    int cur = rootIndex + ply;
    if (ply >= MAX_PLY + 8) return false; // yigin tasmasi korumasi
    const uint64_t key = board.zobristKey;
    keyStack[cur] = key;

    int limit = cur - board.halfMoveClock;
    if (limit < 0) limit = 0;

    // cur - 2 den baslayarak geriye adimla:
    // Eger arama yolundaysa (i >= rootIndex), bu pozisyon kokten sonra zaten gecilmis demektir -> tekrar (2-fold in search tree = draw).
    // Eger oyun gecmisindeyse (i < rootIndex), arama yolu bu pozisyonu olusturursa oyundaki tekrar sayisi 1 artar.
    // 3-fold kurali geregi ve standart satranc motorlarinda: eger pozisyon oyun gecmisinde daha once 1 kez bile gorulduyse,
    // simdi bu hamleyle tekrar girmek beraberlige sebep olur veya rakip 3. tekrari zorlayabilir. Bu yuzden i < rootIndex iken gorulmesi de draw (0) dondurur!
    for (int i = cur - 2; i >= limit; i -= 2) {
        if (keyStack[i] == key) {
            return true;
        }
    }
    return false;
}

int SimpleEngine::negamax(BoardState& board, int depth, int ply, int alpha, int beta, int color, bool allowNull, Move16 excludedMove) {
    nodesSearched++;

    if ((nodesSearched & 2047) == 0) {
        if (std::chrono::steady_clock::now() >= searchHardEndTime) stopSearch = true;
    }
    if (stopSearch) return 0;

    // BERABERLİK KONTROLLERİ (Tekrar veya 50 Hamle Kuralı)
    // 50 hamle kuralı (100 ply): son piyon sürüşü veya taş alımından sonra 50 tam hamle geçince draw (0).
    // Kök düğüm hariç (ply > 0) kontrol edilir.
    if (ply > 0 && board.halfMoveClock >= 100) return 0;
    if (isRepetition(board, ply)) return 0;

    int alphaOriginal = alpha;
    int ttScore = 0;
    uint16_t ttMove = 0;
    int ttDepth = 0;
    int ttBound = 0;

    if (excludedMove == 0 && tt.probe(board.zobristKey, depth, alpha, beta, ply, ttScore, ttMove, &ttDepth, &ttBound)) {
        return ttScore;
    }

    if (depth <= 0) return quiescence(board, alpha, beta, ply, color);

    int enemyColor = color ^ 1;
    bool inCheck = board.isSquareAttacked(board.getKingSquare(color), enemyColor);

    int staticEval = 0;
    if (!inCheck) {
        staticEval = evaluate(board, color, ply);
    }

    // REVERSE FUTILITY PRUNING (RFP / Static Null Move Pruning)
    if (!inCheck && depth <= 6 && std::abs(beta) < 900000) {
        int rfpMargin = 75 * depth;
        if (staticEval - rfpMargin >= beta) {
            return beta;
        }
    }

    // NULL MOVE PRUNING (NMP)
    // Sadece staticEval >= beta iken (pozisyon zaten beta'yı geçebilecek güçteyken) boş hamle dene!
    // Dinamik R: derinlik arttıkça daha agresif indirgeme (3 + depth/6).
    if (allowNull && !inCheck && depth >= 3 && staticEval >= beta && board.hasNonPawnMaterial(color)) {
        
        UndoState nullUndo;
        board.makeNullMove(nullUndo); 
        if (ply + 1 < MAX_PLY + 16) {
            nnueStack[ply + 1].data.dirtyPiece.dirtyNum = 0;
            nnueStack[ply + 1].data.dirtyPiece.pc[0] = 0;
            nnueStack[ply + 1].data.accumulator.computedAccumulation = 0;
        }
        
        int R = 3 + depth / 6; // Dynamic reduction
        int nullScore = -negamax(board, depth - 1 - R, ply + 1, -beta, -beta + 1, enemyColor, false);
        
        board.undoNullMove(nullUndo); 
        
        if (nullScore >= beta) {
            return beta;
        }
    }

    // SINGULAR EXTENSIONS (SE)
    bool isSingular = false;
    if (excludedMove == 0 
        && depth >= 6 
        && ttMove != 0 
        && ttDepth >= depth - 3 
        && ttBound != TT_UPPER 
        && std::abs(ttScore) < 900000 
        && ply > 0) 
    {
        int singularMargin = 3 * depth;
        int singularBeta = ttScore - singularMargin;
        int singularDepth = (depth - 1) / 2;

        int singularScore = negamax(board, singularDepth, ply, singularBeta - 1, singularBeta, color, false, ttMove);
        if (stopSearch) return 0;

        if (singularScore < singularBeta) {
            isSingular = true;
        }
    }

    // Move Picker Initialization
    MovePicker picker(board, ttMove, killerMoves[ply][0], killerMoves[ply][1], history);

    int maxScore = -999999;
    int legalCount = 0;
    uint16_t bestMove = 0;
    bool foundPv = false;
    Move16 move;

    while ((move = picker.nextMove()) != 0) {
        if (move == excludedMove) continue;

        bool isAdvPawn = false;
        int pt = board.pieceOnSquare(moveFrom(move));
        if (pt == PAWN_IDX) {
            int toRank = moveTo(move) / 8;
            if (color == WHITE_SIDE && toRank >= 5) isAdvPawn = true;
            if (color == BLACK_SIDE && toRank <= 2) isAdvPawn = true;
        }

        UndoState undo;
        if (!board.makeMove(move, undo)) continue;
        legalCount++;

        if (ply + 1 < MAX_PLY + 16) {
            recordMoveNNUE(ply + 1, move, pt, color, undo);
        }

        bool isQuietMove = !isCapture(move) && !isPromotion(move);
        bool givesCheck = board.isInCheck();

        // FUTILITY PRUNING (FP)
        // Derinlik 1-2 iken, şah çekilmiyorsa ve hamle taktik değilse:
        // Eğer staticEval + margin <= alpha ise bu sessiz hamleyi buda!
        if (!inCheck && depth <= 2 && legalCount > 1 && !givesCheck && std::abs(alpha) < 900000) {
            if (isQuietMove) {
                int fpMargin = 120 * depth;
                if (staticEval + fpMargin <= alpha) {
                    board.undoMove(move, undo);
                    continue;
                }
            }
        }

        int extension = 0;
        if (givesCheck && ply < MAX_PLY) {
            extension = 1;
        } else if (move == ttMove && isSingular && ply < MAX_PLY) {
            extension = 1;
        }
        
        int newDepth = depth - 1 + extension;

        int score;
        if (legalCount > 1) {
            bool isLmrStage = (picker.stage == STAGE_QUIETS || picker.stage == STAGE_BAD_CAPTURES);
            
            // Extra LMR Security Conditions
            bool canLMR = true;
            if (isAdvPawn) canLMR = false;                    // Advanced pawn protection
            if (!isLmrStage) canLMR = false;                  // Stage is STAGE_TT, STAGE_GOOD_CAPTURES, or STAGE_KILLERS
            if (inCheck) canLMR = false;                      // Not in check
            if (givesCheck) canLMR = false;                   // Move does not give check
            if (isPromotion(move)) canLMR = false;            // Not a promotion move
            
            // LMR Execution
            if (canLMR && legalCount > 3 && newDepth >= 2) {
                int dClamped = std::min(newDepth, 127);
                int mClamped = std::min(legalCount, 63);
                int R = lmrTable[dClamped][mClamped];

                // History heuristic ile dinamik ayarlama
                int hist = history[color][moveFrom(move)][moveTo(move)];
                if (hist < -1000) R += 1;
                else if (hist > 2000) R -= 1;

                // Safety bound: prevent R from resetting the search completely
                if (R < 1) R = 1;
                if (R >= newDepth) R = newDepth - 1; 
                
                // 1. PVS Search with Zero Window & Reduction
                score = -negamax(board, newDepth - R, ply + 1, -alpha - 1, -alpha, enemyColor, true);
                
                if (score > alpha) {
                    // 2. PVS Re-search (Full depth Zero Window)
                    score = -negamax(board, newDepth, ply + 1, -alpha - 1, -alpha, enemyColor, true);
                }
            } else {
                // Normal Null Window Search
                score = -negamax(board, newDepth, ply + 1, -alpha - 1, -alpha, enemyColor, true);
            }
            
            // Research with full window if it beats alpha but not beta
            if (score > alpha && score < beta) {
                score = -negamax(board, newDepth, ply + 1, -beta, -alpha, enemyColor, true);
            }
        } else {
            // First move: Full Window Search
            score = -negamax(board, newDepth, ply + 1, -beta, -alpha, enemyColor, true);
        }

        board.undoMove(move, undo);

        if (stopSearch) return 0;

        if (score > maxScore) {
            maxScore = score;
            bestMove = move;
        }
        if (maxScore > alpha) {
            alpha = maxScore;
            foundPv = true; // Raise alpha, next moves are searched with null window
        }
        if (alpha >= beta) {
            // Beta Cutoff
            if (isQuietMove) {
                if (ply < MAX_PLY) {
                    if (move != killerMoves[ply][0]) {
                        killerMoves[ply][1] = killerMoves[ply][0];
                        killerMoves[ply][0] = move;
                    }
                }
                int bonus = depth * depth;
                int MAX_HISTORY = 16384;
                int& hist = history[color][moveFrom(move)][moveTo(move)];
                hist += bonus - (hist * std::abs(bonus) / MAX_HISTORY);
            }
            break; 
        }
    }

    if (legalCount == 0) {
        if (excludedMove != 0) return -999999;
        if (board.isInCheck()) return -999999 + ply; // Mate
        return 0; // Stalemate
    }

    if (excludedMove == 0) {
        int bound = TT_EXACT;
        if (maxScore <= alphaOriginal) {
            bound = TT_UPPER;
        } else if (maxScore >= beta) {
            bound = TT_LOWER;
        }
        tt.store(board.zobristKey, depth, ply, maxScore, bound, bestMove);
    }

    return maxScore;
}

int SimpleEngine::quiescence(BoardState& board, int alpha, int beta, int ply, int color) {
    nodesSearched++;

    if ((nodesSearched & 2047) == 0) {
        if (std::chrono::steady_clock::now() >= searchHardEndTime) stopSearch = true;
    }
    if (stopSearch) return 0;

    int standPat = evaluate(board, color, ply);
    if (standPat >= beta) return beta;

    const int DELTA_MARGIN = QUEEN_VAL + 200;
    if (standPat + DELTA_MARGIN < alpha) return alpha;

    if (standPat > alpha) alpha = standPat;

    MoveList moves;
    MoveGen::generateTacticalMoves(board, moves);
    orderMoves(moves, board, 0, ply, color);

    for (Move16 move : moves) {
        int movingPiece = board.pieceOnSquare(moveFrom(move));
        UndoState undo;
        if (!board.makeMove(move, undo)) continue;

        if (ply + 1 < MAX_PLY + 16) {
            recordMoveNNUE(ply + 1, move, movingPiece, color, undo);
        }

        int score = -quiescence(board, -beta, -alpha, ply + 1, color ^ 1);
        board.undoMove(move, undo);

        if (stopSearch) return 0;

        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }

    return alpha;
}

void SimpleEngine::recordMoveNNUE(int nextPly, Move16 m, int movingPiece, int side, const UndoState& undo) {
    if (nextPly < 0 || nextPly >= MAX_PLY + 16) return;

    static const int PIECE_MAP[2][6] = {
        // WHITE_SIDE: PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING
        { wpawn, wknight, wbishop, wrook, wqueen, wking },
        // BLACK_SIDE: PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING
        { bpawn, bknight, bbishop, brook, bqueen, bking }
    };

    DirtyPiece& dp = nnueStack[nextPly].data.dirtyPiece;
    nnueStack[nextPly].data.accumulator.computedAccumulation = 0;

    int from = moveFrom(m);
    int to   = moveTo(m);
    int opp  = side ^ 1;

    if (isCastle(m)) {
        int rookFrom, rookTo;
        if (moveFlags(m) == FLAG_KING_CASTLE) {
            rookFrom = (side == WHITE_SIDE) ? H1 : H8;
            rookTo   = (side == WHITE_SIDE) ? F1 : F8;
        } else {
            rookFrom = (side == WHITE_SIDE) ? A1 : A8;
            rookTo   = (side == WHITE_SIDE) ? D1 : D8;
        }
        dp.dirtyNum = 2;
        dp.pc[0] = PIECE_MAP[side][KING_IDX];
        dp.from[0] = from;
        dp.to[0] = to;
        dp.pc[1] = PIECE_MAP[side][ROOK_IDX];
        dp.from[1] = rookFrom;
        dp.to[1] = rookTo;
    }
    else if (isPromotion(m)) {
        int promoType = promotionPiece(m); // 1=Knight, 2=Bishop, 3=Rook, 4=Queen
        if (promoType < 1 || promoType > 4) promoType = QUEEN_IDX;
        if (isCapture(m)) {
            dp.dirtyNum = 3;
            dp.pc[0] = PIECE_MAP[side][PAWN_IDX];
            dp.from[0] = from;
            dp.to[0] = 64;
            dp.pc[1] = PIECE_MAP[side][promoType];
            dp.from[1] = 64;
            dp.to[1] = to;
            int capPc = (undo.capturedPiece >= 0 && undo.capturedPiece < 6) ? undo.capturedPiece : PAWN_IDX;
            dp.pc[2] = PIECE_MAP[opp][capPc];
            dp.from[2] = to;
            dp.to[2] = 64;
        } else {
            dp.dirtyNum = 2;
            dp.pc[0] = PIECE_MAP[side][PAWN_IDX];
            dp.from[0] = from;
            dp.to[0] = 64;
            dp.pc[1] = PIECE_MAP[side][promoType];
            dp.from[1] = 64;
            dp.to[1] = to;
        }
    }
    else if (isEnPassant(m)) {
        int epCapSq = (side == WHITE_SIDE) ? to - 8 : to + 8;
        dp.dirtyNum = 2;
        dp.pc[0] = PIECE_MAP[side][PAWN_IDX];
        dp.from[0] = from;
        dp.to[0] = to;
        dp.pc[1] = PIECE_MAP[opp][PAWN_IDX];
        dp.from[1] = epCapSq;
        dp.to[1] = 64;
    }
    else if (isCapture(m)) {
        dp.dirtyNum = 2;
        if (movingPiece < 0 || movingPiece >= 6) movingPiece = PAWN_IDX;
        dp.pc[0] = PIECE_MAP[side][movingPiece];
        dp.from[0] = from;
        dp.to[0] = to;
        int capPc = (undo.capturedPiece >= 0 && undo.capturedPiece < 6) ? undo.capturedPiece : PAWN_IDX;
        dp.pc[1] = PIECE_MAP[opp][capPc];
        dp.from[1] = to;
        dp.to[1] = 64;
    }
    else {
        // Normal quiet move
        dp.dirtyNum = 1;
        if (movingPiece < 0 || movingPiece >= 6) movingPiece = PAWN_IDX;
        dp.pc[0] = PIECE_MAP[side][movingPiece];
        dp.from[0] = from;
        dp.to[0] = to;
    }
}

int SimpleEngine::evaluate(const BoardState& board, int activeColor, int ply) {
    if (is_nnue_loaded()) {
        static const int PIECE_MAP[2][6] = {
            // WHITE_SIDE: PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING
            { wpawn, wknight, wbishop, wrook, wqueen, wking },
            // BLACK_SIDE: PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING
            { bpawn, bknight, bbishop, brook, bqueen, bking }
        };

        int pieces[33];
        int squares[33];

        pieces[0] = wking;
        squares[0] = lsb(board.pieces[WHITE_SIDE][KING_IDX]);

        pieces[1] = bking;
        squares[1] = lsb(board.pieces[BLACK_SIDE][KING_IDX]);

        int count = 2;
        for (int side = WHITE_SIDE; side <= BLACK_SIDE; side++) {
            for (int pt = PAWN_IDX; pt < KING_IDX; pt++) {
                uint64_t bb = board.pieces[side][pt];
                int nnuePiece = PIECE_MAP[side][pt];
                while (bb) {
                    squares[count] = popLsb(bb);
                    pieces[count] = nnuePiece;
                    count++;
                }
            }
        }
        pieces[count] = 0;
        squares[count] = 0;

        int player = (activeColor == WHITE_SIDE) ? white : black;

        if (ply >= 0 && ply < MAX_PLY + 16) {
            NNUEdata* nnuePtrs[3];
            nnuePtrs[0] = &nnueStack[ply].data;
            nnuePtrs[1] = (ply >= 1) ? &nnueStack[ply - 1].data : nullptr;
            nnuePtrs[2] = nullptr; // Strict parent-only update to guarantee zero skipped-ply desync
            return nnue_evaluate_incremental(player, pieces, squares, nnuePtrs);
        }

        return nnue_evaluate(player, pieces, squares);
    }

    int scores[2] = {0, 0};
    bool endgame = isEndgame(board);

    for (int side = WHITE_SIDE; side <= BLACK_SIDE; side++) {
        for (int pt = PAWN_IDX; pt <= KING_IDX; pt++) {
            uint64_t bb = board.pieces[side][pt];
            int pieceVal = getBasicValue(pt);
            
            while (bb) {
                int sq = popLsb(bb);
                int psq = (pt == KING_IDX && endgame) ? 
                          evalParams.kingEgTable[side == WHITE_SIDE ? sq : (sq ^ 56)] : 
                          getPsqBonus(pt, sq, side);
                
                scores[side] += pieceVal + psq;
            }
        }
    }

    // Pozisyonel Bonuslar:
    // 1. Çift Fil Bonusu (Bishop Pair)
    if (popCount(board.pieces[WHITE_SIDE][BISHOP_IDX]) >= 2) scores[WHITE_SIDE] += evalParams.bishopPair;
    if (popCount(board.pieces[BLACK_SIDE][BISHOP_IDX]) >= 2) scores[BLACK_SIDE] += evalParams.bishopPair;

    // 2. Açık ve Yarı Açık Hatta Kale Bonusu (Rooks on open/semi-open files)
    static const uint64_t FILE_MASKS[8] = {
        0x0101010101010101ULL, 0x0202020202020202ULL, 0x0404040404040404ULL, 0x0808080808080808ULL,
        0x1010101010101010ULL, 0x2020202020202020ULL, 0x4040404040404040ULL, 0x8080808080808080ULL
    };
    uint64_t wRooks = board.pieces[WHITE_SIDE][ROOK_IDX];
    while (wRooks) {
        int sq = popLsb(wRooks);
        int file = sq & 7;
        uint64_t fMask = FILE_MASKS[file];
        bool hasFriendlyPawn = (board.pieces[WHITE_SIDE][PAWN_IDX] & fMask) != 0;
        bool hasEnemyPawn    = (board.pieces[BLACK_SIDE][PAWN_IDX] & fMask) != 0;
        if (!hasFriendlyPawn) {
            scores[WHITE_SIDE] += (!hasEnemyPawn) ? evalParams.rookOpenFile : evalParams.rookSemiOpenFile;
        }
    }
    uint64_t bRooks = board.pieces[BLACK_SIDE][ROOK_IDX];
    while (bRooks) {
        int sq = popLsb(bRooks);
        int file = sq & 7;
        uint64_t fMask = FILE_MASKS[file];
        bool hasFriendlyPawn = (board.pieces[BLACK_SIDE][PAWN_IDX] & fMask) != 0;
        bool hasEnemyPawn    = (board.pieces[WHITE_SIDE][PAWN_IDX] & fMask) != 0;
        if (!hasFriendlyPawn) {
            scores[BLACK_SIDE] += (!hasEnemyPawn) ? evalParams.rookOpenFile : evalParams.rookSemiOpenFile;
        }
    }

    // 3. Basit Rok Bonusu (Simple Castling Bonus in Middlegame)
    if (!endgame) {
        uint64_t wKingBB = board.pieces[WHITE_SIDE][KING_IDX];
        if (wKingBB) {
            int kSq = lsb(wKingBB);
            if (kSq == G1 || kSq == H1 || kSq == C1 || kSq == B1) {
                scores[WHITE_SIDE] += evalParams.castleBonus;
            }
        }
        uint64_t bKingBB = board.pieces[BLACK_SIDE][KING_IDX];
        if (bKingBB) {
            int kSq = lsb(bKingBB);
            if (kSq == G8 || kSq == H8 || kSq == C8 || kSq == B8) {
                scores[BLACK_SIDE] += evalParams.castleBonus;
            }
        }
    }

    // 4. İzole Piyonlar ve Kademeli Geçer Piyonlar (Pawn Structure: Isolated & Passed Pawns)
    uint64_t wPawns = board.pieces[WHITE_SIDE][PAWN_IDX];
    uint64_t bPawns = board.pieces[BLACK_SIDE][PAWN_IDX];

    // Beyaz Piyonlar
    uint64_t tempWPawns = wPawns;
    while (tempWPawns) {
        int sq = popLsb(tempWPawns);
        int file = sq & 7;
        int rank = sq >> 3;

        uint64_t adjacentFiles = (file > 0 ? FILE_MASKS[file - 1] : 0ULL) | 
                                 (file < 7 ? FILE_MASKS[file + 1] : 0ULL);
        // İzole piyon
        if ((wPawns & adjacentFiles) == 0ULL) {
            scores[WHITE_SIDE] -= evalParams.isolatedPawn;
        }

        // Geçer piyon
        uint64_t forwardRanks = ~((1ULL << ((rank + 1) * 8)) - 1);
        uint64_t spanFiles = FILE_MASKS[file] | adjacentFiles;
        if ((bPawns & (forwardRanks & spanFiles)) == 0ULL) {
            if (rank == 3)      scores[WHITE_SIDE] += evalParams.passedPawnRank4;
            else if (rank == 4) scores[WHITE_SIDE] += evalParams.passedPawnRank5;
            else if (rank == 5) scores[WHITE_SIDE] += evalParams.passedPawnRank6;
            else if (rank >= 6) scores[WHITE_SIDE] += evalParams.passedPawnRank7;
        }
    }

    // Siyah Piyonlar
    uint64_t tempBPawns = bPawns;
    while (tempBPawns) {
        int sq = popLsb(tempBPawns);
        int file = sq & 7;
        int rank = sq >> 3;
        int relRank = 7 - rank;

        uint64_t adjacentFiles = (file > 0 ? FILE_MASKS[file - 1] : 0ULL) | 
                                 (file < 7 ? FILE_MASKS[file + 1] : 0ULL);
        // İzole piyon
        if ((bPawns & adjacentFiles) == 0ULL) {
            scores[BLACK_SIDE] -= evalParams.isolatedPawn;
        }

        // Geçer piyon
        uint64_t forwardRanks = (1ULL << (rank * 8)) - 1;
        uint64_t spanFiles = FILE_MASKS[file] | adjacentFiles;
        if ((wPawns & (forwardRanks & spanFiles)) == 0ULL) {
            if (relRank == 3)      scores[BLACK_SIDE] += evalParams.passedPawnRank4;
            else if (relRank == 4) scores[BLACK_SIDE] += evalParams.passedPawnRank5;
            else if (relRank == 5) scores[BLACK_SIDE] += evalParams.passedPawnRank6;
            else if (relRank >= 6) scores[BLACK_SIDE] += evalParams.passedPawnRank7;
        }
    }

    // 5. 7. Yatayda Kale Bonusu (Rook on 7th Rank)
    uint64_t wRooks7th = board.pieces[WHITE_SIDE][ROOK_IDX] & 0x00FF000000000000ULL;
    if (wRooks7th) scores[WHITE_SIDE] += popCount(wRooks7th) * evalParams.rookOn7th;

    uint64_t bRooks2nd = board.pieces[BLACK_SIDE][ROOK_IDX] & 0x000000000000FF00ULL;
    if (bRooks2nd) scores[BLACK_SIDE] += popCount(bRooks2nd) * evalParams.rookOn7th;

    // 6. Taş Aktifliği / Hareketlilik (Mobility: Knights & Bishops)
    uint64_t wPawnAttacks = ((wPawns << 7) & ~FILE_MASKS[7]) | ((wPawns << 9) & ~FILE_MASKS[0]);
    uint64_t bPawnAttacks = ((bPawns >> 7) & ~FILE_MASKS[0]) | ((bPawns >> 9) & ~FILE_MASKS[7]);

    uint64_t wKnights = board.pieces[WHITE_SIDE][KNIGHT_IDX];
    while (wKnights) {
        int sq = popLsb(wKnights);
        uint64_t safeMoves = Attacks::getKnightAttacks(sq) & ~board.occupancy[WHITE_SIDE] & ~bPawnAttacks;
        scores[WHITE_SIDE] += popCount(safeMoves) * evalParams.knightMobility;
    }
    uint64_t wBishops = board.pieces[WHITE_SIDE][BISHOP_IDX];
    while (wBishops) {
        int sq = popLsb(wBishops);
        uint64_t safeMoves = Magics::getBishopAttacks(sq, board.allOccupancy) & ~board.occupancy[WHITE_SIDE] & ~bPawnAttacks;
        scores[WHITE_SIDE] += popCount(safeMoves) * evalParams.bishopMobility;
    }

    uint64_t bKnights = board.pieces[BLACK_SIDE][KNIGHT_IDX];
    while (bKnights) {
        int sq = popLsb(bKnights);
        uint64_t safeMoves = Attacks::getKnightAttacks(sq) & ~board.occupancy[BLACK_SIDE] & ~wPawnAttacks;
        scores[BLACK_SIDE] += popCount(safeMoves) * evalParams.knightMobility;
    }
    uint64_t bBishops = board.pieces[BLACK_SIDE][BISHOP_IDX];
    while (bBishops) {
        int sq = popLsb(bBishops);
        uint64_t safeMoves = Magics::getBishopAttacks(sq, board.allOccupancy) & ~board.occupancy[BLACK_SIDE] & ~wPawnAttacks;
        scores[BLACK_SIDE] += popCount(safeMoves) * evalParams.bishopMobility;
    }

    return (activeColor == WHITE_SIDE) ? (scores[WHITE_SIDE] - scores[BLACK_SIDE]) 
                                       : (scores[BLACK_SIDE] - scores[WHITE_SIDE]);
}

int SimpleEngine::getBasicValue(int pt) const {
    switch (pt) {
        case PAWN_IDX:   return evalParams.pawnVal;
        case KNIGHT_IDX: return evalParams.knightVal;
        case BISHOP_IDX: return evalParams.bishopVal;
        case ROOK_IDX:   return evalParams.rookVal;
        case QUEEN_IDX:  return evalParams.queenVal;
        case KING_IDX:   return evalParams.kingVal;
        default: return 0;
    }
}

int SimpleEngine::getPsqBonus(int pt, int sq, int side) const {
    int idx = (side == WHITE_SIDE) ? sq : (sq ^ 56);
    
    switch (pt) {
        case PAWN_IDX:   return evalParams.pawnTable[idx];
        case KNIGHT_IDX: return evalParams.knightTable[idx];
        case BISHOP_IDX: return evalParams.bishopTable[idx];
        case ROOK_IDX:   return evalParams.rookTable[idx];
        case QUEEN_IDX:  return evalParams.queenTable[idx];
        case KING_IDX:   return evalParams.kingMgTable[idx];
        default: return 0;
    }
}

bool SimpleEngine::isEndgame(const BoardState& board) const {
    int wQueens = popCount(board.pieces[WHITE_SIDE][QUEEN_IDX]);
    int bQueens = popCount(board.pieces[BLACK_SIDE][QUEEN_IDX]);
    
    int wMinors = popCount(board.pieces[WHITE_SIDE][KNIGHT_IDX] | board.pieces[WHITE_SIDE][BISHOP_IDX]);
    int bMinors = popCount(board.pieces[BLACK_SIDE][KNIGHT_IDX] | board.pieces[BLACK_SIDE][BISHOP_IDX]);
    
    wMinors += 2 * popCount(board.pieces[WHITE_SIDE][ROOK_IDX]);
    bMinors += 2 * popCount(board.pieces[BLACK_SIDE][ROOK_IDX]);

    bool wEndgame = (wQueens == 0) || (wQueens == 1 && wMinors <= 1);
    bool bEndgame = (bQueens == 0) || (bQueens == 1 && bMinors <= 1);
    
    return wEndgame && bEndgame;
}

void SimpleEngine::orderMoves(MoveList& moves, const BoardState& board, uint16_t ttMove, int ply, int color) {
    if (moves.count == 0) return;
    
    // Sort logic using plain insertion sort or std::sort 
    // We'll create a side array of scores
    int curScores[256];
    for (int i = 0; i < moves.count; i++) {
        curScores[i] = scoreMove(moves.moves[i], board, ttMove, ply, color);
    }

    for (int i = 1; i < moves.count; i++) {
        Move16 tmpMove = moves.moves[i];
        int tmpScore = curScores[i];
        int j = i - 1;
        while (j >= 0 && curScores[j] < tmpScore) {
            curScores[j + 1] = curScores[j];
            moves.moves[j + 1] = moves.moves[j];
            j--;
        }
        moves.moves[j + 1] = tmpMove;
        curScores[j + 1] = tmpScore;
    }
}

int SimpleEngine::scoreMove(Move16 m, const BoardState& board, uint16_t ttMove, int ply, int color) {
    if (ttMove != 0 && m == ttMove) {
        return 2000000;
    }

    int score = 0;
    int from = moveFrom(m);
    int to = moveTo(m);
    
    int victim = board.pieceOnSquare(to);
    bool isCapture = false;

    if (victim != -1) {
        isCapture = true;
        int attacker = board.pieceOnSquare(from);
        score = 1000000 + 10 * getBasicValue(victim) - (attacker != -1 ? getBasicValue(attacker) : 0);
    } else if (isEnPassant(m)) {
        isCapture = true;
        score = 1000000 + 10 * PAWN_VAL - PAWN_VAL;
    }

    if (isPromotion(m)) {
        score += 90000;
    }

    if (isCapture || isPromotion(m)) {
        return score;
    }

    // Quiet moves: Killer Moves
    if (ply < MAX_PLY) {
        if (m == killerMoves[ply][0]) return 900000;
        if (m == killerMoves[ply][1]) return 800000;
    }

    if (isCastle(m)) return 750000;

    // History Heuristic
    return history[color][from][to];
}

// ============================================================
//  MovePicker — tek üretim (generateAllMoves x1) + selection sort
//
//  Eski sürüm düğüm başına 4x generateAllMoves + 1x generateTacticalMoves
//  çağırıyordu (TT doğrulama, killer1, killer2, quiets). Bu sürüm hamleleri
//  bir kez üretir, kategorilere ayırır ve her aşamada kalanların en iyisini
//  seçer. Hamle SIRASI eski sürümle birebir aynıdır (stabil seçim), yani
//  sabit derinlikte arama ağacı / node sayısı değişmez; sadece hız artar.
// ============================================================
MovePicker::MovePicker(BoardState& b, uint16_t ttM, uint16_t k1, uint16_t k2, int hist[2][64][64])
    : board(b), ttMove(ttM), killer1(k1), killer2(k2), history(hist),
      stage(STAGE_TT), moveIndex(0), goodEnd(0), quietEnd(0), badEnd(0),
      killer1Ok(false), killer2Ok(false) {
}

// QS'deki generateTacticalMoves ile aynı küme: tüm taş almalar (EP dahil),
// sadece VEZİR terfileri (alan ve almayan). Alt-terfiler bu kümede değil.
static inline bool isPickerTactical(Move16 m) {
    int f = moveFlags(m);
    if (f == FLAG_QUEEN_PROMO || f == FLAG_QUEEN_PROMO_CAP) return true;
    return f == FLAG_CAPTURE || f == FLAG_EP_CAPTURE;
}

void MovePicker::generateAndPartition() {
    MoveList all;
    MoveGen::generateAllMoves(board, all);   // DÜĞÜM BAŞINA TEK ÜRETİM

    bool ttOk = false;
    Move16 bad[256];
    int badCount = 0;
    int n = 0;

    // --- Geçiş 1: taktik hamleler (good -> moves[], bad -> geçici) ---
    for (int i = 0; i < all.count; i++) {
        Move16 m = all.moves[i];
        if (m == ttMove) { ttOk = true; continue; }
        if (!isPickerTactical(m)) continue;

        int see = board.getSEE(m);
        if (see >= 0) {
            moves[n]  = m;
            scores[n] = isCapture(m) ? see : 90000; // queen promo push = 90000
            n++;
        } else {
            bad[badCount++] = m;
        }
    }
    goodEnd = n;

    // --- Geçiş 2: quiet hamleler + killer doğrulama ---
    // History puanı BURADA değil, STAGE_QUIETS'e girerken okunur: önceki
    // hamlelerin (TT/capture/killer) alt ağaçları history'yi güncelliyor.
    for (int i = 0; i < all.count; i++) {
        Move16 m = all.moves[i];
        if (isCapture(m) || isPromotion(m)) continue;
        if (m == ttMove) continue;
        if (m == killer1) { killer1Ok = true; continue; }
        if (m == killer2) { killer2Ok = true; continue; }
        moves[n++] = m;
    }
    quietEnd = n;

    // --- Bad captures (üretim sırasında, sıralanmaz — eski davranış) ---
    for (int i = 0; i < badCount; i++) moves[n++] = bad[i];
    badEnd = n;

    // NOT: Alt-terfiler (=N/=B/=R) eski sürümde olduğu gibi TT hamlesi
    // olmadıkça aranmıyor. Davranışı birebir korumak için bilerek bırakıldı.

    // Killer geçerlilik koşulları (eskisiyle aynı)
    if (killer1 == 0 || killer1 == ttMove) killer1Ok = false;
    if (killer2 == 0 || killer2 == ttMove || killer2 == killer1) killer2Ok = false;

    if (!ttOk) ttMove = 0; // geçersiz/çakışan TT hamlesi
}

// [begin,end) aralığındaki en yüksek puanlı hamleyi 'begin' konumuna taşır.
// İlk maksimumu seçip aradakileri kaydırdığı için STABİLDİR: eşit puanlılar
// üretim sırasını korur (eski insertion sort ile birebir aynı sıra).
Move16 MovePicker::pickBest(int begin, int end) {
    int best = begin;
    int bestScore = scores[begin];
    for (int i = begin + 1; i < end; i++) {
        if (scores[i] > bestScore) { bestScore = scores[i]; best = i; }
    }
    Move16 m = moves[best];
    for (int i = best; i > begin; i--) {
        moves[i]  = moves[i - 1];
        scores[i] = scores[i - 1];
    }
    moves[begin]  = m;
    scores[begin] = bestScore;
    return m;
}

Move16 MovePicker::nextMove() {
    while (true) {
        switch (stage) {
            case STAGE_TT:
                generateAndPartition();
                stage = STAGE_GOOD_CAPTURES;
                moveIndex = 0;
                if (ttMove != 0) return ttMove;
                break;

            case STAGE_GOOD_CAPTURES:
                if (moveIndex < goodEnd) {
                    Move16 m = pickBest(moveIndex, goodEnd);
                    moveIndex++;
                    return m;
                }
                stage = STAGE_KILLERS;
                moveIndex = 0;
                break;

            case STAGE_KILLERS:
                if (moveIndex == 0) {
                    moveIndex++;
                    if (killer1Ok) return killer1;
                } else if (moveIndex == 1) {
                    moveIndex++;
                    if (killer2Ok) return killer2;
                } else {
                    // Quiet puanlarını şimdi oku (güncel history)
                    const int color = board.sideToMove;
                    for (int i = goodEnd; i < quietEnd; i++) {
                        Move16 m = moves[i];
                        int s = history[color][moveFrom(m)][moveTo(m)];
                        if (isCastle(m)) s += 100000; // Rok hamlelerini quiet sıralamasında en öne al
                        scores[i] = s;
                    }
                    stage = STAGE_QUIETS;
                    moveIndex = goodEnd;
                }
                break;

            case STAGE_QUIETS:
                if (moveIndex < quietEnd) {
                    Move16 m = pickBest(moveIndex, quietEnd);
                    moveIndex++;
                    return m;
                }
                stage = STAGE_BAD_CAPTURES;
                moveIndex = quietEnd;
                break;

            case STAGE_BAD_CAPTURES:
                if (moveIndex < badEnd) {
                    return moves[moveIndex++];
                }
                return 0; // HAMLE BİTTİ

            default: // STAGE_GEN_CAPTURES / STAGE_GEN_QUIETS artık kullanılmıyor
                return 0;
        }
    }
}
