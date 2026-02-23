#include "UCIEngine.h"
#include "TerminalGame.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    // Command-line argument shortcuts
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
    }

    // No arguments: wait for the first line from stdin
    // Tournament software (cutechess-cli, etc.) will send "uci" as the first command
    std::string firstLine;
    if (std::getline(std::cin, firstLine)) {
        // Trim
        while (!firstLine.empty() && (firstLine.back() == '\r' || firstLine.back() == '\n' || firstLine.back() == ' '))
            firstLine.pop_back();
        while (!firstLine.empty() && (firstLine.front() == ' '))
            firstLine.erase(firstLine.begin());

        if (firstLine == "uci") {
            // Enter UCI mode - respond with identification then start the loop
            UCIEngine engine;
            // handleUCI is called inside run() when it receives "uci", but
            // we already consumed the first "uci" line, so respond here:
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

        // Default: play interactively
        TerminalGame game;
        game.run();
    }

    return 0;
}
