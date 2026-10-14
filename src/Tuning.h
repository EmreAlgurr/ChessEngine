#pragma once

#include "BitBoard.h"
#include <string>
#include <vector>

struct PositionEntry {
    BoardState board;
    double result; // 1.0 (White won), 0.5 (Draw), 0.0 (Black won)
};

class Tuner {
public:
    static void run(const std::string& datasetPath, int epochs = 50, int numThreads = 4, int maxPositions = 0);

private:
    static std::vector<PositionEntry> loadDataset(const std::string& path, int maxPositions = 0);
    static double computeK(const std::vector<PositionEntry>& dataset, int numThreads);
    static double computeMSE(const std::vector<PositionEntry>& dataset, double K, int numThreads);
    static double sigmoid(double score, double K);
    static void printParameters();
};
