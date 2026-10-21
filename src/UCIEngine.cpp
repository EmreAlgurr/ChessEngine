#include "UCIEngine.h"
#include "Tuning.h"
#include "Book.h"
#include "nnue.h"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cctype>

UCIEngine::UCIEngine() : debug(false), currentColor(WHITE_SIDE) {
    engine.setDepth(4);
    board.setFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

std::vector<std::string> UCIEngine::split(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream ss(line);
    std::string token;
    while (ss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

void UCIEngine::run() {
    std::string line;
    while (std::getline(std::cin, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' '))
            line.pop_back();
        while (!line.empty() && (line.front() == ' '))
            line.erase(line.begin());

        if (line.empty()) continue;

        auto tokens = split(line);
        if (tokens.empty()) continue;

        const std::string& command = tokens[0];

        if (command == "uci") {
            handleUCI();
        } else if (command == "debug") {
            if (tokens.size() > 1) handleDebug(tokens[1]);
        } else if (command == "isready") {
            handleIsReady();
        } else if (command == "setoption") {
            handleSetOption(tokens);
        } else if (command == "ucinewgame") {
            handleUCINewGame();
        } else if (command == "position") {
            handlePosition(tokens);
        } else if (command == "go") {
            handleGo(tokens);
        } else if (command == "stop") {
            handleStop();
        } else if (command == "quit") {
            handleQuit();
            return;
        } else if (command == "tune") {
            std::string datasetPath = (tokens.size() > 1) ? tokens[1] : "dataset.epd";
            int epochs = (tokens.size() > 2) ? std::stoi(tokens[2]) : 50;
            int threads = (tokens.size() > 3) ? std::stoi(tokens[3]) : 4;
            int maxPositions = (tokens.size() > 4) ? std::stoi(tokens[4]) : 0;
            Tuner::run(datasetPath, epochs, threads, maxPositions);
        } else if (command == "d" || command == "display" || command == "print") {
            board.display();
        } else if (command.length() >= 4 && std::islower(command[0]) && std::isdigit(command[1])) {
            makeUCIMove(command);
            board.display();
            std::vector<std::string> goTokens = {"go", "depth", "4"};
            handleGo(goTokens);
            board.display();
        } else {
            if (debug) {
                std::cout << "info string Unknown command: " << command << std::endl;
            }
        }
    }
}

void UCIEngine::handleUCI() {
    std::cout << "id name MyEngine_CPP 1.0" << std::endl;
    std::cout << "id author Antigravity" << std::endl;
    std::cout << "option name Hash type spin default 64 min 1 max 4096" << std::endl;
    std::cout << "option name OwnBook type check default true" << std::endl;
    std::cout << "option name BookFile type string default gm2001.bin" << std::endl;
    std::cout << "option name EvalFile type string default nn-04cf2b4ed1da.nnue" << std::endl;
    std::cout << "uciok" << std::endl;
}

void UCIEngine::handleDebug(const std::string& arg) {
    debug = (arg == "on");
}

void UCIEngine::handleIsReady() {
    std::cout << "readyok" << std::endl;
}

void UCIEngine::handleSetOption(const std::vector<std::string>& tokens) {
    std::string name, val;
    bool readingName = false, readingVal = false;
    for (size_t i = 1; i < tokens.size(); i++) {
        if (tokens[i] == "name") {
            readingName = true; readingVal = false;
        } else if (tokens[i] == "value") {
            readingName = false; readingVal = true;
        } else {
            if (readingName) {
                if (!name.empty()) name += " ";
                name += tokens[i];
            } else if (readingVal) {
                if (!val.empty()) val += " ";
                val += tokens[i];
            }
        }
    }

    auto toLower = [](std::string s) {
        for (char& c : s) c = std::tolower(static_cast<unsigned char>(c));
        return s;
    };

    std::string lowerName = toLower(name);
    if (lowerName == "hash") {
        try {
            int mb = std::stoi(val);
            engine.setHashSize(mb);
        } catch (...) {}
    } else if (lowerName == "ownbook") {
        std::string lowerVal = toLower(val);
        openingBook.setEnabled(lowerVal == "true" || lowerVal == "1");
    } else if (lowerName == "bookfile") {
        openingBook.open(val);
    } else if (lowerName == "evalfile") {
        nnue_init(val.c_str());
    }
}

void UCIEngine::handleUCINewGame() {
    board.setFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    currentColor = WHITE_SIDE;
    gameKeys.clear();
    gameKeys.push_back(board.zobristKey);
}

void UCIEngine::handlePosition(const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) return;

    if (tokens[1] == "startpos") {
        board.setFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        currentColor = WHITE_SIDE;
        gameKeys.clear();
        gameKeys.push_back(board.zobristKey);

        int movesIndex = -1;
        for (size_t i = 2; i < tokens.size(); i++) {
            if (tokens[i] == "moves") {
                movesIndex = static_cast<int>(i) + 1;
                break;
            }
        }

        if (movesIndex != -1) {
            for (size_t i = movesIndex; i < tokens.size(); i++) {
                makeUCIMove(tokens[i]);
            }
        }
    } else if (tokens[1] == "fen") {
        std::string fen;
        int movesIndex = -1;

        for (size_t i = 2; i < tokens.size(); i++) {
            if (tokens[i] == "moves") {
                movesIndex = static_cast<int>(i) + 1;
                break;
            }
            if (!fen.empty()) fen += " ";
            fen += tokens[i];
        }

        board.setFromFEN(fen);
        gameKeys.clear();
        gameKeys.push_back(board.zobristKey);

        std::istringstream fenSS(fen);
        std::string dummy, colorStr;
        fenSS >> dummy >> colorStr;
        currentColor = (colorStr == "w") ? WHITE_SIDE : BLACK_SIDE;

        if (movesIndex != -1) {
            for (size_t i = movesIndex; i < tokens.size(); i++) {
                makeUCIMove(tokens[i]);
            }
        }
    }
}

void UCIEngine::makeUCIMove(const std::string& uciMove) {
    if (uciMove.length() < 4) return;

    MoveList legalMoves;
    MoveGen::generateAllMoves(board, legalMoves);

    for (Move16 move : legalMoves) {
        if (moveToUCI(move) == uciMove) {
            UndoState undo;
            if (board.makeMove(move, undo)) {
                currentColor = (currentColor == WHITE_SIDE) ? BLACK_SIDE : WHITE_SIDE;
                gameKeys.push_back(board.zobristKey);
                return;
            }
        }
    }

    if (debug) {
        std::cout << "info string Illegal move/not found: " << uciMove << std::endl;
    }
}

int UCIEngine::getCurrentPlayerFromBoard() {
    return currentColor;
}

void UCIEngine::handleGo(const std::vector<std::string>& tokens) {
    long wtime = 0, btime = 0, winc = 0, binc = 0;
    int depth = 0, movetime = 0;
    bool infinite = false;

    for (size_t i = 1; i < tokens.size(); i++) {
        if (tokens[i] == "wtime" && i + 1 < tokens.size()) {
            wtime = std::stol(tokens[++i]);
        } else if (tokens[i] == "btime" && i + 1 < tokens.size()) {
            btime = std::stol(tokens[++i]);
        } else if (tokens[i] == "winc" && i + 1 < tokens.size()) {
            winc = std::stol(tokens[++i]);
        } else if (tokens[i] == "binc" && i + 1 < tokens.size()) {
            binc = std::stol(tokens[++i]);
        } else if (tokens[i] == "depth" && i + 1 < tokens.size()) {
            depth = std::stoi(tokens[++i]);
        } else if (tokens[i] == "movetime" && i + 1 < tokens.size()) {
            movetime = std::stoi(tokens[++i]);
        } else if (tokens[i] == "infinite") {
            infinite = true;
        }
    }

    long timeToThink = 0;

    if (movetime > 0) {
        timeToThink = movetime - 15;
        if (timeToThink < 1) timeToThink = 1;
    } else {
        long time = (currentColor == WHITE_SIDE) ? wtime : btime;
        long inc = (currentColor == WHITE_SIDE) ? winc : binc;

        if (time > 0) {
            long moveOverhead = 15; // fastchess ve lokal GUI için 15ms pay yeterli
            long usableTime = time - moveOverhead;
            if (usableTime < 0) usableTime = 0;

            // Formül: Kalan süreyi 25 hamleye yay + arttırmanın (increment) %75'ini ekle
            timeToThink = (usableTime / 25) + (inc * 3 / 4);

            // Güvenlik tavanı: Tek hamlede kalan sürenin %20'sinden fazlasını ASLA harcama
            long maxAlloc = static_cast<long>(usableTime * 0.20);
            if (timeToThink > maxAlloc) {
                timeToThink = maxAlloc;
            }

            // Güvenlik tabanı: usableTime > 30 iken en az 10ms düşün
            if (timeToThink < 10 && usableTime > 30) {
                timeToThink = std::min(10L, usableTime / 2);
            }
            if (timeToThink < 1) timeToThink = 1;
        }
    }

    if (depth > 0) engine.setDepth(depth);
    else if (infinite) { engine.setDepth(20); timeToThink = 0; }
    else if (timeToThink > 0) engine.setDepth(20);
    else engine.setDepth(5);

    engine.setGameHistory(gameKeys);
    Move16 bestMove = engine.getBestMove(board, currentColor, timeToThink);
    std::string uci = moveToUCI(bestMove);

    if (uci.empty() || uci == "0000" || uci == "a0a0") {
        MoveList legalMoves;
        MoveGen::generateAllMoves(board, legalMoves);
        if (legalMoves.count > 0) {
            // Find first purely legal (not just pseudo) move
            for (Move16 m : legalMoves) {
                UndoState u;
                if (board.makeMove(m, u)) {
                    bestMove = m;
                    uci = moveToUCI(m);
                    std::cerr << "info string WARNING: search returned no move, using fallback: " << uci << std::endl;
                    board.undoMove(m, u);
                    break;
                }
            }
        }
        
        if (uci.empty() || uci == "0000" || uci == "a0a0") {
            std::cout << "bestmove 0000" << std::endl;
            return;
        }
    }

    std::cout << "bestmove " << uci << std::endl;
}

void UCIEngine::handleStop() {}

void UCIEngine::handleQuit() {
    std::exit(0);
}
