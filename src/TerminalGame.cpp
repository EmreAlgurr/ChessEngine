#include "TerminalGame.h"
#include <iostream>
#include <sstream>
#include <cctype>
#include <chrono>
#include <iomanip>

TerminalGame::TerminalGame()
    : humanColor(Color::WHITE), currentTurn(Color::WHITE), engineDepth(4) {
    engine.setDepth(engineDepth);
    board.setFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

std::string TerminalGame::trim(const std::string& s) {
    std::string result = s;
    while (!result.empty() && (result.back() == '\r' || result.back() == '\n' || result.back() == ' '))
        result.pop_back();
    while (!result.empty() && result.front() == ' ')
        result.erase(result.begin());
    return result;
}

void TerminalGame::showWelcome() {
    std::cout << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << "    Satranc Motoru - Terminal Modu" << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << "Komutlar:" << std::endl;
    std::cout << "  e2e4      - Hamle yap" << std::endl;
    std::cout << "  d         - Tahtayi goster" << std::endl;
    std::cout << "  moves     - Uygun hamleleri goster" << std::endl;
    std::cout << "  new       - Yeni oyun" << std::endl;
    std::cout << "  depth N   - Motor derinligini ayarla" << std::endl;
    std::cout << "  color w/b - Renk sec (w=beyaz, b=siyah)" << std::endl;
    std::cout << "  fen <FEN> - FEN pozisyonu yukle" << std::endl;
    std::cout << "  perft N   - Perft testi (derinlik N)" << std::endl;
    std::cout << "  divide N  - Perft divide (derinlik N)" << std::endl;
    std::cout << "  test      - Perft test suite calistir" << std::endl;
    std::cout << "  quit      - Cikis" << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << std::endl;
}

void TerminalGame::showBoard() {
    board.display();
    std::cout << std::endl;
}

void TerminalGame::newGame() {
    board.setFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    currentTurn = Color::WHITE;
    std::cout << "Yeni oyun baslatildi!" << std::endl;
    showBoard();
}

void TerminalGame::showLegalMoves() {
    std::vector<Move> moves = board.getAllLegalMoves(currentTurn);
    std::cout << "Uygun hamleler (" << moves.size() << "):";
    for (const Move& m : moves) {
        std::cout << " " << m.toUCI();
    }
    std::cout << std::endl;
}

bool TerminalGame::checkGameEnd() {
    if (board.isCheckmate(currentTurn)) {
        showBoard();
        std::string winner = (currentTurn == Color::WHITE) ? "Siyah" : "Beyaz";
        std::cout << "## SAH MAT! " << winner << " kazandi! ##" << std::endl;
        return true;
    }
    if (board.isStalemate(currentTurn)) {
        showBoard();
        std::cout << "## PAT! Oyun berabere. ##" << std::endl;
        return true;
    }
    if (board.isInCheck(currentTurn)) {
        std::cout << ">> SAH! <<" << std::endl;
    }
    return false;
}

void TerminalGame::handleEngineMove() {
    std::string turnStr = (currentTurn == Color::WHITE) ? "Beyaz" : "Siyah";
    std::cout << "[" << turnStr << " - Motor dusunuyor...]" << std::endl;
    std::cout.flush();

    auto start = std::chrono::steady_clock::now();
    Move bestMove = engine.getBestMove(board, currentTurn, 0);
    auto end = std::chrono::steady_clock::now();
    long ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::string uci = bestMove.toUCI();
    if (!uci.empty() && uci != "a0a0") {
        board.makeMove(bestMove);
        std::cout << "Motor hamlesi: " << uci << " (" << ms << "ms)" << std::endl;
        currentTurn = (currentTurn == Color::WHITE) ? Color::BLACK : Color::WHITE;
    } else {
        std::cout << "Motor hamle bulamadi!" << std::endl;
    }

    std::cout << std::endl;
    showBoard();
}

bool TerminalGame::tryMakeMove(const std::string& moveStr) {
    if (moveStr.length() < 4) return false;

    std::string fromStr = moveStr.substr(0, 2);
    std::string toStr = moveStr.substr(2, 2);
    Position from(fromStr);
    Position to(toStr);

    PieceType promotion = PieceType::NONE;
    if (moveStr.length() == 5) {
        char pc = moveStr[4];
        switch (pc) {
            case 'q': promotion = PieceType::QUEEN; break;
            case 'r': promotion = PieceType::ROOK; break;
            case 'b': promotion = PieceType::BISHOP; break;
            case 'n': promotion = PieceType::KNIGHT; break;
            default: break;
        }
    }

    std::vector<Move> legalMoves = board.getAllLegalMoves(currentTurn);
    for (Move& move : legalMoves) {
        if (move.from == from && move.to == to) {
            if (promotion == PieceType::NONE || move.promotionPiece == promotion) {
                board.makeMove(move);
                currentTurn = (currentTurn == Color::WHITE) ? Color::BLACK : Color::WHITE;
                return true;
            }
        }
    }
    return false;
}

void TerminalGame::handleSetDepth(const std::string& input) {
    try {
        int d = std::stoi(input.substr(6));
        if (d < 1 || d > 20) {
            std::cout << "Derinlik 1-20 arasi olmali." << std::endl;
            return;
        }
        engineDepth = d;
        engine.setDepth(d);
        std::cout << "Motor derinligi: " << d << std::endl;
    } catch (...) {
        std::cout << "Gecersiz derinlik. Kullanim: depth 4" << std::endl;
    }
}

void TerminalGame::handleSetColor(const std::string& input) {
    std::string arg = trim(input.substr(5));
    if (arg == "w" || arg == "white" || arg == "beyaz") {
        humanColor = Color::WHITE;
        std::cout << "Sen beyazla oynuyorsun." << std::endl;
    } else if (arg == "b" || arg == "black" || arg == "siyah") {
        humanColor = Color::BLACK;
        std::cout << "Sen siyahla oynuyorsun." << std::endl;
    } else {
        std::cout << "Gecersiz renk. Kullanim: color w veya color b" << std::endl;
    }
}

void TerminalGame::handlePerft(const std::string& input) {
    try {
        int depth = std::stoi(input.substr(5));
        if (depth < 1 || depth > 7) {
            std::cout << "Perft derinligi 1-7 arasi olmali." << std::endl;
            return;
        }

        std::cout << "Perft(" << depth << ") hesaplaniyor..." << std::endl;
        auto start = std::chrono::steady_clock::now();
        uint64_t nodes = perft(board, depth, currentTurn);
        auto end = std::chrono::steady_clock::now();
        long ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        std::cout << "Perft(" << depth << ") = " << nodes;
        if (ms > 0) {
            std::cout << "  (" << ms << "ms, " << (nodes * 1000 / ms) << " nps)";
        }
        std::cout << std::endl;
    } catch (...) {
        std::cout << "Kullanim: perft 5" << std::endl;
    }
}

// ===== PERFT =====
uint64_t TerminalGame::perft(ChessBoard& b, int depth, Color color) {
    if (depth == 0) return 1;

    std::vector<Move> moves = b.getAllLegalMoves(color);

    if (depth == 1) return moves.size();

    uint64_t nodes = 0;
    Color next = (color == Color::WHITE) ? Color::BLACK : Color::WHITE;

    for (Move& m : moves) {
        b.makeMove(m);
        nodes += perft(b, depth - 1, next);
        b.undoMove(m);
    }
    return nodes;
}

void TerminalGame::perftDivide(ChessBoard& b, int depth, Color color) {
    std::vector<Move> moves = b.getAllLegalMoves(color);
    Color next = (color == Color::WHITE) ? Color::BLACK : Color::WHITE;
    uint64_t total = 0;

    for (Move& m : moves) {
        b.makeMove(m);
        uint64_t cnt = (depth <= 1) ? 1 : perft(b, depth - 1, next);
        b.undoMove(m);
        std::cout << m.toUCI() << ": " << cnt << std::endl;
        total += cnt;
    }
    std::cout << std::endl;
    std::cout << "Total: " << total << std::endl;
}

void TerminalGame::runPerftSuite() {
    struct PerftTest {
        std::string name;
        std::string fen;
        int depth;
        uint64_t expected;
    };

    std::vector<PerftTest> tests = {
        // Position 1 - Initial position
        {"Startpos depth 1", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 1, 20},
        {"Startpos depth 2", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 2, 400},
        {"Startpos depth 3", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 3, 8902},
        {"Startpos depth 4", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 4, 197281},

        // Position 2 - Kiwipete (many special moves)
        {"Kiwipete depth 1", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 1, 48},
        {"Kiwipete depth 2", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 2, 2039},
        {"Kiwipete depth 3", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 3, 97862},

        // Position 3 - Endgame with en passant
        {"Position 3 depth 1", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 1, 14},
        {"Position 3 depth 2", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 2, 191},
        {"Position 3 depth 3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 3, 2812},
        {"Position 3 depth 4", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 4, 43238},
        {"Position 3 depth 5", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 5, 674624},

        // Position 4 - Promotions + castling
        {"Position 4 depth 1", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 1, 6},
        {"Position 4 depth 2", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 2, 264},
        {"Position 4 depth 3", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 3, 9467},
        {"Position 4 depth 4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 4, 422333},

        // Position 5 - Tricky position
        {"Position 5 depth 1", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 1, 44},
        {"Position 5 depth 2", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 2, 1486},
        {"Position 5 depth 3", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 3, 62379},

        // Position 6 - Complex middlegame
        {"Position 6 depth 1", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 1, 46},
        {"Position 6 depth 2", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 2, 2079},
        {"Position 6 depth 3", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 3, 89890},
    };

    int passed = 0;
    int failed = 0;

    std::cout << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << "         PERFT TEST SUITE" << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << std::endl;

    for (const auto& test : tests) {
        ChessBoard b;
        b.setFromFEN(test.fen);

        // Determine color from FEN
        Color color = Color::WHITE;
        std::istringstream ss(test.fen);
        std::string dummy, colorStr;
        ss >> dummy >> colorStr;
        if (colorStr == "b") color = Color::BLACK;

        auto start = std::chrono::steady_clock::now();
        uint64_t result = perft(b, test.depth, color);
        auto end = std::chrono::steady_clock::now();
        long ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        bool ok = (result == test.expected);
        if (ok) passed++; else failed++;

        std::cout << (ok ? "[PASS]" : "[FAIL]") << " " << std::left << std::setw(25) << test.name
                  << " => " << std::setw(10) << result;
        if (!ok) {
            std::cout << " (beklenen: " << test.expected << ")";
        }
        if (ms > 0) {
            std::cout << "  [" << ms << "ms]";
        }
        std::cout << std::endl;
    }

    std::cout << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << "  Sonuc: " << passed << " GECTI, " << failed << " KALDI" << std::endl;
    if (failed == 0) {
        std::cout << "  TUMU BASARILI!" << std::endl;
    } else {
        std::cout << "  HATALAR VAR - hamle uretimi kontrol edilmeli!" << std::endl;
    }
    std::cout << "============================================" << std::endl;
    std::cout << std::endl;
}

void TerminalGame::run() {
    showWelcome();
    showBoard();

    std::string line;
    while (true) {
        // If it's the engine's turn, let it play
        if (currentTurn != humanColor) {
            handleEngineMove();
            if (checkGameEnd()) break;
            continue;
        }

        // Player's turn
        std::string turnStr = (currentTurn == Color::WHITE) ? "Beyaz" : "Siyah";
        std::cout << "[" << turnStr << " - Senin hamlen] > ";
        std::cout.flush();

        if (!std::getline(std::cin, line)) break;
        line = trim(line);
        if (line.empty()) continue;

        // === Commands ===
        if (line == "quit" || line == "exit") {
            std::cout << "Gorusuruz!" << std::endl;
            break;
        }
        if (line == "d" || line == "display") {
            showBoard();
            continue;
        }
        if (line == "moves") {
            showLegalMoves();
            continue;
        }
        if (line == "new") {
            newGame();
            continue;
        }
        if (line == "test") {
            runPerftSuite();
            continue;
        }
        if (line.length() >= 6 && line.substr(0, 5) == "depth") {
            handleSetDepth(line);
            continue;
        }
        if (line.length() >= 6 && line.substr(0, 5) == "color") {
            handleSetColor(line);
            continue;
        }
        if (line.length() >= 6 && line.substr(0, 5) == "perft") {
            handlePerft(line);
            continue;
        }
        if (line.length() >= 7 && line.substr(0, 6) == "divide") {
            try {
                int depth = std::stoi(line.substr(7));
                std::cout << "Divide(" << depth << "):" << std::endl;
                perftDivide(board, depth, currentTurn);
            } catch (...) {
                std::cout << "Kullanim: divide 3" << std::endl;
            }
            continue;
        }
        if (line.length() >= 4 && line.substr(0, 3) == "fen") {
            std::string fen = trim(line.substr(4));
            if (fen.empty()) {
                std::cout << "Kullanim: fen <FEN string>" << std::endl;
            } else {
                board.setFromFEN(fen);
                // Determine color from FEN
                std::istringstream ss(fen);
                std::string dummy, colorStr;
                ss >> dummy >> colorStr;
                currentTurn = (colorStr == "b") ? Color::BLACK : Color::WHITE;
                std::cout << "FEN yuklendi." << std::endl;
                showBoard();
            }
            continue;
        }

        // === Try as a move ===
        if (line.length() >= 4 && std::islower(line[0]) && std::isdigit(line[1])
            && std::islower(line[2]) && std::isdigit(line[3])) {

            if (tryMakeMove(line)) {
                std::cout << std::endl;
                showBoard();
                if (checkGameEnd()) break;
            } else {
                std::cout << "Gecersiz hamle: " << line << std::endl;
                showLegalMoves();
            }
        } else {
            std::cout << "Bilinmeyen komut: " << line << std::endl;
            std::cout << "Yardim: e2e4 (hamle), d (tahta), moves, new, depth N, perft N, test, quit" << std::endl;
        }
    }
}
