#include "UCIEngine.h"
#include <iostream>
#include <sstream>
#include <cstdlib>

UCIEngine::UCIEngine() : debug(false), currentColor(Color::WHITE) {
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
        // Trim
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
        } else if (command == "d" || command == "display" || command == "print") {
            board.display();
        } else if (command.length() >= 4 && std::islower(command[0]) && std::isdigit(command[1])) {
            // Basit hamle girişi desteği (örn: e2e4)
            makeUCIMove(command);
            board.display();
            // Motorun cevap vermesini tetikle (go depth 4 gibi)
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
    std::cout << "uciok" << std::endl;
}

void UCIEngine::handleDebug(const std::string& arg) {
    debug = (arg == "on");
}

void UCIEngine::handleIsReady() {
    std::cout << "readyok" << std::endl;
}

void UCIEngine::handleSetOption(const std::vector<std::string>& tokens) {
    // Handle engine options here if needed
}

void UCIEngine::handleUCINewGame() {
    board.setFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    currentColor = Color::WHITE;
}

void UCIEngine::handlePosition(const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) return;

    if (tokens[1] == "startpos") {
        board.setFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        currentColor = Color::WHITE;

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

        // Parse current color from FEN
        std::istringstream fenSS(fen);
        std::string dummy, colorStr;
        fenSS >> dummy >> colorStr;
        currentColor = (colorStr == "w") ? Color::WHITE : Color::BLACK;

        if (movesIndex != -1) {
            for (size_t i = movesIndex; i < tokens.size(); i++) {
                makeUCIMove(tokens[i]);
            }
        }
    }
}

void UCIEngine::makeUCIMove(const std::string& uciMove) {
    if (uciMove.length() < 4) return;

    std::string fromSquare = uciMove.substr(0, 2);
    std::string toSquare = uciMove.substr(2, 2);

    Position from(fromSquare);
    Position to(toSquare);

    PieceType promotion = PieceType::NONE;
    if (uciMove.length() == 5) {
        char promChar = uciMove[4];
        switch (promChar) {
            case 'q': promotion = PieceType::QUEEN; break;
            case 'r': promotion = PieceType::ROOK; break;
            case 'b': promotion = PieceType::BISHOP; break;
            case 'n': promotion = PieceType::KNIGHT; break;
            default: break;
        }
    }

    std::vector<Move> legalMoves = board.getAllLegalMoves(currentColor);

    for (Move& move : legalMoves) {
        if (move.from == from && move.to == to) {
            if (promotion == PieceType::NONE || move.promotionPiece == promotion) {
                board.makeMove(move);
                currentColor = (currentColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
                return;
            }
        }
    }

    if (debug) {
        std::cout << "info string Illegal move: " << uciMove << std::endl;
    }
}

Color UCIEngine::getCurrentPlayerFromBoard() {
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
        timeToThink = movetime - 50;
    } else {
        Color color = getCurrentPlayerFromBoard();
        long time = (color == Color::WHITE) ? wtime : btime;
        long inc = (color == Color::WHITE) ? winc : binc;

        if (time > 0) {
            long moveOverhead = 50;
            long usableTime = time - moveOverhead;
            if (usableTime < 0) usableTime = 0;

            if (usableTime < 1000) {
                timeToThink = usableTime / 4;
                if (timeToThink < 1 && usableTime > 0) timeToThink = 1;
            } else {
                timeToThink = (usableTime / 30) + (inc / 2);
                if (timeToThink > static_cast<long>(usableTime * 0.8)) {
                    timeToThink = static_cast<long>(usableTime * 0.8);
                }
            }

            if (timeToThink < 50 && usableTime > 200) {
                timeToThink = 50;
            }
        }
    }

    if (depth > 0) {
        engine.setDepth(depth);
    } else if (infinite) {
        engine.setDepth(20);
        timeToThink = 0;
    } else if (timeToThink > 0) {
        engine.setDepth(20);
    } else {
        engine.setDepth(5);
    }

    Color currentCol = getCurrentPlayerFromBoard();
    Move bestMove = engine.getBestMove(board, currentCol, timeToThink);

    std::string uci = moveToUCI(bestMove);

    // Validate the bestmove - if empty or invalid, try fallback
    if (uci.empty() || uci == "a0a0" || uci.length() < 4) {
        // Fallback: pick the first legal move
        std::vector<Move> legalMoves = board.getAllLegalMoves(currentCol);
        if (!legalMoves.empty()) {
            bestMove = legalMoves[0];
            uci = moveToUCI(bestMove);
            std::cerr << "info string WARNING: search returned no move, using fallback: " << uci << std::endl;
        } else {
            // No legal moves at all (checkmate or stalemate)
            std::cout << "bestmove 0000" << std::endl;
            return;
        }
    }

    std::cout << "bestmove " << uci << std::endl;
}

std::string UCIEngine::moveToUCI(const Move& move) {
    std::string uciMove = move.from.toNotation() + move.to.toNotation();

    if (move.promotionPiece != PieceType::NONE) {
        switch (move.promotionPiece) {
            case PieceType::QUEEN:  uciMove += "q"; break;
            case PieceType::ROOK:   uciMove += "r"; break;
            case PieceType::BISHOP: uciMove += "b"; break;
            case PieceType::KNIGHT: uciMove += "n"; break;
            default: break;
        }
    }

    return uciMove;
}

void UCIEngine::handleStop() {
    // In a multithreaded engine, this would signal the search to stop
}

void UCIEngine::handleQuit() {
    std::exit(0);
}
