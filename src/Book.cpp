#include "Book.h"
#include "PolyglotKeys.h"
#include "MoveGenerator.h"
#include "MoveEncoding.h"
#include <fstream>
#include <algorithm>
#include <cstdlib>
#include <iostream>

#if defined(_MSC_VER)
    #include <stdlib.h>
    #define BSWAP64(x) _byteswap_uint64(x)
    #define BSWAP16(x) _byteswap_ushort(x)
    #define BSWAP32(x) _byteswap_ulong(x)
#else
    #define BSWAP64(x) __builtin_bswap64(x)
    #define BSWAP16(x) __builtin_bswap16(x)
    #define BSWAP32(x) __builtin_bswap32(x)
#endif

PolyglotBook openingBook;

PolyglotBook::PolyglotBook() : bookEnabled(true) {
    // Varsayılan olarak aynı dizinde gm2001.bin veya book.bin varsa yüklemeyi dene
    if (!open("gm2001.bin")) {
        open("book.bin");
    }
}

PolyglotBook::~PolyglotBook() {
    close();
}

bool PolyglotBook::open(const std::string& filename) {
    close();
    bookFd = open_file(filename.c_str());
    if (bookFd == FD_ERR) {
        return false;
    }

    size_t bytes = file_size(bookFd);
    if (bytes < sizeof(BookEntry)) {
        close();
        return false;
    }

    const void* view = map_file(bookFd, &bookMapping);
    if (!view) {
        close();
        return false;
    }

    mappedEntries = static_cast<const BookEntry*>(view);
    entryCount = bytes / sizeof(BookEntry);
    currentFile = filename;
    bookEnabled = true;
    return entryCount > 0;
}

void PolyglotBook::close() {
    if (mappedEntries) {
        unmap_file(mappedEntries, bookMapping);
        mappedEntries = nullptr;
    }
    bookMapping = map_t{};
    if (bookFd != FD_ERR) {
        close_file(bookFd);
        bookFd = FD_ERR;
    }
    entryCount = 0;
}

uint64_t PolyglotBook::computePolyglotKey(const BoardState& board) {
    uint64_t key = 0ULL;

    // 1. Taşlar (Polyglot indeksleme)
    // Siyah: p=0, n=2, b=4, r=6, q=8, k=10
    // Beyaz: P=1, N=3, B=5, R=7, Q=9, K=11
    static const int PIECE_MAP[2][6] = {
        { 1, 3, 5, 7, 9, 11 }, // WHITE_SIDE (0)
        { 0, 2, 4, 6, 8, 10 }  // BLACK_SIDE (1)
    };

    for (int side = WHITE_SIDE; side <= BLACK_SIDE; side++) {
        for (int pt = PAWN_IDX; pt <= KING_IDX; pt++) {
            uint64_t bb = board.pieces[side][pt];
            int pieceIdx = PIECE_MAP[side][pt];
            while (bb) {
                int sq = popLsb(bb);
                key ^= POLYGLOT_RANDOM[64 * pieceIdx + sq];
            }
        }
    }

    // 2. Rok Hakları
    // Polyglot sırası: Beyaz Şah (768), Beyaz Vezir (769), Siyah Şah (770), Siyah Vezir (771)
    if (board.castlingRights & CASTLE_WHITE_KING)  key ^= POLYGLOT_RANDOM[768];
    if (board.castlingRights & CASTLE_WHITE_QUEEN) key ^= POLYGLOT_RANDOM[769];
    if (board.castlingRights & CASTLE_BLACK_KING)  key ^= POLYGLOT_RANDOM[770];
    if (board.castlingRights & CASTLE_BLACK_QUEEN) key ^= POLYGLOT_RANDOM[771];

    // 3. Geçerken Alma (En Passant)
    // Yalnızca sırası gelen oyuncunun piyonu bu kareyi alabiliyorsa hash'e dahil edilir
    if (board.enPassantSq != SQ_NONE) {
        int epFile = board.enPassantSq & 7;
        bool canCapture = false;

        if (board.sideToMove == WHITE_SIDE) {
            // En passant hedef karesi 6. yataydadır (rank == 5).
            // Beyaz piyonlar 5. yatayda olmalıdır (rank == 4).
            uint64_t wPawns = board.pieces[WHITE_SIDE][PAWN_IDX];
            if (epFile > 0 && (wPawns & (1ULL << makeSquare(4, epFile - 1)))) canCapture = true;
            if (epFile < 7 && (wPawns & (1ULL << makeSquare(4, epFile + 1)))) canCapture = true;
        } else {
            // En passant hedef karesi 3. yataydadır (rank == 2).
            // Siyah piyonlar 4. yatayda olmalıdır (rank == 3).
            uint64_t bPawns = board.pieces[BLACK_SIDE][PAWN_IDX];
            if (epFile > 0 && (bPawns & (1ULL << makeSquare(3, epFile - 1)))) canCapture = true;
            if (epFile < 7 && (bPawns & (1ULL << makeSquare(3, epFile + 1)))) canCapture = true;
        }

        if (canCapture) {
            key ^= POLYGLOT_RANDOM[772 + epFile];
        }
    }

    // 4. Hamle Sırası
    // Polyglot yalnızca sıra Beyaz'dayken 780 indeksini XOR'lar
    if (board.sideToMove == WHITE_SIDE) {
        key ^= POLYGLOT_RANDOM[780];
    }

    return key;
}

Move16 PolyglotBook::convertMove(uint16_t rawMove, BoardState& board) {
    int to_file = rawMove & 0x7;
    int to_rank = (rawMove >> 3) & 0x7;
    int from_file = (rawMove >> 6) & 0x7;
    int from_rank = (rawMove >> 9) & 0x7;
    int promo = (rawMove >> 12) & 0x7; // 1: At, 2: Fil, 3: Kale, 4: Vezir

    int from_sq = from_rank * 8 + from_file;
    int to_sq = to_rank * 8 + to_file;

    // Yasal hamle listesi ile eşleştir
    MoveList legalMoves;
    MoveGen::generateAllMoves(board, legalMoves);

    for (Move16 m : legalMoves) {
        if (moveFrom(m) != from_sq) continue;

        // Rok kontrolü (Polyglot şahın kalenin karesine gitmesi olarak kodlar)
        if (isCastle(m)) {
            if (from_sq == E1 && to_sq == H1 && moveTo(m) == G1) return m;
            if (from_sq == E1 && to_sq == A1 && moveTo(m) == C1) return m;
            if (from_sq == E8 && to_sq == H8 && moveTo(m) == G8) return m;
            if (from_sq == E8 && to_sq == A8 && moveTo(m) == C8) return m;
        }

        if (moveTo(m) != to_sq) continue;

        if (isPromotion(m)) {
            if (promotionPiece(m) == promo) return m;
        } else {
            if (promo == 0) return m;
        }
    }

    return MOVE_NONE;
}

Move16 PolyglotBook::getBookMove(BoardState& board) {
    if (!isEnabled()) return MOVE_NONE;

    uint64_t polyKey = computePolyglotKey(board);

    // Zero-copy ikili arama (dogrudan mmap/MapViewOfFile adresi uzerinden)
    const BookEntry* begin = mappedEntries;
    const BookEntry* end   = mappedEntries + entryCount;

    auto comp = [](const BookEntry& a, uint64_t k) {
        return BSWAP64(a.key) < k;
    };

    const BookEntry* it = std::lower_bound(begin, end, polyKey, comp);
    if (it == end || BSWAP64(it->key) != polyKey) {
        return MOVE_NONE; // Pozisyon kitapta yok
    }

    // Aynı pozisyon için tüm hamle varyantlarını stack tamponuna topla (sifir heap tahsisi)
    BookEntry matches[64];
    int matchCount = 0;
    while (it != end && BSWAP64(it->key) == polyKey && matchCount < 64) {
        matches[matchCount].key    = polyKey;
        matches[matchCount].move   = BSWAP16(it->move);
        matches[matchCount].weight = BSWAP16(it->weight);
        matches[matchCount].learn  = BSWAP32(it->learn);
        matchCount++;
        ++it;
    }

    if (matchCount == 0) return MOVE_NONE;

    // Ağırlıklı rastgele seçim (büyükustaların en sık oynadığı hamleler en yüksek şansa sahip olur)
    int totalWeight = 0;
    for (int i = 0; i < matchCount; ++i) totalWeight += matches[i].weight;

    BookEntry chosen;
    if (totalWeight <= 0) {
        chosen = matches[0];
    } else {
        int r = rand() % totalWeight;
        int acc = 0;
        chosen = matches[matchCount - 1];
        for (int i = 0; i < matchCount; ++i) {
            acc += matches[i].weight;
            if (r < acc) {
                chosen = matches[i];
                break;
            }
        }
    }

    return convertMove(chosen.move, board);
}
