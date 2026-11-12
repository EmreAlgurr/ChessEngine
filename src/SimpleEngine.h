#pragma once

#include "BitBoard.h"
#include "MoveGenerator.h"
#include "MemoryPool.h"
#include <cstdint>
#include <vector>
#include "TranspositionTable.h"
#include "nnue.h"

struct alignas(64) NNUEdataAligned {
    NNUEdata data;
};

enum MovePickerStage {
    STAGE_TT,
    STAGE_GOOD_CAPTURES,
    STAGE_KILLERS,
    STAGE_QUIETS,
    STAGE_BAD_CAPTURES
};

// ------------------------------------------------------------
//  MovePicker (tek üretim + selection sort)
//  - Düğüm başına generateAllMoves SADECE 1 kez çağrılır.
//  - Hamleler üretimden sonra kategorilere ayrılır (partition):
//      [0, goodEnd)          : good captures (SEE >= 0) + queen promo
//      [goodEnd, quietEnd)   : quiet hamleler (history ile sıralanır)
//      [quietEnd, badEnd)    : bad captures (SEE < 0)
//  - Her aşamada kalan en yüksek puanlı hamle seçilerek (selection)
//    döndürülür; sıralama tembeldir, beta kesmesi olursa geri kalan
//    hamleler hiç sıralanmaz.
// ------------------------------------------------------------
struct MovePicker {
    BoardState& board;
    uint16_t ttMove;
    uint16_t killer1;
    uint16_t killer2;
    int (*history)[64][64];
    int (*captureHistory)[64][6];

    MovePickerStage stage;
    Move16 moves[256];
    int    scores[256];
    int    moveIndex;   // aktif aşamadaki okuma imleci
    int    goodEnd;     // good captures bitişi
    int    quietEnd;    // quiet hamleler bitişi
    int    badEnd;      // bad captures bitişi
    bool   killer1Ok;   // killer1 bu pozisyonda pseudo-legal mi?
    bool   killer2Ok;

    MovePicker(BoardState& b, uint16_t ttM, uint16_t k1, uint16_t k2, int hist[2][64][64], int capHist[64][64][6]);
    Move16 nextMove();

private:
    void   generateAndPartition();
    Move16 pickBest(int begin, int end); // [begin,end) içinden en iyiyi begin'e taşır (stabil)
};

class SimpleEngine {
public:
    SimpleEngine();
    explicit SimpleEngine(int depth);

    void setDepth(int depth);
    void setHashSize(int sizeInMB) { tt.resize(sizeInMB); }
    Move16 getBestMove(BoardState& board, int color, long timeLimitMs);
    int64_t getNodesSearched() const;
    void clearTT() { tt.clear(); }
    void clearHistoryAndKillers();

    // Oyun gecmisi (kok pozisyon DAHIL, son eleman = kok). Zero-copy span referansi.
    void setGameHistory(const uint64_t* keys, size_t count) {
        gameKeysPtr = keys;
        gameKeysCount = count;
    }
    void setGameHistory(const std::vector<uint64_t>& keys) {
        gameKeysPtr = keys.data();
        gameKeysCount = keys.size();
    }

    int evaluate(const BoardState& board, int activeColor, int ply = -1);

private:
    TranspositionTable tt;
    MemoryPool searchPool; // Contiguous 64-byte aligned arena for all search buffers
    int maxDepth;
    static constexpr int MAX_PLY = 128;
    int64_t searchSoftEndNs = 0;
    int64_t searchHardEndNs = 0;
    bool stopSearch;
    int64_t nodesSearched;

    NNUEdataAligned* nnueStack = nullptr;
    void recordMoveNNUE(int nextPly, Move16 m, int movingPiece, int side, const UndoState& undo);

    uint16_t (*killerMoves)[2] = nullptr;
    int (*history)[64][64] = nullptr;
    int (*captureHistory)[64][6] = nullptr;

    // --- Tekrar (repetition) tespiti ---
    static constexpr int MAX_GAME_KEYS = 1024;
    const uint64_t* gameKeysPtr = nullptr;                // zero-copy pointer to UCI history
    size_t          gameKeysCount = 0;
    uint64_t*       keyStack = nullptr;                   // gecmis + arama yolu (searchPool)
    int             rootIndex = 0;                        // kokun keyStack indeksi
    bool isRepetition(const BoardState& board, int ply);

    void initSearchPool();

    // Piece Values
    static constexpr int PAWN_VAL   = 100;
    static constexpr int KNIGHT_VAL = 320;
    static constexpr int BISHOP_VAL = 330;
    static constexpr int ROOK_VAL   = 500;
    static constexpr int QUEEN_VAL  = 900;
    static constexpr int KING_VAL   = 20000;

    // Piece-Square Tables (mirrored internally for black)
    static const int PAWN_TABLE[64];
    static const int KNIGHT_TABLE[64];
    static const int BISHOP_TABLE[64];
    static const int ROOK_TABLE[64];
    static const int QUEEN_TABLE[64];
    static const int KING_MG_TABLE[64];  // middlegame
    static const int KING_EG_TABLE[64];  // endgame

    bool isEndgame(const BoardState& board) const;

    int negamax(BoardState& board, int depth, int ply, int alpha, int beta, int color, bool allowNull = true, Move16 excludedMove = 0);
    int quiescence(BoardState& board, int alpha, int beta, int ply, int color);

    int getBasicValue(int pt) const;
    int getPsqBonus(int pt, int sq, int side) const;

    void orderMoves(MoveList& moves, const BoardState& board, uint16_t ttMove, int ply, int color);
    int scoreMove(Move16 m, const BoardState& board, uint16_t ttMove, int ply, int color);
};
