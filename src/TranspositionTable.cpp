#include "TranspositionTable.h"

// Tabloya Yazma
void TranspositionTable::store(uint64_t key, int depth, int ply, int score, int bound, uint16_t move) {
    uint64_t index = key % numEntries; // Veya numEntries power of 2 ise (key & (numEntries - 1))
    TTEntry* entry = &table[index];

    // Replacement Scheme (Değiştirme Stratejisi):
    // Her zaman yeni veriyi yaz (Always Replace) veya daha derin bir veriyse yaz.
    // Şimdilik en basit ve agresif olanı: Her zaman üzerine yaz!
    // Sadece eğer hamle yoksa eski hamleyi koru ki Move Ordering bozulmasın.
    if (move == 0 && entry->key == key) {
        move = entry->move;
    }

    int ttScore = score;
    if (score > 900000) ttScore += ply;
    else if (score < -900000) ttScore -= ply;

    entry->key = key;
    entry->score = ttScore;
    entry->bound = bound;
    entry->depth = depth;
    entry->move = move;
    entry->age = currentAge;
}

// Tablodan Okuma
bool TranspositionTable::probe(uint64_t key, int depth, int alpha, int beta, int ply, int& returnScore, uint16_t& ttMove) {
    uint64_t index = key % numEntries;
    TTEntry* entry = &table[index];

    // Hash eşleşmiyorsa bu karede aradığımız şey yok demektir.
    if (entry->key != key) {
        ttMove = 0; // Hamle yok
        return false;
    }

    // Eşleşme var! Move ordering için hamleyi kesinlikle döndür.
    ttMove = entry->move;

    // Eğer bu skoru daha düşük bir derinlikte bulduysak, puana GÜVENEMEYİZ. 
    // Sadece hamleyi alıp çıkarız.
    if (entry->depth < depth) {
        return false;
    }

    // Skoru Alfa-Beta sınırlarına göre doğrula
    returnScore = entry->score;
    
    if (returnScore > 900000) returnScore -= ply;
    else if (returnScore < -900000) returnScore += ply;
    
    if (entry->bound == TT_EXACT) {
        return true; // Gerçek skor, direkt kullan!
    } 
    if (entry->bound == TT_UPPER && returnScore <= alpha) {
        return true; // Puan zaten çok düşük, hesaplamaya değmez (Fail-Low)
    } 
    if (entry->bound == TT_LOWER && returnScore >= beta) {
        return true; // Puan çok iyi, rakip buraya girmez (Fail-High)
    }

    return false;
}
