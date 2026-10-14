#include "EvalParams.h"
#include <cstring>

EvalParams evalParams;

void EvalParams::initDefaults() {
    pawnVal   = 100;
    knightVal = 320;
    bishopVal = 322;
    rookVal   = 515;
    queenVal  = 945;
    kingVal   = 20000;

    bishopPair       = 61;
    rookOpenFile     = 28;
    rookSemiOpenFile = 23;
    castleBonus      = 38;

    isolatedPawn     = 11;
    passedPawnRank4  = 13;
    passedPawnRank5  = 34;
    passedPawnRank6  = 65;
    passedPawnRank7  = 95;
    rookOn7th        = 7;
    knightMobility   = 2;
    bishopMobility   = 4;

    static const int DEFAULT_PAWN_TABLE[64] = {
          0,   0,   0,   0,   0,   0,   0,   0,
          5,  13,   9,   5,   5,   9,  13,   5,
          0,   0,   0,   4,   4,   0,   0,   0,
         -6,  -3,  -5,   9,   9,  -5,  -3,  -6,
         20,   7,  -1,   1,   1,  -1,   7,  20,
         50,  40,  31,   5,   5,  31,  40,  50,
         50,  43,  46,  25,  25,  46,  43,  50,
          0,   0,   0,   0,   0,   0,   0,   0
    };

    static const int DEFAULT_KNIGHT_TABLE[64] = {
        -55, -19, -14, -16, -16, -14, -19, -55,
          2, -19,  -2,  14,  14,  -2, -19,   2,
        -19,   1,  24,  15,  15,  24,   1, -19,
         -9,  -5,  19,  27,  27,  19,  -5,  -9,
         -2,  16,  21,  34,  34,  21,  16,  -2,
        -26, -12,   9,  18,  18,   9, -12, -26,
        -55, -37,  20, -14, -14,  20, -37, -55,
        -70, -52, -70, -28, -28, -70, -52, -70
    };

    static const int DEFAULT_BISHOP_TABLE[64] = {
        -23, -24, -19,  -7,  -7, -19, -24, -23,
        -13,  15,  -1,  -4,  -4,  -1,  15, -13,
         -1,   2,   3,   3,   3,   3,   2,  -1,
        -32,  -8,  -8,   7,   7,  -8,  -8, -32,
         -9, -16, -13,   6,   6, -13, -16,  -9,
         -8,  -5,  -4,   0,   0,  -4,  -5,  -8,
        -37, -28, -30, -27, -27, -30, -28, -37,
        -25, -38, -30, -45, -45, -30, -38, -25
    };

    static const int DEFAULT_ROOK_TABLE[64] = {
         -1,   3,  11,  12,  12,  11,   3,  -1,
         -9,   1,  -3,  -3,  -3,  -3,   1,  -9,
         -6,   9,  -5,   3,   3,  -5,   9,  -6,
         -3,   4,  -4,  -1,  -1,  -4,   4,  -3,
         12,   4,  10,   5,   5,  10,   4,  12,
         14,  18,   8,  12,  12,   8,  18,  14,
         11,  15,  12,   6,   6,  12,  15,  11,
         31,  16,  14,  21,  21,  14,  16,  31
    };

    static const int DEFAULT_QUEEN_TABLE[64] = {
         25,   5,  14,  22,  22,  14,   5,  25,
         16,  30,  29,  30,  30,  29,  30,  16,
         -1,  25,  29,  30,  30,  29,  25,  -1,
         10,  30,  22,  30,  30,  22,  30,  10,
         17,  19,  30,  30,  30,  30,  19,  17,
         30,  13,  29,  30,  30,  29,  13,  30,
          5,  -5,  30,  24,  24,  30,  -5,   5,
         17,  30,  30,  30,  30,  30,  30,  17
    };

    static const int DEFAULT_KING_MG_TABLE[64] = {
        -40, -18, -48, -18, -18, -48, -18, -40,
         15,  -2, -43, -70, -70, -43,  -2,  15,
         15, -41, -56, -58, -58, -56, -41,  15,
        -70, -51, -45, -33, -33, -45, -51, -70,
        -15, -30, -70, -70, -70, -70, -30, -15,
        -15,  25, -65, -37, -37, -65,  25, -15,
         50,  15,  -5,  45,  45,  -5,  15,  50,
         50,  50,  15,   5,   5,  15,  50,  50
    };

    static const int DEFAULT_KING_EG_TABLE[64] = {
        -47, -18, -24, -26, -26, -24, -18, -47,
        -22,   2,   6,   1,   1,   6,   2, -22,
        -14,  -1,  12,   9,   9,  12,  -1, -14,
        -25,  -7,   8,  15,  15,   8,  -7, -25,
         -5,   9,  15,  15,  15,  15,   9,  -5,
         -5,  15,  19,   5,   5,  19,  15,  -5,
         -5,  -5,   9, -10, -10,   9,  -5,  -5,
        -70, -11,  -5, -24, -24,  -5, -11, -70
    };

    std::memcpy(pawnTable,    DEFAULT_PAWN_TABLE,    sizeof(pawnTable));
    std::memcpy(knightTable,  DEFAULT_KNIGHT_TABLE,  sizeof(knightTable));
    std::memcpy(bishopTable,  DEFAULT_BISHOP_TABLE,  sizeof(bishopTable));
    std::memcpy(rookTable,    DEFAULT_ROOK_TABLE,    sizeof(rookTable));
    std::memcpy(queenTable,   DEFAULT_QUEEN_TABLE,   sizeof(queenTable));
    std::memcpy(kingMgTable,  DEFAULT_KING_MG_TABLE, sizeof(kingMgTable));
    std::memcpy(kingEgTable,  DEFAULT_KING_EG_TABLE, sizeof(kingEgTable));
}

// Otomatik başlatıcı: program açıldığında varsayılanlar yüklenir
struct AutoInitEvalParams {
    AutoInitEvalParams() { evalParams.initDefaults(); }
};
static AutoInitEvalParams autoInitParams;
