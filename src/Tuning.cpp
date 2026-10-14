#include "Tuning.h"
#include "SimpleEngine.h"
#include "EvalParams.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <thread>
#include <chrono>
#include <iomanip>
#include <algorithm>

struct ParameterRef {
    std::string name;
    int* ptr1;
    int* ptr2; // Optional mirror pointer for horizontal symmetry (can be nullptr)
    int minVal;
    int maxVal;
};

static std::vector<ParameterRef> getTuningParameters() {
    std::vector<ParameterRef> list;

    // 1. Taş Temel Değerleri (Pawn=100 anchor olarak sabit tutulur)
    list.push_back({"knightVal", &evalParams.knightVal, nullptr, 250, 450});
    list.push_back({"bishopVal", &evalParams.bishopVal, nullptr, 250, 450});
    list.push_back({"rookVal",   &evalParams.rookVal,   nullptr, 400, 650});
    list.push_back({"queenVal",  &evalParams.queenVal,  nullptr, 800, 1100});

    // 2. Pozisyonel Bonuslar ve Cezalar
    list.push_back({"bishopPair",       &evalParams.bishopPair,       nullptr, 0, 80});
    list.push_back({"rookOpenFile",     &evalParams.rookOpenFile,     nullptr, 0, 60});
    list.push_back({"rookSemiOpenFile", &evalParams.rookSemiOpenFile, nullptr, 0, 40});
    list.push_back({"castleBonus",      &evalParams.castleBonus,      nullptr, 0, 60});

    list.push_back({"isolatedPawn",    &evalParams.isolatedPawn,    nullptr, 0, 35});
    list.push_back({"passedPawnRank4", &evalParams.passedPawnRank4, nullptr, 0, 35});
    list.push_back({"passedPawnRank5", &evalParams.passedPawnRank5, nullptr, 0, 50});
    list.push_back({"passedPawnRank6", &evalParams.passedPawnRank6, nullptr, 5, 80});
    list.push_back({"passedPawnRank7", &evalParams.passedPawnRank7, nullptr, 15, 140});
    list.push_back({"rookOn7th",       &evalParams.rookOn7th,       nullptr, 0, 50});
    list.push_back({"knightMobility",  &evalParams.knightMobility,  nullptr, 0, 10});
    list.push_back({"bishopMobility",  &evalParams.bishopMobility,  nullptr, 0, 10});

    // 3. Kare Değer Tabloları (Yatay simetri: sol kanat ile sağ kanat çifti birlikte güncellenir)
    auto addSymmetricTable = [&](const std::string& prefix, int* table, int minV, int maxV, int startRank = 0, int endRank = 7) {
        for (int rank = startRank; rank <= endRank; rank++) {
            for (int file = 0; file < 4; file++) {
                int sq = rank * 8 + file;
                int mirrorSq = rank * 8 + (7 - file);
                std::string name = prefix + "[" + std::to_string(sq) + "]";
                if (sq == mirrorSq) {
                    list.push_back({name, &table[sq], nullptr, minV, maxV});
                } else {
                    list.push_back({name, &table[sq], &table[mirrorSq], minV, maxV});
                }
            }
        }
    };

    addSymmetricTable("pawnTable",   evalParams.pawnTable,   -50, 80, 1, 6); // Rank 2..7
    addSymmetricTable("knightTable", evalParams.knightTable, -70, 50, 0, 7);
    addSymmetricTable("bishopTable", evalParams.bishopTable, -50, 50, 0, 7);
    addSymmetricTable("rookTable",   evalParams.rookTable,   -30, 40, 0, 7);
    addSymmetricTable("queenTable",  evalParams.queenTable,  -40, 30, 0, 7);
    addSymmetricTable("kingMgTable", evalParams.kingMgTable, -70, 50, 0, 7);
    addSymmetricTable("kingEgTable", evalParams.kingEgTable, -70, 70, 0, 7);

    return list;
}

double Tuner::sigmoid(double score, double K) {
    return 1.0 / (1.0 + std::pow(10.0, -K * score / 400.0));
}

double Tuner::computeMSE(const std::vector<PositionEntry>& dataset, double K, int numThreads) {
    const size_t total = dataset.size();
    if (total == 0) return 0.0;

    std::vector<double> threadErrors(numThreads, 0.0);
    std::vector<std::thread> workers;
    size_t chunkSize = (total + numThreads - 1) / numThreads;

    for (int t = 0; t < numThreads; t++) {
        size_t start = t * chunkSize;
        size_t end = std::min(start + chunkSize, total);
        if (start >= total) break;

        workers.emplace_back([&, t, start, end]() {
            double localError = 0.0;
            SimpleEngine engine;
            for (size_t i = start; i < end; i++) {
                int score = engine.evaluate(dataset[i].board, dataset[i].board.sideToMove);
                double predicted = sigmoid(score, K);
                double diff = dataset[i].result - predicted;
                localError += diff * diff;
            }
            threadErrors[t] = localError;
        });
    }

    for (auto& w : workers) {
        if (w.joinable()) w.join();
    }

    double totalError = 0.0;
    for (double err : threadErrors) totalError += err;
    return totalError / total;
}

double Tuner::computeK(const std::vector<PositionEntry>& dataset, int numThreads) {
    std::cout << "[Tuner] Optimal K sabiti araniyor..." << std::endl;
    double bestK = 1.0;
    double bestError = 1e9;

    for (double k = 0.6; k <= 2.2; k += 0.1) {
        double err = computeMSE(dataset, k, numThreads);
        if (err < bestError) {
            bestError = err;
            bestK = k;
        }
    }
    std::cout << "[Tuner] Optimal K = " << bestK << " (Baslangic MSE: " 
              << std::fixed << std::setprecision(6) << bestError << ")" << std::endl;
    return bestK;
}

std::vector<PositionEntry> Tuner::loadDataset(const std::string& path, int maxPositions) {
    std::vector<PositionEntry> dataset;
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[HATA] Veri seti dosyasi acilamadi: " << path << std::endl;
        return dataset;
    }

    std::cout << "[Tuner] Veri seti okunuyor: " << path << " ..." << std::endl;
    std::string line;
    size_t lineCount = 0;

    while (std::getline(file, line)) {
        if (maxPositions > 0 && dataset.size() >= static_cast<size_t>(maxPositions)) break;
        lineCount++;
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' '))
            line.pop_back();
        if (line.empty()) continue;

        double whiteScore = -1.0;
        // Sonuç formatları: "1/2", "1-0", "0-1" (önce beraberlik kontrol edilir)
        if (line.find("1/2") != std::string::npos || line.find("\"0.5\"") != std::string::npos || line.find("[0.5]") != std::string::npos) {
            whiteScore = 0.5;
        } else if (line.find("1-0") != std::string::npos || line.find("\"1.0\"") != std::string::npos || line.find("[1.0]") != std::string::npos) {
            whiteScore = 1.0;
        } else if (line.find("0-1") != std::string::npos || line.find("\"0.0\"") != std::string::npos || line.find("[0.0]") != std::string::npos) {
            whiteScore = 0.0;
        } else {
            // Satır sonu düz float kontrolü (örn. " ... | 1.0")
            size_t lastSpace = line.find_last_of(" \t|");
            if (lastSpace != std::string::npos) {
                std::string resStr = line.substr(lastSpace + 1);
                try {
                    whiteScore = std::stod(resStr);
                } catch (...) {}
            }
        }

        if (whiteScore < 0.0) continue;

        // FEN kısmını ayıkla (ilk 4 parça: pieces, side, castle, ep)
        std::istringstream ss(line);
        std::string fen, word;
        int tokensCount = 0;
        while (ss >> word && tokensCount < 4) {
            if (word == "fen") continue;
            if (!fen.empty()) fen += " ";
            fen += word;
            tokensCount++;
        }

        if (tokensCount >= 4) {
            BoardState board;
            board.setFromFEN(fen);
            // Sonuç, hamle sırasındaki oyuncunun (sideToMove) bakış açısına çevrilmelidir!
            double relativeResult = (board.sideToMove == WHITE_SIDE) ? whiteScore : (1.0 - whiteScore);
            dataset.push_back({board, relativeResult});
        }
    }

    std::cout << "[Tuner] " << dataset.size() << " adet pozisyon belleğe yüklendi." << std::endl;
    return dataset;
}

void Tuner::run(const std::string& datasetPath, int epochs, int numThreads, int maxPositions) {
    if (numThreads <= 0) {
        numThreads = std::thread::hardware_concurrency();
        if (numThreads <= 0) numThreads = 4;
    }

    auto dataset = loadDataset(datasetPath, maxPositions);
    if (dataset.empty()) {
        std::cout << "[Tuner] Veri seti bos veya okunamadi. Islemi sonlandirildi." << std::endl;
        return;
    }

    evalParams.initDefaults();
    auto params = getTuningParameters();

    double K = computeK(dataset, numThreads);
    double currentMSE = computeMSE(dataset, K, numThreads);

    std::cout << "[Tuner] Baslangic Hatasi (MSE): " << std::fixed << std::setprecision(6) << currentMSE << std::endl;
    std::cout << "[Tuner] Optimize edilecek parametre grubu: " << params.size() << std::endl;
    std::cout << "[Tuner] " << numThreads << " thread ile Local Search basliyor..." << std::endl;
    std::cout << "------------------------------------------------------------" << std::endl;

    for (int epoch = 1; epoch <= epochs; epoch++) {
        auto startTime = std::chrono::steady_clock::now();
        int improvedCount = 0;

        for (auto& p : params) {
            int originalVal = *(p.ptr1);

            // 1. Deneme: +1
            if (originalVal + 1 <= p.maxVal) {
                *(p.ptr1) = originalVal + 1;
                if (p.ptr2) *(p.ptr2) = originalVal + 1;

                double newMSE = computeMSE(dataset, K, numThreads);
                if (newMSE < currentMSE) {
                    currentMSE = newMSE;
                    improvedCount++;
                    continue; // +1 kazandırdı, bu parametrede kal
                }
            }

            // 2. Deneme: -1
            if (originalVal - 1 >= p.minVal) {
                *(p.ptr1) = originalVal - 1;
                if (p.ptr2) *(p.ptr2) = originalVal - 1;

                double newMSE = computeMSE(dataset, K, numThreads);
                if (newMSE < currentMSE) {
                    currentMSE = newMSE;
                    improvedCount++;
                    continue; // -1 kazandırdı
                }
            }

            // İkisi de iyileştirmediyse orijinal değere geri al
            *(p.ptr1) = originalVal;
            if (p.ptr2) *(p.ptr2) = originalVal;
        }

        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();

        std::cout << "Epoch " << std::setw(2) << epoch << "/" << epochs
                  << " | MSE: " << std::fixed << std::setprecision(6) << currentMSE
                  << " | Guncellenen: " << std::setw(3) << improvedCount
                  << " | Sure: " << (elapsedMs / 1000.0) << "s" << std::endl;

        if (improvedCount == 0) {
            std::cout << "[Tuner] Yerel minimuma (optimum) ulasildi. Tebrikler!" << std::endl;
            break;
        }
    }

    std::cout << "\n============================================\n";
    std::cout << "        YENI OPTIMIZE EDILMIS PARAMETRELER   \n";
    std::cout << "============================================\n";
    printParameters();
}

static void printTable64(const std::string& name, const int* table) {
    std::cout << "    static const int " << name << "[64] = {\n";
    for (int r = 0; r < 8; r++) {
        std::cout << "       ";
        for (int f = 0; f < 8; f++) {
            int sq = r * 8 + f;
            std::cout << std::setw(4) << table[sq] << (sq == 63 ? "" : ",");
        }
        std::cout << "\n";
    }
    std::cout << "    };\n\n";
}

void Tuner::printParameters() {
    std::cout << "    pawnVal   = " << evalParams.pawnVal   << ";\n";
    std::cout << "    knightVal = " << evalParams.knightVal << ";\n";
    std::cout << "    bishopVal = " << evalParams.bishopVal << ";\n";
    std::cout << "    rookVal   = " << evalParams.rookVal   << ";\n";
    std::cout << "    queenVal  = " << evalParams.queenVal  << ";\n\n";

    std::cout << "    bishopPair       = " << evalParams.bishopPair       << ";\n";
    std::cout << "    rookOpenFile     = " << evalParams.rookOpenFile     << ";\n";
    std::cout << "    rookSemiOpenFile = " << evalParams.rookSemiOpenFile << ";\n";
    std::cout << "    castleBonus      = " << evalParams.castleBonus      << ";\n\n";

    std::cout << "    isolatedPawn     = " << evalParams.isolatedPawn     << ";\n";
    std::cout << "    passedPawnRank4  = " << evalParams.passedPawnRank4  << ";\n";
    std::cout << "    passedPawnRank5  = " << evalParams.passedPawnRank5  << ";\n";
    std::cout << "    passedPawnRank6  = " << evalParams.passedPawnRank6  << ";\n";
    std::cout << "    passedPawnRank7  = " << evalParams.passedPawnRank7  << ";\n";
    std::cout << "    rookOn7th        = " << evalParams.rookOn7th        << ";\n";
    std::cout << "    knightMobility   = " << evalParams.knightMobility   << ";\n";
    std::cout << "    bishopMobility   = " << evalParams.bishopMobility   << ";\n\n";

    printTable64("DEFAULT_PAWN_TABLE",   evalParams.pawnTable);
    printTable64("DEFAULT_KNIGHT_TABLE", evalParams.knightTable);
    printTable64("DEFAULT_BISHOP_TABLE", evalParams.bishopTable);
    printTable64("DEFAULT_ROOK_TABLE",   evalParams.rookTable);
    printTable64("DEFAULT_QUEEN_TABLE",  evalParams.queenTable);
    printTable64("DEFAULT_KING_MG_TABLE",evalParams.kingMgTable);
    printTable64("DEFAULT_KING_EG_TABLE",evalParams.kingEgTable);
}
