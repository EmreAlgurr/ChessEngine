#pragma once
#include <cstdint>

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
    TTEntry* table;
    uint64_t numEntries;
    uint16_t currentAge;

public:
    TranspositionTable(int sizeInMB) {
        // 1 MB = 1024 * 1024 byte. Bunu 16 byte'lık struct boyutuna bölüyoruz.
        numEntries = (sizeInMB * 1024 * 1024) / sizeof(TTEntry);
        table = new TTEntry[numEntries](); // Tüm tabloyu RAM'de sıfırlarla oluştur
        currentAge = 0;
    }

    ~TranspositionTable() {
        delete[] table; // Bellek sızıntısı olmasın
    }

    // Yeni nesil aramalarda eski verilerin üzerine yazmayı kolaylaştırmak için
    void newSearch() {
        currentAge++;
    }

    // TT'den veri okuma (Probe)
    bool probe(uint64_t key, int depth, int alpha, int beta, int ply, int& returnScore, uint16_t& ttMove);

    // TT'ye veri yazma (Store)
    void store(uint64_t key, int depth, int ply, int score, int evalType, uint16_t move);
    
    // Tabloyu tamamen temizleme
    void clear() {
        for (uint64_t i = 0; i < numEntries; ++i) {
            table[i] = TTEntry();
        }
    }
};
