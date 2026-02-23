#include "ChessBoard.h"
#include "King.h"
#include "Queen.h"
#include "Rook.h"
#include "Bishop.h"
#include "Knight.h"
#include "Pawn.h"
#include <iostream>
#include <sstream>
#include <cmath>
#include <cctype>

ChessBoard::ChessBoard() : enPassantTarget(nullptr) {
    for (int i = 0; i < 120; i++) {
        board[i] = nullptr;
    }
}

ChessBoard::~ChessBoard() {
    for (int i = 0; i < 120; i++) {
        delete board[i];
        board[i] = nullptr;
    }
    delete enPassantTarget;
}

Piece* ChessBoard::createPiece(Color color, PieceType type) {
    switch (type) {
        case PieceType::KING:   return new King(color);
        case PieceType::QUEEN:  return new Queen(color);
        case PieceType::ROOK:   return new Rook(color);
        case PieceType::BISHOP: return new Bishop(color);
        case PieceType::KNIGHT: return new Knight(color);
        case PieceType::PAWN:   return new Pawn(color);
        default: return nullptr;
    }
}

Piece* ChessBoard::getPiece(const Position& pos) const {
    return board[pos.square];
}

void ChessBoard::setPiece(const Position& pos, Piece* piece) {
    board[pos.square] = piece;
}

const Position* ChessBoard::getEnPassantTarget() const {
    return enPassantTarget;
}

void ChessBoard::setEnPassantTarget(const Position* target) {
    delete enPassantTarget;
    if (target) {
        enPassantTarget = new Position(target->square);
    } else {
        enPassantTarget = nullptr;
    }
}

Position ChessBoard::getKingPosition(Color color) const {
    int key = (color == Color::WHITE) ? 0 : 1;
    auto it = kingPositions.find(key);
    if (it != kingPositions.end()) {
        return it->second;
    }
    return Position(0); // Fallback
}

bool ChessBoard::makeMove(Move& move) {
    Piece* piece = getPiece(move.from);
    if (piece == nullptr) return false;

    // Save previous state
    move.capturedPiece = getPiece(move.to);
    if (enPassantTarget) {
        move.prevEnPassantTarget = *enPassantTarget;
    } else {
        move.prevEnPassantTarget = Position(-1); // sentinel
    }
    move.prevFromMoved = piece->getHasMoved();
    if (move.capturedPiece != nullptr) {
        move.prevToMoved = move.capturedPiece->getHasMoved();
    }

    // Handle en passant
    if (move.isEnPassant) {
        int captureSquare = (piece->getColor() == Color::WHITE)
            ? move.to.square + 10 : move.to.square - 10;
        move.capturedPiece = getPiece(Position::get(captureSquare));
        setPiece(Position::get(captureSquare), nullptr);
    }

    // Handle castling
    if (piece->getType() == PieceType::KING && std::abs(move.to.square - move.from.square) == 2) {
        move.isCastling = true;
        bool kingside = move.to.square > move.from.square;
        int rookFromSquare = kingside ? move.from.square + 3 : move.from.square - 4;
        int rookToSquare = kingside ? move.to.square - 1 : move.to.square + 1;

        Piece* rook = getPiece(Position(rookFromSquare));
        move.prevRookMoved = rook->getHasMoved();
        move.rookFromSquare = rookFromSquare;

        setPiece(Position(rookFromSquare), nullptr);
        setPiece(Position(rookToSquare), rook);
        rook->setMoved();
    }

    // Make the move
    setPiece(move.from, nullptr);

    // Handle pawn promotion
    Piece* pieceOnTarget = piece;
    if (piece->getType() == PieceType::PAWN && move.promotionPiece != PieceType::NONE) {
        move.promotedPiece = createPiece(piece->getColor(), move.promotionPiece);
        pieceOnTarget = move.promotedPiece;
    }

    setPiece(move.to, pieceOnTarget);
    piece->setMoved();

    // Update king position
    if (piece->getType() == PieceType::KING) {
        int key = (piece->getColor() == Color::WHITE) ? 0 : 1;
        kingPositions[key] = move.to;
    }

    // Update en passant target
    delete enPassantTarget;
    enPassantTarget = nullptr;
    if (piece->getType() == PieceType::PAWN && std::abs(move.to.square - move.from.square) == 20) {
        int epSquare = (move.from.square + move.to.square) / 2;
        enPassantTarget = new Position(epSquare);
    }

    updateCastlingRights();

    activeColor = (activeColor == Color::WHITE) ? Color::BLACK : Color::WHITE;

    return true;
}

void ChessBoard::undoMove(Move& move) {
    activeColor = (activeColor == Color::WHITE) ? Color::BLACK : Color::WHITE;

    Piece* piece = getPiece(move.to);

    // Undo promotion
    if (move.promotedPiece != nullptr) {
        Pawn* pawn = new Pawn(piece->getColor());
        setPiece(move.from, pawn);
    } else {
        setPiece(move.from, piece);
    }

    // Add captured piece back
    if (move.capturedPiece != nullptr && !move.isEnPassant) {
        setPiece(move.to, move.capturedPiece);
    } else {
        setPiece(move.to, nullptr);
    }

    // Restore moved flags
    Piece* fromPiece = getPiece(move.from);
    if (fromPiece != nullptr)
        fromPiece->hasMoved = move.prevFromMoved;
    if (move.capturedPiece != nullptr)
        move.capturedPiece->hasMoved = move.prevToMoved;

    // Update king position
    if (fromPiece != nullptr && fromPiece->getType() == PieceType::KING) {
        int key = (fromPiece->getColor() == Color::WHITE) ? 0 : 1;
        kingPositions[key] = move.from;
    }

    // Handle castling undo
    if (move.isCastling) {
        bool kingside = move.to.square > move.from.square;
        int rookFromSquare = kingside ? move.from.square + 3 : move.from.square - 4;
        int rookToSquare = kingside ? move.to.square - 1 : move.to.square + 1;

        Piece* rook = getPiece(Position(rookToSquare));

        setPiece(Position(rookToSquare), nullptr);
        setPiece(Position(rookFromSquare), rook);
        rook->hasMoved = move.prevRookMoved;
    }

    // Handle en passant undo
    if (move.isEnPassant) {
        Color pieceColor = getPiece(move.from)->getColor();
        int captureSquare = (pieceColor == Color::WHITE)
            ? move.to.square + 10 : move.to.square - 10;
        setPiece(Position::get(captureSquare), move.capturedPiece);
        setPiece(move.to, nullptr);
    }

    // Restore en passant target
    delete enPassantTarget;
    if (move.prevEnPassantTarget.square >= 0) {
        enPassantTarget = new Position(move.prevEnPassantTarget.square);
    } else {
        enPassantTarget = nullptr;
    }

    updateCastlingRights();

    // Clean up promoted piece
    if (move.promotedPiece != nullptr) {
        delete move.promotedPiece;
        move.promotedPiece = nullptr;
    }
}

int ChessBoard::getCastlingMask() const {
    int mask = 0;
    if (canCastleWhiteKingside)  mask |= 1;
    if (canCastleWhiteQueenside) mask |= 2;
    if (canCastleBlackKingside)  mask |= 4;
    if (canCastleBlackQueenside) mask |= 8;
    return mask;
}

void ChessBoard::updateCastlingRights() {
    Piece* wKing = board[95]; // e1
    bool wk = (wKing != nullptr && wKing->getType() == PieceType::KING && !wKing->hasMoved);
    Piece* wRookK = board[98]; // h1
    Piece* wRookQ = board[91]; // a1

    canCastleWhiteKingside = wk && (wRookK != nullptr && !wRookK->hasMoved);
    canCastleWhiteQueenside = wk && (wRookQ != nullptr && !wRookQ->hasMoved);

    Piece* bKing = board[25]; // e8
    bool bk = (bKing != nullptr && bKing->getType() == PieceType::KING && !bKing->hasMoved);
    Piece* bRookK = board[28]; // h8
    Piece* bRookQ = board[21]; // a8

    canCastleBlackKingside = bk && (bRookK != nullptr && !bRookK->hasMoved);
    canCastleBlackQueenside = bk && (bRookQ != nullptr && !bRookQ->hasMoved);
}

// Zobrist logic removed

bool ChessBoard::isInCheck(Color color) {
    Position kingPos = getKingPosition(color);
    return isSquareUnderAttack(kingPos, color);
}

bool ChessBoard::wouldBeInCheck(Color color, const Position& from, const Position& to) {
    Piece* piece = getPiece(from);
    Piece* captured = getPiece(to);

    setPiece(from, nullptr);
    setPiece(to, piece);
    int key = (color == Color::WHITE) ? 0 : 1;
    Position origKingPos;
    if (piece->getType() == PieceType::KING) {
        origKingPos = kingPositions[key];
        kingPositions[key] = to;
    }

    bool inCheck = isInCheck(color);

    setPiece(from, piece);
    setPiece(to, captured);
    if (piece->getType() == PieceType::KING) {
        kingPositions[key] = origKingPos;
    }

    return inCheck;
}

bool ChessBoard::isSquareUnderAttack(const Position& pos, Color defendingColor) {
    Color attackingColor = (defendingColor == Color::WHITE) ? Color::BLACK : Color::WHITE;

    for (int square = 21; square <= 98; square++) {
        if ((square % 10) < 1 || (square % 10) > 8) continue;

        Piece* piece = board[square];
        if (piece != nullptr && piece->getColor() == attackingColor) {
            std::vector<Move> moves;
            if (piece->getType() == PieceType::KING) {
                King* king = static_cast<King*>(piece);
                moves = king->getPossibleMoves(Position::get(square), *this, true);
            } else {
                moves = piece->getPossibleMoves(Position::get(square), *this);
            }
            for (const Move& move : moves) {
                if (move.to == pos) {
                    return true;
                }
            }
        }
    }
    return false;
}

std::vector<Move> ChessBoard::getAllLegalMoves(Color color) {
    std::vector<Move> legalMoves;
    getAllLegalMoves(color, legalMoves);
    return legalMoves;
}

void ChessBoard::getAllLegalMoves(Color color, std::vector<Move>& legalMoves) {
    for (int square = 21; square <= 98; square++) {
        if ((square % 10) < 1 || (square % 10) > 8) continue;

        Piece* piece = board[square];
        if (piece != nullptr && piece->getColor() == color) {
            const Position& from = Position::get(square);
            std::vector<Move> possibleMoves = piece->getPossibleMoves(from, *this);

            for (Move& move : possibleMoves) {
                if (isLegalMove(move, color)) {
                    legalMoves.push_back(move);
                }
            }
        }
    }
}

void ChessBoard::generateCaptureMoves(Color color, std::vector<Move>& captureMoves) {
    for (int square = 21; square <= 98; square++) {
        if ((square % 10) < 1 || (square % 10) > 8) continue;

        Piece* piece = board[square];
        if (piece != nullptr && piece->getColor() == color) {
            const Position& from = Position::get(square);
            std::vector<Move> possibleMoves = piece->getPossibleMoves(from, *this);

            for (Move& move : possibleMoves) {
                // Only include captures and promotions
                bool isCapture = (getPiece(move.to) != nullptr) || move.isEnPassant;
                bool isPromotion = (move.promotionPiece != PieceType::NONE);

                if ((isCapture || isPromotion) && isLegalMove(move, color)) {
                    captureMoves.push_back(move);
                }
            }
        }
    }
}

bool ChessBoard::isLegalMove(Move& move, Color color) {
    // En passant special handling
    if (move.isEnPassant) {
        Piece* piece = getPiece(move.from);
        Piece* captured = getPiece(move.to);

        int captureSquare = (piece->getColor() == Color::WHITE)
            ? move.to.square + 10 : move.to.square - 10;
        Piece* capturedPawn = getPiece(Position::get(captureSquare));

        setPiece(move.from, nullptr);
        setPiece(move.to, piece);
        setPiece(Position::get(captureSquare), nullptr);

        bool inCheck = isInCheck(color);

        setPiece(move.from, piece);
        setPiece(move.to, captured);
        setPiece(Position::get(captureSquare), capturedPawn);

        if (inCheck) return false;
    } else {
        if (wouldBeInCheck(color, move.from, move.to)) {
            return false;
        }
    }

    // Castling check
    Piece* piece = getPiece(move.from);
    if (piece->getType() == PieceType::KING && std::abs(move.to.square - move.from.square) == 2) {
        if (isInCheck(color)) return false;

        bool kingside = move.to.square > move.from.square;
        int direction = kingside ? 1 : -1;

        for (int sq = move.from.square + direction; sq != move.to.square + direction; sq += direction) {
            if (wouldBeInCheck(color, move.from, Position::get(sq))) {
                return false;
            }
        }
    }

    return true;
}

bool ChessBoard::isCheckmate(Color color) {
    return isInCheck(color) && getAllLegalMoves(color).empty();
}

bool ChessBoard::isStalemate(Color color) {
    return !isInCheck(color) && getAllLegalMoves(color).empty();
}

void ChessBoard::display() const {
    std::cout << "  a b c d e f g h" << std::endl;
    for (int row = 0; row < 8; row++) {
        std::cout << (8 - row) << " ";
        for (int col = 0; col < 8; col++) {
            Piece* piece = board[21 + row * 10 + col];
            char symbol = piece != nullptr ? piece->getSymbol() : '.';
            std::cout << symbol << " ";
        }
        std::cout << (8 - row) << std::endl;
    }
    std::cout << "  a b c d e f g h" << std::endl;
}

void ChessBoard::setFromFEN(const std::string& fen) {
    // Clear board
    for (int i = 0; i < 120; i++) {
        delete board[i];
        board[i] = nullptr;
    }
    kingPositions.clear();

    std::istringstream ss(fen);
    std::string piecePlacement, activeColorStr, castlingStr, epStr;
    ss >> piecePlacement >> activeColorStr >> castlingStr >> epStr;

    // Parse piece placement
    int r = 0, c = 0;
    for (char ch : piecePlacement) {
        if (ch == '/') {
            r++;
            c = 0;
        } else if (std::isdigit(ch)) {
            c += ch - '0';
        } else {
            Color color = std::isupper(ch) ? Color::WHITE : Color::BLACK;
            PieceType type;
            switch (std::tolower(ch)) {
                case 'k': type = PieceType::KING; break;
                case 'q': type = PieceType::QUEEN; break;
                case 'r': type = PieceType::ROOK; break;
                case 'b': type = PieceType::BISHOP; break;
                case 'n': type = PieceType::KNIGHT; break;
                case 'p': type = PieceType::PAWN; break;
                default: c++; continue;
            }
            int square = 21 + r * 10 + c;
            board[square] = createPiece(color, type);
            if (type == PieceType::KING) {
                int key = (color == Color::WHITE) ? 0 : 1;
                kingPositions[key] = Position::get(square);
            }
            c++;
        }
    }

    // Active Color
    activeColor = (activeColorStr == "w") ? Color::WHITE : Color::BLACK;

    // Castling rights
    canCastleWhiteKingside = false;
    canCastleWhiteQueenside = false;
    canCastleBlackKingside = false;
    canCastleBlackQueenside = false;
    if (!castlingStr.empty() && castlingStr != "-") {
        for (char ch : castlingStr) {
            if (ch == 'K') canCastleWhiteKingside = true;
            if (ch == 'Q') canCastleWhiteQueenside = true;
            if (ch == 'k') canCastleBlackKingside = true;
            if (ch == 'q') canCastleBlackQueenside = true;
        }
    }

    // Set hasMoved flags based on castling rights
    // White King (e1 = square 95)
    Piece* whiteKing = board[95];
    if (whiteKing && whiteKing->getType() == PieceType::KING && whiteKing->getColor() == Color::WHITE) {
        if (!canCastleWhiteKingside && !canCastleWhiteQueenside)
            whiteKing->hasMoved = true;
    }
    Piece* whiteRookA = board[91];
    if (whiteRookA && whiteRookA->getType() == PieceType::ROOK && whiteRookA->getColor() == Color::WHITE) {
        if (!canCastleWhiteQueenside) whiteRookA->hasMoved = true;
    }
    Piece* whiteRookH = board[98];
    if (whiteRookH && whiteRookH->getType() == PieceType::ROOK && whiteRookH->getColor() == Color::WHITE) {
        if (!canCastleWhiteKingside) whiteRookH->hasMoved = true;
    }
    // Black King (e8 = square 25)
    Piece* blackKing = board[25];
    if (blackKing && blackKing->getType() == PieceType::KING && blackKing->getColor() == Color::BLACK) {
        if (!canCastleBlackKingside && !canCastleBlackQueenside)
            blackKing->hasMoved = true;
    }
    Piece* blackRookA = board[21];
    if (blackRookA && blackRookA->getType() == PieceType::ROOK && blackRookA->getColor() == Color::BLACK) {
        if (!canCastleBlackQueenside) blackRookA->hasMoved = true;
    }
    Piece* blackRookH = board[28];
    if (blackRookH && blackRookH->getType() == PieceType::ROOK && blackRookH->getColor() == Color::BLACK) {
        if (!canCastleBlackKingside) blackRookH->hasMoved = true;
    }

    // En passant
    delete enPassantTarget;
    enPassantTarget = nullptr;
    if (!epStr.empty() && epStr != "-") {
        enPassantTarget = new Position(epStr);
    }

    // Finalize state
    updateCastlingRights();
}
