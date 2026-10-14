#pragma once

#include <cstdint>
#include <string>

// ============================================================
//  16-Bit Packed Move Encoding
//
//  Layout:  FFFF TTTTTT FFFFFF
//           ^^^^ ^^^^^^ ^^^^^^
//           |    |      └── bits 0-5 : from square (0-63)
//           |    └── bits 6-11: to square   (0-63)
//           └── bits 12-15: move type flag  (0-15)
//
//  Total: 16 bits = 1 uint16_t
// ============================================================

typedef uint16_t Move16;

// Sentinel value: "no move"
constexpr Move16 MOVE_NONE = 0;

// ============================================================
//  Move Type Flags  (4 bits → 16 possible values)
//
//  Bit 14 = promotion flag
//  Bit 12 = capture flag  (for non-promo moves)
//
//  Code | Binary | Description
//  -----|--------|----------------------------
//    0  | 0000   | Quiet move
//    1  | 0001   | Double pawn push
//    2  | 0010   | King-side castling  (O-O)
//    3  | 0011   | Queen-side castling (O-O-O)
//    4  | 0100   | Capture
//    5  | 0101   | En passant capture
//    8  | 1000   | Knight promotion
//    9  | 1001   | Bishop promotion
//   10  | 1010   | Rook promotion
//   11  | 1011   | Queen promotion
//   12  | 1100   | Knight promotion + capture
//   13  | 1101   | Bishop promotion + capture
//   14  | 1110   | Rook promotion + capture
//   15  | 1111   | Queen promotion + capture
// ============================================================

enum MoveFlag : uint16_t {
    FLAG_QUIET          = 0,    // 0000
    FLAG_DOUBLE_PAWN    = 1,    // 0001
    FLAG_KING_CASTLE    = 2,    // 0010
    FLAG_QUEEN_CASTLE   = 3,    // 0011
    FLAG_CAPTURE        = 4,    // 0100
    FLAG_EP_CAPTURE     = 5,    // 0101
    // 6, 7 reserved
    FLAG_KNIGHT_PROMO   = 8,    // 1000
    FLAG_BISHOP_PROMO   = 9,    // 1001
    FLAG_ROOK_PROMO     = 10,   // 1010
    FLAG_QUEEN_PROMO    = 11,   // 1011
    FLAG_KNIGHT_PROMO_CAP = 12, // 1100
    FLAG_BISHOP_PROMO_CAP = 13, // 1101
    FLAG_ROOK_PROMO_CAP   = 14, // 1110
    FLAG_QUEEN_PROMO_CAP  = 15  // 1111
};

// ============================================================
//  Encoding  —  pack from, to, flags into a Move16
// ============================================================

inline constexpr Move16 encodeMove(int from, int to, int flags = FLAG_QUIET) {
    return static_cast<uint16_t>(
          (static_cast<uint16_t>(flags) << 12)
        | (static_cast<uint16_t>(to)    << 6)
        | (static_cast<uint16_t>(from))
    );
}

// ============================================================
//  Decoding  —  extract fields from a Move16
// ============================================================

inline constexpr int moveFrom(Move16 m)  { return  m        & 0x3F; }  // bits 0-5
inline constexpr int moveTo(Move16 m)    { return (m >>  6) & 0x3F; }  // bits 6-11
inline constexpr int moveFlags(Move16 m) { return (m >> 12) & 0x0F; }  // bits 12-15

// ============================================================
//  Query Helpers
// ============================================================

// Is this a capture? (normal capture, en passant, or promotion+capture)
inline constexpr bool isCapture(Move16 m) {
    int f = moveFlags(m);
    return f == FLAG_CAPTURE || f == FLAG_EP_CAPTURE || f >= FLAG_KNIGHT_PROMO_CAP;
}

// Is this any kind of promotion?
inline constexpr bool isPromotion(Move16 m) {
    return (moveFlags(m) & 0x8) != 0;   // bit 15 set → promotion
}

// Is this a castling move?
inline constexpr bool isCastle(Move16 m) {
    int f = moveFlags(m);
    return f == FLAG_KING_CASTLE || f == FLAG_QUEEN_CASTLE;
}

// Is this an en passant capture?
inline constexpr bool isEnPassant(Move16 m) {
    return moveFlags(m) == FLAG_EP_CAPTURE;
}

// Is this a double pawn push?
inline constexpr bool isDoublePawnPush(Move16 m) {
    return moveFlags(m) == FLAG_DOUBLE_PAWN;
}

// Is this a quiet (non-tactical) move?
inline constexpr bool isQuiet(Move16 m) {
    int f = moveFlags(m);
    return f == FLAG_QUIET || f == FLAG_DOUBLE_PAWN;
}

// Is this a tactical move? (capture or promotion — worth searching in QS)
inline constexpr bool isTactical(Move16 m) {
    return isCapture(m) || isPromotion(m);
}

// ============================================================
//  Promotion Piece
//  Returns promoted piece index (1=Knight, 2=Bishop, 3=Rook, 4=Queen)
//  compatible with PieceIndex in BitBoard.h
//  Returns -1 if not a promotion
// ============================================================
inline constexpr int promotionPiece(Move16 m) {
    int f = moveFlags(m);
    if (f >= FLAG_KNIGHT_PROMO_CAP) return (f & 0x3) + 1;  // 12→1, 13→2, 14→3, 15→4
    if (f >= FLAG_KNIGHT_PROMO)     return (f & 0x3) + 1;  //  8→1,  9→2, 10→3, 11→4
    return -1;
}

// ============================================================
//  UCI String Conversion
//  e.g. "e2e4", "e7e8q", "e1g1" (castling)
// ============================================================

inline std::string moveToUCI(Move16 m) {
    if (m == MOVE_NONE) return "0000";

    int from = moveFrom(m);
    int to   = moveTo(m);

    std::string uci;
    uci += static_cast<char>('a' + (from & 7));   // from file
    uci += static_cast<char>('1' + (from >> 3));   // from rank
    uci += static_cast<char>('a' + (to   & 7));   // to file
    uci += static_cast<char>('1' + (to   >> 3));   // to rank

    // Append promotion piece letter
    if (isPromotion(m)) {
        static const char promoChars[] = { 'n', 'b', 'r', 'q' };
        int idx = promotionPiece(m) - 1;  // 1→0, 2→1, 3→2, 4→3
        if (idx >= 0 && idx < 4) uci += promoChars[idx];
    }

    return uci;
}

// ============================================================
//  UCI String → Move16 Parsing
//  Parses "e2e4", "e7e8q", etc.
//  Needs board context for castling/EP/double-push detection,
//  so this is a minimal version that encodes from+to only.
//  Flag detection should be done by the move generator or caller.
// ============================================================

inline Move16 moveFromUCI(const std::string& uci, int flags = FLAG_QUIET) {
    if (uci.size() < 4) return MOVE_NONE;

    int fromFile = uci[0] - 'a';
    int fromRank = uci[1] - '1';
    int toFile   = uci[2] - 'a';
    int toRank   = uci[3] - '1';

    if (fromFile < 0 || fromFile > 7 || fromRank < 0 || fromRank > 7) return MOVE_NONE;
    if (toFile   < 0 || toFile   > 7 || toRank   < 0 || toRank   > 7) return MOVE_NONE;

    int from = (fromRank << 3) | fromFile;
    int to   = (toRank   << 3) | toFile;

    // Auto-detect promotion from 5th character
    if (uci.size() >= 5) {
        switch (uci[4]) {
            case 'n': case 'N': flags = (flags == FLAG_CAPTURE || flags >= FLAG_KNIGHT_PROMO_CAP)
                                         ? FLAG_KNIGHT_PROMO_CAP : FLAG_KNIGHT_PROMO; break;
            case 'b': case 'B': flags = (flags == FLAG_CAPTURE || flags >= FLAG_KNIGHT_PROMO_CAP)
                                         ? FLAG_BISHOP_PROMO_CAP : FLAG_BISHOP_PROMO; break;
            case 'r': case 'R': flags = (flags == FLAG_CAPTURE || flags >= FLAG_KNIGHT_PROMO_CAP)
                                         ? FLAG_ROOK_PROMO_CAP   : FLAG_ROOK_PROMO;   break;
            case 'q': case 'Q': flags = (flags == FLAG_CAPTURE || flags >= FLAG_KNIGHT_PROMO_CAP)
                                         ? FLAG_QUEEN_PROMO_CAP  : FLAG_QUEEN_PROMO;  break;
            default: break;
        }
    }

    return encodeMove(from, to, flags);
}

// ============================================================
//  Debug: human-readable move description
// ============================================================
inline std::string moveToString(Move16 m) {
    if (m == MOVE_NONE) return "NONE";

    std::string s = moveToUCI(m);

    int f = moveFlags(m);
    switch (f) {
        case FLAG_QUIET:             s += " (quiet)";          break;
        case FLAG_DOUBLE_PAWN:       s += " (double pawn)";    break;
        case FLAG_KING_CASTLE:       s += " (O-O)";            break;
        case FLAG_QUEEN_CASTLE:      s += " (O-O-O)";          break;
        case FLAG_CAPTURE:           s += " (capture)";        break;
        case FLAG_EP_CAPTURE:        s += " (en passant)";     break;
        case FLAG_KNIGHT_PROMO:      s += " (=N)";             break;
        case FLAG_BISHOP_PROMO:      s += " (=B)";             break;
        case FLAG_ROOK_PROMO:        s += " (=R)";             break;
        case FLAG_QUEEN_PROMO:       s += " (=Q)";             break;
        case FLAG_KNIGHT_PROMO_CAP:  s += " (x=N)";            break;
        case FLAG_BISHOP_PROMO_CAP:  s += " (x=B)";            break;
        case FLAG_ROOK_PROMO_CAP:    s += " (x=R)";            break;
        case FLAG_QUEEN_PROMO_CAP:   s += " (x=Q)";            break;
        default:                     s += " (?)";              break;
    }
    return s;
}
