#include "BitBoard.h"
#include <sstream>
#include <cctype>
#include <cassert>
#include "Zobrist.h"
#include "LeaperAttacks.h"
#include "Magics.h"

// Castling rights updates when moving to/from these squares
const int CASTLE_UPDATES[64] = {
    13, 15, 15, 15, 12, 15, 15, 14, // a1..h1
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
     7, 15, 15, 15,  3, 15, 15, 11  // a8..h8
};

// ============================================================
//  BoardState::clear  —  zero everything
// ============================================================
void BoardState::clear() {
    for (int s = 0; s < SIDE_COUNT; s++)
        for (int p = 0; p < PIECE_COUNT; p++)
            pieces[s][p] = 0ULL;

    occupancy[WHITE_SIDE] = 0ULL;
    occupancy[BLACK_SIDE] = 0ULL;
    allOccupancy          = 0ULL;

    sideToMove     = WHITE_SIDE;
    castlingRights = CASTLE_NONE;
    enPassantSq    = SQ_NONE;
    halfMoveClock  = 0;
    fullMoveNumber = 1;
    zobristKey     = 0ULL;
}

// ============================================================
//  BoardState::updateOccupancy
// ============================================================
void BoardState::updateOccupancy() {
    for (int s = 0; s < SIDE_COUNT; s++) {
        occupancy[s] = 0ULL;
        for (int p = 0; p < PIECE_COUNT; p++)
            occupancy[s] |= pieces[s][p];
    }
    allOccupancy = occupancy[WHITE_SIDE] | occupancy[BLACK_SIDE];
}

// ============================================================
//  BoardState::pieceOnSquare  —  which piece is on sq?
//  Returns PieceIndex (0-5) or -1 if empty
// ============================================================
int BoardState::pieceOnSquare(int sq) const {
    uint64_t mask = sqBB(sq);
    if (!(allOccupancy & mask)) return -1;

    for (int s = 0; s < SIDE_COUNT; s++) {
        for (int p = 0; p < PIECE_COUNT; p++) {
            if (pieces[s][p] & mask) return p;
        }
    }
    return -1;
}

// ============================================================
//  BoardState::colorOnSquare  —  which side is on sq?
//  Returns WHITE_SIDE, BLACK_SIDE, or -1 if empty
// ============================================================
int BoardState::colorOnSquare(int sq) const {
    uint64_t mask = sqBB(sq);
    if (occupancy[WHITE_SIDE] & mask) return WHITE_SIDE;
    if (occupancy[BLACK_SIDE] & mask) return BLACK_SIDE;
    return -1;
}

// ============================================================
//  FEN  →  BoardState
//
//  Standard FEN example (starting position):
//  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
// ============================================================
void BoardState::setFromFEN(const std::string& fen) {
    clear();

    std::istringstream ss(fen);
    std::string piecePlacement, activeColor, castling, epSquare;
    int halfmove = 0, fullmove = 1;

    ss >> piecePlacement >> activeColor >> castling >> epSquare;
    if (ss >> halfmove) halfMoveClock = halfmove;
    if (ss >> fullmove) fullMoveNumber = fullmove;

    // ---- 1. Piece Placement ----
    // FEN ranks go from rank 8 (top) to rank 1 (bottom)
    // Our bitboard: rank 0 = rank 1 (a1 = bit 0)
    int rank = 7;  // start from rank 8 (index 7)
    int file = 0;

    for (char ch : piecePlacement) {
        if (ch == '/') {
            rank--;
            file = 0;
            continue;
        }
        if (std::isdigit(ch)) {
            file += (ch - '0');
            continue;
        }

        // Determine side and piece type
        int side = std::isupper(ch) ? WHITE_SIDE : BLACK_SIDE;
        int pieceIdx = -1;

        switch (std::tolower(ch)) {
            case 'p': pieceIdx = PAWN_IDX;   break;
            case 'n': pieceIdx = KNIGHT_IDX; break;
            case 'b': pieceIdx = BISHOP_IDX; break;
            case 'r': pieceIdx = ROOK_IDX;   break;
            case 'q': pieceIdx = QUEEN_IDX;  break;
            case 'k': pieceIdx = KING_IDX;   break;
            default:  break;
        }

        if (pieceIdx >= 0 && rank >= 0 && rank <= 7 && file >= 0 && file <= 7) {
            int sq = makeSquare(rank, file);
            setBit(pieces[side][pieceIdx], sq);
        }

        file++;
    }

    // ---- 2. Active Color ----
    sideToMove = (activeColor == "w") ? WHITE_SIDE : BLACK_SIDE;

    // ---- 3. Castling Rights ----
    castlingRights = CASTLE_NONE;
    if (castling != "-") {
        for (char c : castling) {
            switch (c) {
                case 'K': castlingRights |= CASTLE_WHITE_KING;  break;
                case 'Q': castlingRights |= CASTLE_WHITE_QUEEN; break;
                case 'k': castlingRights |= CASTLE_BLACK_KING;  break;
                case 'q': castlingRights |= CASTLE_BLACK_QUEEN; break;
                default: break;
            }
        }
    }

    // ---- 4. En Passant Square ----
    enPassantSq = SQ_NONE;
    if (epSquare != "-" && epSquare.size() == 2) {
        int f = epSquare[0] - 'a';  // file: 0-7
        int r = epSquare[1] - '1';  // rank: 0-7
        if (f >= 0 && f <= 7 && r >= 0 && r <= 7) {
            enPassantSq = makeSquare(r, f);
        }
    }

    // ---- 5. Recalculate aggregate occupancy ----
    updateOccupancy();

    // ---- 6. Initialize Zobrist Key ----
    zobristKey = Zobrist::computeKey(*this);
}

// ============================================================
//  BoardState  →  FEN string
// ============================================================
std::string BoardState::toFEN() const {
    std::string fen;

    // ---- 1. Piece Placement ----
    for (int rank = 7; rank >= 0; rank--) {
        int emptyCount = 0;
        for (int file = 0; file < 8; file++) {
            int sq = makeSquare(rank, file);
            int piece = pieceOnSquare(sq);
            int color = colorOnSquare(sq);

            if (piece < 0) {
                emptyCount++;
                continue;
            }

            if (emptyCount > 0) {
                fen += std::to_string(emptyCount);
                emptyCount = 0;
            }

            // Map piece index to FEN character
            static const char pieceChars[PIECE_COUNT] = {
                'p', 'n', 'b', 'r', 'q', 'k'
            };
            char c = pieceChars[piece];
            if (color == WHITE_SIDE) c = static_cast<char>(std::toupper(c));
            fen += c;
        }
        if (emptyCount > 0) fen += std::to_string(emptyCount);
        if (rank > 0) fen += '/';
    }

    // ---- 2. Active Color ----
    fen += (sideToMove == WHITE_SIDE) ? " w " : " b ";

    // ---- 3. Castling Rights ----
    if (castlingRights == CASTLE_NONE) {
        fen += '-';
    } else {
        if (castlingRights & CASTLE_WHITE_KING)  fen += 'K';
        if (castlingRights & CASTLE_WHITE_QUEEN) fen += 'Q';
        if (castlingRights & CASTLE_BLACK_KING)  fen += 'k';
        if (castlingRights & CASTLE_BLACK_QUEEN) fen += 'q';
    }
    fen += ' ';

    // ---- 4. En Passant Square ----
    if (enPassantSq == SQ_NONE) {
        fen += '-';
    } else {
        fen += static_cast<char>('a' + fileOf(enPassantSq));
        fen += static_cast<char>('1' + rankOf(enPassantSq));
    }

    // ---- 5. Half-move clock and full-move number ----
    fen += ' ' + std::to_string(halfMoveClock);
    fen += ' ' + std::to_string(fullMoveNumber);

    return fen;
}

// ============================================================
//  Debug display  —  prints the board to stdout
// ============================================================
void BoardState::display() const {
    static const char pieceChars[PIECE_COUNT] = {
        'P', 'N', 'B', 'R', 'Q', 'K'
    };

    std::cout << "\n  +---+---+---+---+---+---+---+---+\n";
    for (int rank = 7; rank >= 0; rank--) {
        std::cout << (rank + 1) << " |";
        for (int file = 0; file < 8; file++) {
            int sq = makeSquare(rank, file);
            char c = '.';

            int piece = pieceOnSquare(sq);
            int color = colorOnSquare(sq);
            if (piece >= 0) {
                c = pieceChars[piece];
                if (color == BLACK_SIDE) c = static_cast<char>(std::tolower(c));
            }

            std::cout << " " << c << " |";
        }
        std::cout << "\n  +---+---+---+---+---+---+---+---+\n";
    }
    std::cout << "    a   b   c   d   e   f   g   h\n\n";

    // State info
    std::cout << "Side to move: " << (sideToMove == WHITE_SIDE ? "White" : "Black") << "\n";
    std::cout << "Castling:     ";
    if (castlingRights == CASTLE_NONE) std::cout << "-";
    else {
        if (castlingRights & CASTLE_WHITE_KING)  std::cout << "K";
        if (castlingRights & CASTLE_WHITE_QUEEN) std::cout << "Q";
        if (castlingRights & CASTLE_BLACK_KING)  std::cout << "k";
        if (castlingRights & CASTLE_BLACK_QUEEN) std::cout << "q";
    }
    std::cout << "\n";

    std::cout << "En passant:   ";
    if (enPassantSq == SQ_NONE) std::cout << "-";
    else std::cout << static_cast<char>('a' + fileOf(enPassantSq))
                   << static_cast<char>('1' + rankOf(enPassantSq));
    std::cout << "\n";

    std::cout << "Half-move:    " << halfMoveClock << "\n";
    std::cout << "Full-move:    " << fullMoveNumber << "\n";

    // Piece counts
    std::cout << "\n--- Piece Counts ---\n";
    static const char* names[PIECE_COUNT] = {
        "Pawns", "Knights", "Bishops", "Rooks", "Queens", "Kings"
    };
    for (int p = 0; p < PIECE_COUNT; p++) {
        int w = popCount(pieces[WHITE_SIDE][p]);
        int b = popCount(pieces[BLACK_SIDE][p]);
        std::cout << "  " << names[p] << ":   W=" << w << "  B=" << b << "\n";
    }
    std::cout << std::endl;
}

// ============================================================
//  makeMove  —  execute a pseudo-legal move
//  Returns true if legal (King not in check after move)
// ============================================================
bool BoardState::makeMove(Move16 m, UndoState& undo) {
    // 1. Save state
    undo.castlingRights = castlingRights;
    undo.enPassantSq    = enPassantSq;
    undo.halfMoveClock  = halfMoveClock;
    undo.zobristKey     = zobristKey;
    
    int from  = moveFrom(m);
    int to    = moveTo(m);
    int flags = moveFlags(m);
    int side  = sideToMove;
    int opp   = side ^ 1;

    // Reject illegal castling attempts early (cannot castle out of or through check)
    if (isCastle(m)) {
        int transitSq = (flags == FLAG_KING_CASTLE) ? ((side == WHITE_SIDE) ? F1 : F8) : ((side == WHITE_SIDE) ? D1 : D8);
        if (isSquareAttacked(from, opp) || isSquareAttacked(transitSq, opp)) {
            return false;
        }
    }

    int movingPiece = pieceOnSquare(from);
    undo.capturedPiece = -1;

    // Bitboards XOR helpers
    uint64_t fromMask = sqBB(from);
    uint64_t toMask   = sqBB(to);
    uint64_t moveMask = fromMask | toMask;

    // 2. Handle captured piece
    if (isCapture(m)) {
        if (flags == FLAG_EP_CAPTURE) {
            undo.capturedPiece = PAWN_IDX;
            int epCapSq = (side == WHITE_SIDE) ? to - 8 : to + 8;
            uint64_t epMask = sqBB(epCapSq);
            pieces[opp][PAWN_IDX] ^= epMask;
            occupancy[opp] ^= epMask;
            zobristKey ^= Zobrist::pieceKeys[opp][PAWN_IDX][epCapSq];
        } else {
            undo.capturedPiece = pieceOnSquare(to);
            if (undo.capturedPiece != -1) {
                pieces[opp][undo.capturedPiece] ^= toMask;
                occupancy[opp] ^= toMask;
                zobristKey ^= Zobrist::pieceKeys[opp][undo.capturedPiece][to];
            }
        }
        halfMoveClock = 0; // Reset on capture
    } else if (movingPiece == PAWN_IDX) {
        halfMoveClock = 0; // Reset on pawn push
    } else {
        halfMoveClock++;
    }

    // 3. Move the piece
    pieces[side][movingPiece] ^= moveMask;
    occupancy[side] ^= moveMask;
    zobristKey ^= Zobrist::pieceKeys[side][movingPiece][from];
    zobristKey ^= Zobrist::pieceKeys[side][movingPiece][to];

    // 4. Promotions
    if (isPromotion(m)) {
        int promoType = promotionPiece(m);
        pieces[side][PAWN_IDX] ^= toMask; // Remove pawn from 'to'
        pieces[side][promoType] ^= toMask; // Add promoted piece to 'to'
        zobristKey ^= Zobrist::pieceKeys[side][PAWN_IDX][to];
        zobristKey ^= Zobrist::pieceKeys[side][promoType][to];
    }

    // 5. Castling 
    if (isCastle(m)) {
        int rookFrom, rookTo;
        if (flags == FLAG_KING_CASTLE) {
            rookFrom = (side == WHITE_SIDE) ? H1 : H8;
            rookTo   = (side == WHITE_SIDE) ? F1 : F8;
        } else {
            rookFrom = (side == WHITE_SIDE) ? A1 : A8;
            rookTo   = (side == WHITE_SIDE) ? D1 : D8;
        }
        uint64_t rookMask = sqBB(rookFrom) | sqBB(rookTo);
        pieces[side][ROOK_IDX] ^= rookMask;
        occupancy[side] ^= rookMask;
        zobristKey ^= Zobrist::pieceKeys[side][ROOK_IDX][rookFrom];
        zobristKey ^= Zobrist::pieceKeys[side][ROOK_IDX][rookTo];
    }

    // 6. En Passant Square updates
    if (enPassantSq != SQ_NONE) {
        zobristKey ^= Zobrist::enPassantKeys[fileOf(enPassantSq)];
        enPassantSq = SQ_NONE;
    }

    if (flags == FLAG_DOUBLE_PAWN) {
        enPassantSq = (side == WHITE_SIDE) ? from + 8 : from - 8;
        zobristKey ^= Zobrist::enPassantKeys[fileOf(enPassantSq)];
    }

    // 7. Castling Rights updates
    zobristKey ^= Zobrist::castlingKeys[castlingRights];
    castlingRights &= CASTLE_UPDATES[from];
    castlingRights &= CASTLE_UPDATES[to];
    zobristKey ^= Zobrist::castlingKeys[castlingRights];

    // 8. Turn management
    sideToMove = opp;
    zobristKey ^= Zobrist::sideToMoveKey;
    allOccupancy = occupancy[WHITE_SIDE] | occupancy[BLACK_SIDE];

    if (side == BLACK_SIDE) fullMoveNumber++;

    // 9. Legality check. Since we swapped sideToMove, check if 'side' king is in check.
    // The previous side (who just moved) shouldn't be in check
    int kingSq = lsb(pieces[side][KING_IDX]);
    
    // We temporally switch 'sideToMove' back to check accurately
    sideToMove ^= 1; 
    bool inCheck = isInCheck();
    sideToMove ^= 1;

    if (inCheck) {
        undoMove(m, undo);
        return false;
    }

    return true;
}

// ============================================================
//  undoMove  —  roll back exactly the inverse of makeMove
// ============================================================
void BoardState::undoMove(Move16 m, const UndoState& undo) {
    int opp   = sideToMove; // Who just moved?
    int side  = opp ^ 1;
    sideToMove = side;

    if (side == BLACK_SIDE) fullMoveNumber--;

    int from  = moveFrom(m);
    int to    = moveTo(m);
    int flags = moveFlags(m);

    // Retrieve moved piece type (from the 'to' square because it moved there)
    // Careful: if promotion, pieceOnSquare(to) returns the promoted piece (e.g. Queen)!
    int movingPiece = pieceOnSquare(to); 
    if (isPromotion(m)) {
        movingPiece = PAWN_IDX;
        int promoType = promotionPiece(m);
        // Remove promoted piece
        pieces[side][promoType] ^= sqBB(to);
        // Put pawn back on 'to', let the main swap handle it moving back to 'from'
        pieces[side][PAWN_IDX] ^= sqBB(to);
    }

    uint64_t moveMask = sqBB(from) | sqBB(to);
    pieces[side][movingPiece] ^= moveMask;
    occupancy[side] ^= moveMask;

    // Handle captures
    if (isCapture(m)) {
        if (flags == FLAG_EP_CAPTURE) {
            int epCapSq = (side == WHITE_SIDE) ? to - 8 : to + 8;
            pieces[opp][PAWN_IDX] ^= sqBB(epCapSq);
            occupancy[opp] ^= sqBB(epCapSq);
        } else if (undo.capturedPiece != -1) {
            pieces[opp][undo.capturedPiece] ^= sqBB(to);
            occupancy[opp] ^= sqBB(to);
        }
    }

    // Handle castling rook rollback
    if (isCastle(m)) {
        int rookFrom, rookTo;
        if (flags == FLAG_KING_CASTLE) {
            rookFrom = (side == WHITE_SIDE) ? H1 : H8;
             rookTo   = (side == WHITE_SIDE) ? F1 : F8;
        } else {
            rookFrom = (side == WHITE_SIDE) ? A1 : A8;
            rookTo   = (side == WHITE_SIDE) ? D1 : D8;
        }
        uint64_t rookMask = sqBB(rookFrom) | sqBB(rookTo);
        pieces[side][ROOK_IDX] ^= rookMask;
        occupancy[side] ^= rookMask;
    }

    // Restore state
    castlingRights = undo.castlingRights;
    enPassantSq    = undo.enPassantSq;
    halfMoveClock  = undo.halfMoveClock;
    zobristKey     = undo.zobristKey;
    allOccupancy   = occupancy[WHITE_SIDE] | occupancy[BLACK_SIDE];
}

// ============================================================
//  isSquareAttacked  —  Is the square attacked by attackerSide?
// ============================================================
bool BoardState::isSquareAttacked(int sq, int attackerSide) const {
    int defenderSide = attackerSide ^ 1;
    
    // Pawns
    if (Attacks::getPawnAttacks(defenderSide, sq) & pieces[attackerSide][PAWN_IDX]) return true;
    
    // Knights
    if (Attacks::getKnightAttacks(sq) & pieces[attackerSide][KNIGHT_IDX]) return true;

    // Kings
    if (Attacks::getKingAttacks(sq) & pieces[attackerSide][KING_IDX]) return true;

    // Bishops & Queens (Diagonal)
    uint64_t diagAttackers = pieces[attackerSide][BISHOP_IDX] | pieces[attackerSide][QUEEN_IDX];
    if (Magics::getBishopAttacks(sq, allOccupancy) & diagAttackers) return true;

    // Rooks & Queens (Orthogonal)
    uint64_t orthAttackers = pieces[attackerSide][ROOK_IDX] | pieces[attackerSide][QUEEN_IDX];
    if (Magics::getRookAttacks(sq, allOccupancy) & orthAttackers) return true;

    return false;
}

// ============================================================
//  isInCheck  —  Is the sideToMove's King currently attacked?
// ============================================================
bool BoardState::isInCheck() const {
    uint64_t kingBB = pieces[sideToMove][KING_IDX];
    if (!kingBB) return false; // For tests without kings
    return isSquareAttacked(lsb(kingBB), sideToMove ^ 1);
}

// ============================================================
//  Null Move Logic
// ============================================================
void BoardState::makeNullMove(UndoState& undo) {
    undo.castlingRights = castlingRights;
    undo.enPassantSq    = enPassantSq;
    undo.halfMoveClock  = halfMoveClock;
    undo.zobristKey     = zobristKey;
    undo.capturedPiece  = -1; // No piece captured in null move

    if (enPassantSq != SQ_NONE) {
        zobristKey ^= Zobrist::enPassantKeys[fileOf(enPassantSq)];
        enPassantSq = SQ_NONE;
    }

    sideToMove ^= 1;
    zobristKey ^= Zobrist::sideToMoveKey;
    
    // Some engines reset half move clock on null move to prevent false draws
    // others increment it. Let's reset it or increment? Wait.
    // The user simply states: "Hamle sayaçlarını (halfMoveClock) güncelleyecek." 
    // Tekrar tespiti halfMoveClock kadar geriye tarar; null move'un iki
    // yanindaki pozisyonlar gercek bir tekrar sayilmamali -> sifirla.
    // (undoNullMove eski degeri geri yukler.)
    halfMoveClock = 0;
}

void BoardState::undoNullMove(const UndoState& undo) {
    sideToMove ^= 1;
    enPassantSq    = undo.enPassantSq;
    halfMoveClock  = undo.halfMoveClock;
    zobristKey     = undo.zobristKey;
    castlingRights = undo.castlingRights;
}

// ============================================================
//  Material helpers for NMP (hasNonPawnMaterial)
// ============================================================
bool BoardState::hasNonPawnMaterial(int side) const {
    uint64_t nonPawnMaterial = occupancy[side] ^ pieces[side][PAWN_IDX] ^ pieces[side][KING_IDX];
    return nonPawnMaterial != 0ULL;
}

// ============================================================
//  Helper for NMP
// ============================================================
int BoardState::getKingSquare(int side) const {
    uint64_t kingBB = pieces[side][KING_IDX];
    if (!kingBB) return SQ_NONE;
    return lsb(kingBB);
}

// ============================================================
//  Attackers to a square under given occupancy
// ============================================================
uint64_t BoardState::getAttackersToSquare(int sq, uint64_t occ) const {
    uint64_t attackers = 0ULL;

    // Pawns
    attackers |= (Attacks::getPawnAttacks(BLACK_SIDE, sq) & pieces[WHITE_SIDE][PAWN_IDX]);
    attackers |= (Attacks::getPawnAttacks(WHITE_SIDE, sq) & pieces[BLACK_SIDE][PAWN_IDX]);

    // Knights
    uint64_t knightMask = Attacks::getKnightAttacks(sq);
    attackers |= knightMask & (pieces[WHITE_SIDE][KNIGHT_IDX] | pieces[BLACK_SIDE][KNIGHT_IDX]);

    // Kings
    uint64_t kingMask = Attacks::getKingAttacks(sq);
    attackers |= kingMask & (pieces[WHITE_SIDE][KING_IDX] | pieces[BLACK_SIDE][KING_IDX]);

    // Sliders (Bishops, Rooks, Queens) using dynamic occupancy
    uint64_t bishopsQueens = (pieces[WHITE_SIDE][BISHOP_IDX] | pieces[BLACK_SIDE][BISHOP_IDX] |
                              pieces[WHITE_SIDE][QUEEN_IDX]  | pieces[BLACK_SIDE][QUEEN_IDX]);
    if (bishopsQueens) {
        attackers |= Magics::getBishopAttacks(sq, occ) & bishopsQueens;
    }

    uint64_t rooksQueens = (pieces[WHITE_SIDE][ROOK_IDX]  | pieces[BLACK_SIDE][ROOK_IDX] |
                            pieces[WHITE_SIDE][QUEEN_IDX] | pieces[BLACK_SIDE][QUEEN_IDX]);
    if (rooksQueens) {
        attackers |= Magics::getRookAttacks(sq, occ) & rooksQueens;
    }

    return attackers;
}

// ============================================================
//  Static Exchange Evaluation (SEE) — Tam ve Doğru Simülasyon
// ============================================================
int BoardState::getSEE(Move16 move) const {
    int from = moveFrom(move);
    int to   = moveTo(move);

    static const int seeVals[6] = {100, 320, 330, 500, 900, 20000};

    int victim = pieceOnSquare(to);
    int initialGain = 0;

    if (victim >= 0 && victim < 6) {
        initialGain = seeVals[victim];
    } else if (isEnPassant(move)) {
        initialGain = seeVals[PAWN_IDX];
    } else if (isPromotion(move)) {
        initialGain = seeVals[QUEEN_IDX] - seeVals[PAWN_IDX];
    } else {
        return 0; // Taş alımı veya taktik değilse SEE 0
    }

    // İlk saldıran taş
    int movingPiece = pieceOnSquare(from);
    if (movingPiece < 0 || movingPiece >= 6) {
        return 0;
    }

    // Dinamik tahta işgali (occupancy)
    uint64_t occ = allOccupancy;
    clearBit(occ, from);

    // Swap-list (en fazla 32 değişim olabilir)
    int gain[32];
    int d = 0;
    gain[d] = initialGain;

    int currentPiece = movingPiece;
    int side = sideToMove ^ 1; // Sıradaki saldıran karşı taraf

    uint64_t attackers = getAttackersToSquare(to, occ);

    // Çapraz ve düz ışın taşları (x-ray için)
    uint64_t diagSliders = (pieces[WHITE_SIDE][BISHOP_IDX] | pieces[BLACK_SIDE][BISHOP_IDX] |
                            pieces[WHITE_SIDE][QUEEN_IDX]  | pieces[BLACK_SIDE][QUEEN_IDX]);
    uint64_t orthSliders = (pieces[WHITE_SIDE][ROOK_IDX]  | pieces[BLACK_SIDE][ROOK_IDX] |
                            pieces[WHITE_SIDE][QUEEN_IDX] | pieces[BLACK_SIDE][QUEEN_IDX]);

    while (true) {
        d++;
        gain[d] = seeVals[currentPiece] - gain[d - 1];

        // minimax optimizasyonu: eğer rakip önceki kazancı aşamıyorsa dur
        if (std::max(-gain[d - 1], gain[d]) < 0) {
            // Bir sonraki taraf taşını feda etmek zorunda değil
        }

        // Bu tarafın en zayıf saldıranını bul (LVA - Least Valuable Attacker)
        uint64_t myAttackers = attackers & occupancy[side];
        if (!myAttackers) break;

        int nextPiece = -1;
        int nextSq = -1;

        for (int pt = PAWN_IDX; pt <= KING_IDX; pt++) {
            uint64_t pieceSubset = myAttackers & pieces[side][pt];
            if (pieceSubset) {
                nextPiece = pt;
                nextSq = lsb(pieceSubset);
                break;
            }
        }

        if (nextPiece == -1) break;

        currentPiece = nextPiece;

        // Saldıran taşı işgalden düş
        clearBit(occ, nextSq);

        // X-ray arkasındaki yeni taşları ekle
        if (currentPiece == PAWN_IDX || currentPiece == BISHOP_IDX || currentPiece == QUEEN_IDX) {
            attackers |= Magics::getBishopAttacks(to, occ) & diagSliders;
        }
        if (currentPiece == ROOK_IDX || currentPiece == QUEEN_IDX) {
            attackers |= Magics::getRookAttacks(to, occ) & orthSliders;
        }

        // Zaten oynanmış kareleri temizle
        attackers &= occ;

        side ^= 1;
    }

    // Minimax geri çekme (Negamax propagation)
    while (--d > 0) {
        gain[d - 1] = -std::max(-gain[d - 1], gain[d]);
    }

    return gain[0];
}

