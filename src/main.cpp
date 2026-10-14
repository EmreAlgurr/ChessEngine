#include "UCIEngine.h"
#include "TerminalGame.h"
#include "Magics.h"
#include "Zobrist.h"
#include "Tuning.h"
#include <iostream>
#include <string>
#include <sstream>

int main(int argc, char* argv[]) {
    // 1. Mandatory Engine Initialization
    Magics::initMagics();
    Zobrist::init();

    // 2. Command-line argument shortcuts
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "play" || arg == "p") {
            TerminalGame game;
            game.run();
            return 0;
        }
        if (arg == "test" || arg == "perft") {
            TerminalGame::runPerftSuite();
            return 0;
        }
        if (arg == "uci") {
            UCIEngine engine;
            engine.run();
            return 0;
        }
        if (arg == "tune") {
            std::string datasetPath = (argc > 2) ? argv[2] : "quiet-labeled.epd";
            int epochs = (argc > 3) ? std::stoi(argv[3]) : 50;
            int threads = (argc > 4) ? std::stoi(argv[4]) : 4;
            int maxPositions = (argc > 5) ? std::stoi(argv[5]) : 0;
            Tuner::run(datasetPath, epochs, threads, maxPositions);
            return 0;
        }
    }

    // 3. No arguments: wait for first line from stdin
    std::string firstLine;
    if (std::getline(std::cin, firstLine)) {
        while (!firstLine.empty() && (firstLine.back() == '\r' || firstLine.back() == '\n' || firstLine.back() == ' '))
            firstLine.pop_back();
        while (!firstLine.empty() && (firstLine.front() == ' '))
            firstLine.erase(firstLine.begin());

        if (firstLine == "uci") {
            UCIEngine engine;
            std::cout << "id name MyEngine_CPP 1.0" << std::endl;
            std::cout << "id author Antigravity" << std::endl;
            std::cout << "uciok" << std::endl;
            engine.run();
            return 0;
        }
        if (firstLine == "test" || firstLine == "perft") {
            TerminalGame::runPerftSuite();
            return 0;
        }
        if (firstLine == "play" || firstLine == "p") {
            TerminalGame game;
            game.run();
            return 0;
        }
        if (firstLine.rfind("tune", 0) == 0) {
            std::istringstream ss(firstLine);
            std::string cmd, path = "quiet-labeled.epd";
            int epochs = 50, threads = 4, maxPositions = 0;
            ss >> cmd >> path >> epochs >> threads >> maxPositions;
            Tuner::run(path, epochs, threads, maxPositions);
            return 0;
        }

        TerminalGame game;
        game.run();
    }

    return 0;
}
