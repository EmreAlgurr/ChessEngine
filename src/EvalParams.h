#pragma once

struct EvalParams {
    int pawnVal;
    int knightVal;
    int bishopVal;
    int rookVal;
    int queenVal;
    int kingVal;

    int bishopPair;
    int rookOpenFile;
    int rookSemiOpenFile;
    int castleBonus;

    int isolatedPawn;
    int passedPawnRank4;
    int passedPawnRank5;
    int passedPawnRank6;
    int passedPawnRank7;
    int rookOn7th;
    int knightMobility;
    int bishopMobility;

    int pawnTable[64];
    int knightTable[64];
    int bishopTable[64];
    int rookTable[64];
    int queenTable[64];
    int kingMgTable[64];
    int kingEgTable[64];

    void initDefaults();
};

extern EvalParams evalParams;
