#pragma once
#include "MemoryPool.h"
#include <cstdint>
#include <cstring>

// TT Skor Tipleri (Bounds)
enum TTFlag : uint8_t {
    TT_EXACT = 0, // Tam skor (PV Node)
    TT_LOWER = 1, // Beta'yı aştı (Fail-High / Cut Node)
    TT_UPPER = 2  // Alpha'nın altında kaldı (Fail-Low / All Node)
};

// 16 Byte Tightly Packed Struct
struct TTEntry {
    uint64_t key;       // Zobrist Hash anahtarı (8 byte)
    int16_t score;      // Arama skoru (2 byte)
    uint16_t move;      // En iyi hamle (Senin 16-bit Move yapın) (2 byte)
    uint8_t depth;      // Bu skorun hesaplandığı derinlik (1 byte)
    uint8_t bound;      // EXACT, LOWER veya UPPER (1 byte)
    uint16_t age;       // Cache silinmelerini yönetmek için yaş/nesil (2 byte)
};

class TranspositionTable {
private:
    MemoryPool ttPool;
    TTEntry* table = nullptr;
    uint64_t numEntries = 0;
    uint16_t currentAge = 0;

public:
    explicit TranspositionTable(int sizeInMB) {
        resize(sizeInMB);
    }

    ~TranspositionTable() = default;

    void resize(int sizeInMB) {
        if (sizeInMB < 1) sizeInMB = 1;
        if (sizeInMB > 4096) sizeInMB = 4096;
        const size_t bytes = static_cast<size_t>(sizeInMB) * 1024ULL * 1024ULL;
        ttPool.init(bytes + 64, true);
        numEntries = bytes / sizeof(TTEntry);
        table = ttPool.allocateArray<TTEntry>(numEntries, 64, false);
        currentAge = 0;
    }

    // Yeni nesil aramalarda eski verilerin üzerine yazmayı kolaylaştırmak için
    void newSearch() {
        currentAge++;
    }

    // TT'den veri okuma (Probe)
    bool probe(uint64_t key, int depth, int alpha, int beta, int ply, int& returnScore, uint16_t& ttMove, int* outDepth = nullptr, int* outBound = nullptr);

    // TT'ye veri yazma (Store)
    void store(uint64_t key, int depth, int ply, int score, int evalType, uint16_t move);
    
    // Tabloyu tamamen temizleme
    void clear() {
        if (table && numEntries > 0) {
            std::memset(table, 0, numEntries * sizeof(TTEntry));
        }
    }
};
