#include "TranspositionTable.h"

// Tabloya Yazma
void TranspositionTable::store(uint64_t key, int depth, int ply, int score, int bound, uint16_t move) {
    uint64_t index = key % numEntries; // Veya numEntries power of 2 ise (key & (numEntries - 1))
    TTEntry* entry = &table[index];

    // Replacement Scheme (Derinlik Öncelikli + Yaş Temelli):
    // 1. Hücre boşsa veya eski bir aramadan kalmışsa (age != currentAge) -> yaz
    // 2. Aynı pozisyonsa ve yeni arama daha derinse (veya eşitse ya da EXACT ise) -> yaz
    // 3. Farklı pozisyon (çarpışma) ve yeni arama eski derinlikten büyük veya eşitse -> yaz
    bool replace = false;
    if (entry->key == 0 || entry->age != currentAge) {
        replace = true;
    } else if (entry->key == key) {
        if (depth >= entry->depth || (bound == TT_EXACT && entry->bound != TT_EXACT)) {
            replace = true;
        } else if (move != 0 && entry->move == 0) {
            entry->move = move;
        }
    } else {
        if (depth >= entry->depth) {
            replace = true;
        }
    }

    if (!replace) return;

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
bool TranspositionTable::probe(uint64_t key, int depth, int alpha, int beta, int ply, int& returnScore, uint16_t& ttMove, int* outDepth, int* outBound) {
    uint64_t index = key % numEntries;
    TTEntry* entry = &table[index];

    // Hash eşleşmiyorsa bu karede aradığımız şey yok demektir.
    if (entry->key != key) {
        ttMove = 0; // Hamle yok
        return false;
    }

    // Eşleşme var! Move ordering için hamleyi kesinlikle döndür.
    ttMove = entry->move;

    int score = entry->score;
    if (score > 900000) score -= ply;
    else if (score < -900000) score += ply;
    returnScore = score;

    if (outDepth) *outDepth = entry->depth;
    if (outBound) *outBound = entry->bound;

    // Eğer bu skoru daha düşük bir derinlikte bulduysak, puana GÜVENEMEYİZ. 
    // Sadece hamleyi alıp çıkarız.
    if (entry->depth < depth) {
        return false;
    }

    // Skoru Alfa-Beta sınırlarına göre doğrula
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
