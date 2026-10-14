#include "MoveGenerator.h"

namespace MoveGen {

    const uint64_t FILE_A = 0x0101010101010101ULL;
    const uint64_t FILE_H = 0x8080808080808080ULL;

    inline void addPromotionMoves(int from, int to, MoveList& moves, bool isCapture) {
        int baseFlag = isCapture ? FLAG_KNIGHT_PROMO_CAP : FLAG_KNIGHT_PROMO;
        // Queen=3, Rook=2, Bishop=1, Knight=0
        moves.add(encodeMove(from, to, baseFlag + 3)); // Queen
        moves.add(encodeMove(from, to, baseFlag + 2)); // Rook
        moves.add(encodeMove(from, to, baseFlag + 1)); // Bishop
        moves.add(encodeMove(from, to, baseFlag + 0)); // Knight
    }

    template<int Side, bool TacticalOnly>
    void generatePawns(const BoardState& board, MoveList& moves) {
        uint64_t pawns = board.pieces[Side][PAWN_IDX];
        if (!pawns) return;

        uint64_t empty = ~board.allOccupancy;
        uint64_t enemies = board.occupancy[Side ^ 1];
        
        constexpr int upDir = (Side == WHITE_SIDE) ? 8 : -8;
        constexpr int leftDir = (Side == WHITE_SIDE) ? 7 : -9;  // e.g. B2(9) + 7 = A3(16). So Left is +7 for White. Wait.
        // Let's verify: White A2(8), B2(9), C2(10). B2 capturing to A3(16) -> 16 - 9 = +7. Correct.
        // Black B7(49) capturing to A6(40) -> 40 - 49 = -9. Correct. Left meaning towards A file.
        constexpr int rightDir = (Side == WHITE_SIDE) ? 9 : -7; 
        
        constexpr uint64_t TRNK_PROMO = (Side == WHITE_SIDE) ? RANK_8 : RANK_1;
        constexpr uint64_t TRNK_DBL   = (Side == WHITE_SIDE) ? RANK_4 : RANK_5;

        // 1. Pushes
        if constexpr (!TacticalOnly) {
            uint64_t singlePush = (Side == WHITE_SIDE) ? (pawns << 8) : (pawns >> 8);
            singlePush &= empty;
            
            uint64_t doublePush = (Side == WHITE_SIDE) ? (singlePush << 8) : (singlePush >> 8);
            doublePush &= empty;
            doublePush &= TRNK_DBL;

            uint64_t promoPush = singlePush & TRNK_PROMO;
            singlePush &= ~TRNK_PROMO;

            while (singlePush) {
                int to = popLsb(singlePush);
                moves.add(encodeMove(to - upDir, to, FLAG_QUIET));
            }
            while (doublePush) {
                int to = popLsb(doublePush);
                moves.add(encodeMove(to - upDir * 2, to, FLAG_DOUBLE_PAWN));
            }
            while (promoPush) {
                int to = popLsb(promoPush);
                addPromotionMoves(to - upDir, to, moves, false);
            }
        } else {
            // Tactical only: we still generate Queen promotion pushes
            uint64_t singlePush = (Side == WHITE_SIDE) ? (pawns << 8) : (pawns >> 8);
            singlePush &= empty;
            uint64_t promoPush = singlePush & TRNK_PROMO;
            while (promoPush) {
                int to = popLsb(promoPush);
                moves.add(encodeMove(to - upDir, to, FLAG_QUEEN_PROMO)); // only the Queen promo
            }
        }

        // 2. Captures
        uint64_t capLeft = (Side == WHITE_SIDE) ? ((pawns & ~FILE_A) << 7) : ((pawns & ~FILE_A) >> 9);
        uint64_t capRight = (Side == WHITE_SIDE) ? ((pawns & ~FILE_H) << 9) : ((pawns & ~FILE_H) >> 7);

        uint64_t validLeft = capLeft & enemies;
        uint64_t validRight = capRight & enemies;

        uint64_t promoLeft = validLeft & TRNK_PROMO;
        uint64_t promoRight = validRight & TRNK_PROMO;

        validLeft &= ~TRNK_PROMO;
        validRight &= ~TRNK_PROMO;

        while (validLeft) {
            int to = popLsb(validLeft);
            moves.add(encodeMove(to - leftDir, to, FLAG_CAPTURE));
        }
        while (validRight) {
            int to = popLsb(validRight);
            moves.add(encodeMove(to - rightDir, to, FLAG_CAPTURE));
        }
        while (promoLeft) {
            int to = popLsb(promoLeft);
            if (TacticalOnly) moves.add(encodeMove(to - leftDir, to, FLAG_QUEEN_PROMO_CAP));
            else              addPromotionMoves(to - leftDir, to, moves, true);
        }
        while (promoRight) {
            int to = popLsb(promoRight);
             if (TacticalOnly) moves.add(encodeMove(to - rightDir, to, FLAG_QUEEN_PROMO_CAP));
             else              addPromotionMoves(to - rightDir, to, moves, true);
        }

        // 3. En Passant
        if (board.enPassantSq != SQ_NONE) {
            uint64_t epBit = 1ULL << board.enPassantSq;
            uint64_t epLeft = capLeft & epBit;
            uint64_t epRight = capRight & epBit;
            
            if (epLeft) {
                moves.add(encodeMove(board.enPassantSq - leftDir, board.enPassantSq, FLAG_EP_CAPTURE));
            }
            if (epRight) {
                moves.add(encodeMove(board.enPassantSq - rightDir, board.enPassantSq, FLAG_EP_CAPTURE));
            }
        }
    }

    template<bool TacticalOnly>
    void generatePieceMoves(const BoardState& board, MoveList& moves) {
        int side = board.sideToMove;
        uint64_t friendly = board.occupancy[side];
        uint64_t enemies = board.occupancy[side ^ 1];
        uint64_t allPieces = friendly | enemies;
        uint64_t targetMask = TacticalOnly ? enemies : ~friendly;

        // Knights
        uint64_t knights = board.pieces[side][KNIGHT_IDX];
        while (knights) {
            int sq = popLsb(knights);
            uint64_t attacks = Attacks::getKnightAttacks(sq) & targetMask;
            while (attacks) {
                int to = popLsb(attacks);
                moves.add(encodeMove(sq, to, testBit(enemies, to) ? FLAG_CAPTURE : FLAG_QUIET));
            }
        }

        // Kings
        uint64_t kings = board.pieces[side][KING_IDX];
        if (kings) {
            int sq = lsb(kings);
            uint64_t attacks = Attacks::getKingAttacks(sq) & targetMask;
            while (attacks) {
                int to = popLsb(attacks);
                moves.add(encodeMove(sq, to, testBit(enemies, to) ? FLAG_CAPTURE : FLAG_QUIET));
            }
        }

        // Bishops
        uint64_t bishops = board.pieces[side][BISHOP_IDX];
        while (bishops) {
            int sq = popLsb(bishops);
            uint64_t attacks = Magics::getBishopAttacks(sq, allPieces) & targetMask;
            while (attacks) {
                int to = popLsb(attacks);
                moves.add(encodeMove(sq, to, testBit(enemies, to) ? FLAG_CAPTURE : FLAG_QUIET));
            }
        }

        // Rooks
        uint64_t rooks = board.pieces[side][ROOK_IDX];
        while (rooks) {
            int sq = popLsb(rooks);
            uint64_t attacks = Magics::getRookAttacks(sq, allPieces) & targetMask;
            while (attacks) {
                int to = popLsb(attacks);
                moves.add(encodeMove(sq, to, testBit(enemies, to) ? FLAG_CAPTURE : FLAG_QUIET));
            }
        }

        // Queens
        uint64_t queens = board.pieces[side][QUEEN_IDX];
        while (queens) {
            int sq = popLsb(queens);
            uint64_t attacks = Magics::getQueenAttacks(sq, allPieces) & targetMask;
            while (attacks) {
                int to = popLsb(attacks);
                moves.add(encodeMove(sq, to, testBit(enemies, to) ? FLAG_CAPTURE : FLAG_QUIET));
            }
        }
        
        // Castling (Not generated in TacticalOnly mode)
        if constexpr (!TacticalOnly) {
            if (side == WHITE_SIDE) {
                if (board.castlingRights & CASTLE_WHITE_KING) {
                    if (!testBit(allPieces, F1) && !testBit(allPieces, G1))
                        moves.add(encodeMove(E1, G1, FLAG_KING_CASTLE));
                }
                if (board.castlingRights & CASTLE_WHITE_QUEEN) {
                    if (!testBit(allPieces, D1) && !testBit(allPieces, C1) && !testBit(allPieces, B1))
                        moves.add(encodeMove(E1, C1, FLAG_QUEEN_CASTLE));
                }
            } else {
                if (board.castlingRights & CASTLE_BLACK_KING) {
                    if (!testBit(allPieces, F8) && !testBit(allPieces, G8))
                        moves.add(encodeMove(E8, G8, FLAG_KING_CASTLE));
                }
                if (board.castlingRights & CASTLE_BLACK_QUEEN) {
                    if (!testBit(allPieces, D8) && !testBit(allPieces, C8) && !testBit(allPieces, B8))
                        moves.add(encodeMove(E8, C8, FLAG_QUEEN_CASTLE));
                }
            }
        }
    }

    template<bool TacticalOnly>
    void generateMoves(const BoardState& board, MoveList& moves) {
        if (board.sideToMove == WHITE_SIDE) {
            generatePawns<WHITE_SIDE, TacticalOnly>(board, moves);
        } else {
            generatePawns<BLACK_SIDE, TacticalOnly>(board, moves);
        }
        generatePieceMoves<TacticalOnly>(board, moves);
    }

    void generateAllMoves(const BoardState& board, MoveList& moves) {
        moves.count = 0;
        generateMoves<false>(board, moves);
    }

    void generateTacticalMoves(const BoardState& board, MoveList& moves) {
        moves.count = 0;
        generateMoves<true>(board, moves);
    }

} // namespace MoveGen
