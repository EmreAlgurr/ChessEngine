#include "UCIEngine.h"
#include "Tuning.h"
#include "Book.h"
#include "nnue.h"
#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {

inline bool parseIntView(std::string_view sv, int& out) {
    if (sv.empty()) return false;
    auto res = std::from_chars(sv.data(), sv.data() + sv.size(), out);
    return res.ec == std::errc{};
}

inline bool parseLongView(std::string_view sv, long& out) {
    if (sv.empty()) return false;
    auto res = std::from_chars(sv.data(), sv.data() + sv.size(), out);
    return res.ec == std::errc{};
}

inline bool equalsIgnoreCase(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

} // namespace

UCIEngine::UCIEngine() : debug(false), currentColor(WHITE_SIDE) {
    engine.setDepth(4);
    board.setFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    gameKeys.reserve(1024);
    lineBuffer.reserve(4096);
}

size_t UCIEngine::tokenizeZeroCopy(std::string_view line, std::string_view* outTokens, size_t maxTokens) {
    size_t count = 0;
    size_t i = 0;
    const size_t n = line.size();

    while (i < n && count < maxTokens) {
        while (i < n && (line[i] == ' ' || line[i] == '\t' || line[i] == '\r' || line[i] == '\n')) {
            ++i;
        }
        if (i >= n) break;
        const size_t start = i;
        while (i < n && line[i] != ' ' && line[i] != '\t' && line[i] != '\r' && line[i] != '\n') {
            ++i;
        }
        outTokens[count++] = line.substr(start, i - start);
    }
    return count;
}

void UCIEngine::run() {
    while (std::getline(std::cin, lineBuffer)) {
        size_t count = tokenizeZeroCopy(lineBuffer, tokenBuffer.data(), tokenBuffer.size());
        if (count == 0) continue;

        const std::string_view* tokens = tokenBuffer.data();
        const std::string_view command = tokens[0];

        if (command == "uci") {
            handleUCI();
        } else if (command == "debug") {
            if (count > 1) handleDebug(tokens[1]);
        } else if (command == "isready") {
            handleIsReady();
        } else if (command == "setoption") {
            handleSetOption(tokens, count);
        } else if (command == "ucinewgame") {
            handleUCINewGame();
        } else if (command == "position") {
            handlePosition(tokens, count);
        } else if (command == "go") {
            handleGo(tokens, count);
        } else if (command == "stop") {
            handleStop();
        } else if (command == "quit") {
            handleQuit();
            return;
        } else if (command == "tune") {
            std::string datasetPath = (count > 1) ? std::string(tokens[1]) : "dataset.epd";
            int epochs = 50, threads = 4, maxPositions = 0;
            if (count > 2) parseIntView(tokens[2], epochs);
            if (count > 3) parseIntView(tokens[3], threads);
            if (count > 4) parseIntView(tokens[4], maxPositions);
            Tuner::run(datasetPath, epochs, threads, maxPositions);
        } else if (command == "d" || command == "display" || command == "print") {
            board.display();
        } else if (command.length() >= 4 &&
                   std::islower(static_cast<unsigned char>(command[0])) &&
                   std::isdigit(static_cast<unsigned char>(command[1]))) {
            makeUCIMove(command);
            board.display();
            const std::string_view goTokens[3] = {"go", "depth", "4"};
            handleGo(goTokens, 3);
            board.display();
        } else {
            if (debug) {
                std::cout << "info string Unknown command: " << command << std::endl;
            }
        }
    }
}

void UCIEngine::handleUCI() {
    std::cout << "id name MyEngine_CPP 1.0\n"
              << "id author Antigravity\n"
              << "option name Hash type spin default 64 min 1 max 4096\n"
              << "option name OwnBook type check default true\n"
              << "option name BookFile type string default gm2001.bin\n"
              << "option name EvalFile type string default nn-04cf2b4ed1da.nnue\n"
              << "uciok" << std::endl;
}

void UCIEngine::handleDebug(std::string_view arg) {
    debug = (arg == "on");
}

void UCIEngine::handleIsReady() {
    std::cout << "readyok" << std::endl;
}

void UCIEngine::handleSetOption(const std::string_view* tokens, size_t count) {
    // Reconstruct multi-word option names/values inside contiguous lineBuffer slices when possible
    size_t nameStart = count, nameEnd = count;
    size_t valStart  = count, valEnd  = count;

    for (size_t i = 1; i < count; i++) {
        if (tokens[i] == "name") {
            nameStart = i + 1;
            nameEnd = nameStart;
        } else if (tokens[i] == "value") {
            if (nameStart < count && nameEnd == nameStart) nameEnd = i;
            valStart = i + 1;
            valEnd = count;
            break;
        } else if (nameStart < count && valStart == count) {
            nameEnd = i + 1;
        }
    }

    if (nameStart >= nameEnd) return;

    const char* nBegin = tokens[nameStart].data();
    const char* nEnd   = tokens[nameEnd - 1].data() + tokens[nameEnd - 1].size();
    std::string_view nameView(nBegin, static_cast<size_t>(nEnd - nBegin));

    std::string_view valView;
    if (valStart < valEnd) {
        const char* vBegin = tokens[valStart].data();
        const char* vEnd   = tokens[valEnd - 1].data() + tokens[valEnd - 1].size();
        valView = std::string_view(vBegin, static_cast<size_t>(vEnd - vBegin));
    }

    if (equalsIgnoreCase(nameView, "hash")) {
        int mb = 0;
        if (parseIntView(valView, mb)) {
            engine.setHashSize(mb);
        }
    } else if (equalsIgnoreCase(nameView, "ownbook")) {
        openingBook.setEnabled(equalsIgnoreCase(valView, "true") || valView == "1");
    } else if (equalsIgnoreCase(nameView, "bookfile")) {
        openingBook.open(std::string(valView));
    } else if (equalsIgnoreCase(nameView, "evalfile")) {
        char pathBuf[512];
        const size_t copyLen = std::min(valView.size(), sizeof(pathBuf) - 1);
        std::memcpy(pathBuf, valView.data(), copyLen);
        pathBuf[copyLen] = '\0';
        nnue_init(pathBuf);
    }
}

void UCIEngine::handleUCINewGame() {
    board.setFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    currentColor = WHITE_SIDE;
    gameKeys.clear();
    gameKeys.push_back(board.zobristKey);
}

void UCIEngine::handlePosition(const std::string_view* tokens, size_t count) {
    if (count < 2) return;

    if (tokens[1] == "startpos") {
        board.setFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        currentColor = WHITE_SIDE;
        gameKeys.clear();
        gameKeys.push_back(board.zobristKey);

        size_t movesIndex = count;
        for (size_t i = 2; i < count; i++) {
            if (tokens[i] == "moves") {
                movesIndex = i + 1;
                break;
            }
        }

        for (size_t i = movesIndex; i < count; i++) {
            makeUCIMove(tokens[i]);
        }
    } else if (tokens[1] == "fen") {
        if (count < 3) return;

        size_t fenEndToken = count;
        size_t movesIndex = count;
        for (size_t i = 2; i < count; i++) {
            if (tokens[i] == "moves") {
                fenEndToken = i;
                movesIndex = i + 1;
                break;
            }
        }

        if (fenEndToken > 2) {
            // Zero-copy slice directly over lineBuffer for the FEN fields
            const char* fenBegin = tokens[2].data();
            const char* fenEnd   = tokens[fenEndToken - 1].data() + tokens[fenEndToken - 1].size();
            std::string_view fenView(fenBegin, static_cast<size_t>(fenEnd - fenBegin));
            board.setFromFEN(fenView);
        }

        currentColor = board.sideToMove;
        gameKeys.clear();
        gameKeys.push_back(board.zobristKey);

        for (size_t i = movesIndex; i < count; i++) {
            makeUCIMove(tokens[i]);
        }
    }
}

void UCIEngine::makeUCIMove(std::string_view uciMove) {
    if (uciMove.length() < 4) return;

    MoveList legalMoves;
    MoveGen::generateAllMoves(board, legalMoves);

    for (Move16 move : legalMoves) {
        if (moveMatchesUCI(move, uciMove)) {
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

void UCIEngine::handleGo(const std::string_view* tokens, size_t count) {
    long wtime = 0, btime = 0, winc = 0, binc = 0;
    int depth = 0, movetime = 0;
    bool infinite = false;

    for (size_t i = 1; i < count; i++) {
        if (tokens[i] == "wtime" && i + 1 < count) {
            parseLongView(tokens[++i], wtime);
        } else if (tokens[i] == "btime" && i + 1 < count) {
            parseLongView(tokens[++i], btime);
        } else if (tokens[i] == "winc" && i + 1 < count) {
            parseLongView(tokens[++i], winc);
        } else if (tokens[i] == "binc" && i + 1 < count) {
            parseLongView(tokens[++i], binc);
        } else if (tokens[i] == "depth" && i + 1 < count) {
            parseIntView(tokens[++i], depth);
        } else if (tokens[i] == "movetime" && i + 1 < count) {
            parseIntView(tokens[++i], movetime);
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

    engine.setGameHistory(gameKeys.data(), gameKeys.size());
    Move16 bestMove = engine.getBestMove(board, currentColor, timeToThink);

    char bestBuf[6];
    size_t bestLen = moveToUCIBuf(bestMove, bestBuf);
    std::string_view uci(bestBuf, bestLen);

    if (uci.empty() || uci == "0000" || uci == "a0a0") {
        MoveList legalMoves;
        MoveGen::generateAllMoves(board, legalMoves);
        if (legalMoves.count > 0) {
            // Find first purely legal (not just pseudo) move
            for (Move16 m : legalMoves) {
                UndoState u;
                if (board.makeMove(m, u)) {
                    bestMove = m;
                    bestLen = moveToUCIBuf(m, bestBuf);
                    uci = std::string_view(bestBuf, bestLen);
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
