#include "Zobrist.h"
#include <random>

namespace Zobrist {

    uint64_t pieceKeys[SIDE_COUNT][PIECE_COUNT][64];
    uint64_t sideToMoveKey;
    uint64_t castlingKeys[16];
    uint64_t enPassantKeys[8];

    // ============================================================
    //  Initialization
    // ============================================================
    void init() {
        // High quality PRNG (fixed seed to ensure reproducible hashes across runs)
        std::mt19937_64 rng(1804289383);
        
        // 1. Piece keys
        for (int side = WHITE_SIDE; side <= BLACK_SIDE; side++) {
            for (int pc = PAWN_IDX; pc <= KING_IDX; pc++) {
                for (int sq = 0; sq < 64; sq++) {
                    pieceKeys[side][pc][sq] = rng();
                }
            }
        }

        // 2. Side to move key
        sideToMoveKey = rng();

        // 3. Castling keys
        for (int i = 0; i < 16; i++) {
            castlingKeys[i] = rng();
        }

        // 4. En Passant keys (only for files)
        for (int i = 0; i < 8; i++) {
            enPassantKeys[i] = rng();
        }
    }

    // ============================================================
    //  Compute Key Fresh from Board
    // ============================================================
    uint64_t computeKey(const BoardState& board) {
        uint64_t hash = 0;

        // Pieces
        for (int side = WHITE_SIDE; side <= BLACK_SIDE; side++) {
            for (int pc = PAWN_IDX; pc <= KING_IDX; pc++) {
                uint64_t bb = board.pieces[side][pc];
                while (bb) {
                    int sq = popLsb(bb);
                    hash ^= pieceKeys[side][pc][sq];
                }
            }
        }

        // Side to move (only applied if Black is to move)
        if (board.sideToMove == BLACK_SIDE) {
            hash ^= sideToMoveKey;
        }

        // Castling rights
        hash ^= castlingKeys[board.castlingRights];

        // En passant
        if (board.enPassantSq != SQ_NONE) {
            hash ^= enPassantKeys[fileOf(board.enPassantSq)];
        }

        return hash;
    }

} // namespace Zobrist
