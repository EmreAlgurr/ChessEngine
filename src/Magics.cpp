#include "Magics.h"
#include <iostream>

namespace Magics {

    // ============================================================
    //  Constants: Magic Numbers & Shifts
    //  Pre-calculated values from well-known generators
    // ============================================================

    const int RookShifts[64] = {
        52, 53, 53, 53, 53, 53, 53, 52,
        53, 54, 54, 54, 54, 54, 54, 53,
        53, 54, 54, 54, 54, 54, 54, 53,
        53, 54, 54, 54, 54, 54, 54, 53,
        53, 54, 54, 54, 54, 54, 54, 53,
        53, 54, 54, 54, 54, 54, 54, 53,
        53, 54, 54, 54, 54, 54, 54, 53,
        52, 53, 53, 53, 53, 53, 53, 52
    };

    const int BishopShifts[64] = {
        58, 59, 59, 59, 59, 59, 59, 58,
        59, 59, 59, 59, 59, 59, 59, 59,
        59, 59, 57, 57, 57, 57, 59, 59,
        59, 59, 57, 55, 55, 57, 59, 59,
        59, 59, 57, 55, 55, 57, 59, 59,
        59, 59, 57, 57, 57, 57, 59, 59,
        59, 59, 59, 59, 59, 59, 59, 59,
        58, 59, 59, 59, 59, 59, 59, 58
    };

    // Offsets directly calculate accumulated table sizes
    // Total rook table size:   102400 (sum of 2^(64-RookShifts))
    // Total bishop table size: 5248   (sum of 2^(64-BishopShifts))
    const int RookOffsets[64] = {
         0,    4096,  6144,  8192,  10240, 12288, 14336, 16384,
         20480,22528, 23552, 24576, 25600, 26624, 27648, 28672,
         30720,32768, 33792, 34816, 35840, 36864, 37888, 38912,
         40960,43008, 44032, 45056, 46080, 47104, 48128, 49152,
         51200,53248, 54272, 55296, 56320, 57344, 58368, 59392,
         61440,63488, 64512, 65536, 66560, 67584, 68608, 69632,
         71680,73728, 74752, 75776, 76800, 77824, 78848, 79872,
         81920,86016, 88064, 90112, 92160, 94208, 96256, 98304
    };

    const int BishopOffsets[64] = {
         0, 64,  96,  128, 160, 192, 224, 256,
         320, 352, 384, 416, 448, 480, 512, 544,
         576, 608, 640, 768, 896, 1024,1152,1280,
         1312,1344,1376,1888,2400,2912,3040,3072,
         3104,3136,3168,3680,4192,4704,4832,4864,
         4896,4928,4960,5088,5216,5344,5472,5504,
         5536,5568,5600,5632,5664,5696,5728,5760,
         5792,5856,5888,5920,5952,5984,6016,6048 // Last 64 not needed technically, array overlaps slightly 
    };

    uint64_t RookMagics[64] = {0};
    uint64_t BishopMagics[64] = {0};

    uint64_t RookAttacksTable[102400];
    uint64_t BishopAttacksTable[10240];

    uint64_t RookMasks[64];
    uint64_t BishopMasks[64];

    // ============================================================
    //  Helper: Generate relevant movement mask for a square
    //  Must not include edge squares to avoid wrapping bugs!
    // ============================================================
    uint64_t genRookMask(int sq) {
        uint64_t mask = 0;
        int r = sq / 8, f = sq % 8;

        for (int i = r + 1; i <= 6; i++) setBit(mask, i * 8 + f);
        for (int i = r - 1; i >= 1; i--) setBit(mask, i * 8 + f);
        for (int i = f + 1; i <= 6; i++) setBit(mask, r * 8 + i);
        for (int i = f - 1; i >= 1; i--) setBit(mask, r * 8 + i);
        return mask;
    }

    uint64_t genBishopMask(int sq) {
        uint64_t mask = 0;
        int r = sq / 8, f = sq % 8;

        for (int i = r + 1, j = f + 1; i <= 6 && j <= 6; i++, j++) setBit(mask, i * 8 + j);
        for (int i = r + 1, j = f - 1; i <= 6 && j >= 1; i++, j--) setBit(mask, i * 8 + j);
        for (int i = r - 1, j = f + 1; i >= 1 && j <= 6; i--, j++) setBit(mask, i * 8 + j);
        for (int i = r - 1, j = f - 1; i >= 1 && j >= 1; i--, j--) setBit(mask, i * 8 + j);
        return mask;
    }

    // ============================================================
    //  Helper: Generate accurate sliding attacks given blockages
    // ============================================================
    uint64_t genRookAttacks(int sq, uint64_t block) {
        uint64_t attacks = 0;
        int r = sq / 8, f = sq % 8;

        for (int i = r + 1; i <= 7; i++) { setBit(attacks, i * 8 + f); if (testBit(block, i * 8 + f)) break; }
        for (int i = r - 1; i >= 0; i--) { setBit(attacks, i * 8 + f); if (testBit(block, i * 8 + f)) break; }
        for (int i = f + 1; i <= 7; i++) { setBit(attacks, r * 8 + i); if (testBit(block, r * 8 + i)) break; }
        for (int i = f - 1; i >= 0; i--) { setBit(attacks, r * 8 + i); if (testBit(block, r * 8 + i)) break; }
        return attacks;
    }

    uint64_t genBishopAttacks(int sq, uint64_t block) {
        uint64_t attacks = 0;
        int r = sq / 8, f = sq % 8;

        for (int i = r + 1, j = f + 1; i <= 7 && j <= 7; i++, j++) { setBit(attacks, i * 8 + j); if (testBit(block, i * 8 + j)) break; }
        for (int i = r + 1, j = f - 1; i <= 7 && j >= 0; i++, j--) { setBit(attacks, i * 8 + j); if (testBit(block, i * 8 + j)) break; }
        for (int i = r - 1, j = f + 1; i >= 0 && j <= 7; i--, j++) { setBit(attacks, i * 8 + j); if (testBit(block, i * 8 + j)) break; }
        for (int i = r - 1, j = f - 1; i >= 0 && j >= 0; i--, j--) { setBit(attacks, i * 8 + j); if (testBit(block, i * 8 + j)) break; }
        return attacks;
    }

    // ============================================================
    //  Helper: Set an occupancy variation based on an index and mask
    // ============================================================
    uint64_t setOccupancy(int index, int bitsInMask, uint64_t attackMask) {
        uint64_t occupancy = 0;
        for (int i = 0; i < bitsInMask; i++) {
            int square = lsb(attackMask);
            clearBit(attackMask, square);
            if (index & (1 << i)) {
                setBit(occupancy, square);
            }
        }
        return occupancy;
    }

    // ============================================================
    //  PRNG for Magic Generation
    // ============================================================
    uint32_t state = 1804289383;
    uint32_t getRandomU32() {
        uint32_t number = state;
        number ^= number << 13;
        number ^= number >> 17;
        number ^= number << 5;
        state = number;
        return number;
    }
    uint64_t getRandomU64() {
        uint64_t n1 = (uint64_t)(getRandomU32()) & 0xFFFF;
        uint64_t n2 = (uint64_t)(getRandomU32()) & 0xFFFF;
        uint64_t n3 = (uint64_t)(getRandomU32()) & 0xFFFF;
        uint64_t n4 = (uint64_t)(getRandomU32()) & 0xFFFF;
        return n1 | (n2 << 16) | (n3 << 32) | (n4 << 48);
    }
    uint64_t getRandomMagicCandidate() {
        return getRandomU64() & getRandomU64() & getRandomU64(); // Few bits set algorithm
    }

    uint64_t findMagic(int sq, int m, bool isBishop) {
        uint64_t occupancies[4096];
        uint64_t attacks[4096];
        uint64_t used[4096];
        
        uint64_t mask = isBishop ? genBishopMask(sq) : genRookMask(sq);
        int n = popCount(mask);
        int numVariations = 1 << n;

        for (int i = 0; i < numVariations; i++) {
            occupancies[i] = setOccupancy(i, n, mask);
            attacks[i] = isBishop ? genBishopAttacks(sq, occupancies[i]) : genRookAttacks(sq, occupancies[i]);
        }

        for (int k = 0; k < 100000000; k++) {
            uint64_t magic = getRandomMagicCandidate();
            if (popCount((mask * magic) & 0xFF00000000000000ULL) < 6) continue;
            
            for (int i = 0; i < 4096; i++) used[i] = 0ULL;
            bool fail = false;
            
            for (int i = 0; !fail && i < numVariations; i++) {
                int magicIndex = static_cast<int>((occupancies[i] * magic) >> (64 - m));
                if (used[magicIndex] == 0ULL) {
                    used[magicIndex] = attacks[i];
                } else if (used[magicIndex] != attacks[i]) {
                    fail = true;
                }
            }
            if (!fail) return magic;
        }
        std::cerr << "Failed to find magic!" << std::endl;
        return 0ULL;
    }

    // ============================================================
    //  Main Magic INITIALIZATION
    // ============================================================
    void initMagics() {
        // Run once for all squares
        for (int sq = 0; sq < 64; sq++) {

            // ROOKS
            RookMasks[sq] = genRookMask(sq);
            int rookBits = popCount(RookMasks[sq]);
            int rookVariations = 1 << rookBits;

            // Dynamically search magic if not set:
            if (RookMagics[sq] == 0) {
                RookMagics[sq] = findMagic(sq, 64 - RookShifts[sq], false);
            }

            for (int i = 0; i < rookVariations; i++) {
                uint64_t occupancy = setOccupancy(i, rookBits, RookMasks[sq]);
                int magicIndex = static_cast<int>((occupancy * RookMagics[sq]) >> RookShifts[sq]);
                RookAttacksTable[RookOffsets[sq] + magicIndex] = genRookAttacks(sq, occupancy);
            }

            // BISHOPS
            BishopMasks[sq] = genBishopMask(sq);
            int bishopBits = popCount(BishopMasks[sq]);
            int bishopVariations = 1 << bishopBits;

            if (BishopMagics[sq] == 0) {
                BishopMagics[sq] = findMagic(sq, 64 - BishopShifts[sq], true);
            }

            for (int i = 0; i < bishopVariations; i++) {
                uint64_t occupancy = setOccupancy(i, bishopBits, BishopMasks[sq]);
                int magicIndex = static_cast<int>((occupancy * BishopMagics[sq]) >> BishopShifts[sq]);
                BishopAttacksTable[BishopOffsets[sq] + magicIndex] = genBishopAttacks(sq, occupancy);
            }
        }
    }
} // namespace Magics
