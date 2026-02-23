#pragma once

#include "Types.h"
#include "Position.h"
#include "Move.h"
#include "Piece.h"
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <string>

class ChessBoard {
public:
    // Castling rights (public, matching Java)
    bool canCastleWhiteKingside = false;
    bool canCastleWhiteQueenside = false;
    bool canCastleBlackKingside = false;
    bool canCastleBlackQueenside = false;

    // Active Color (Side to Move)
    Color activeColor = Color::WHITE;

    ChessBoard();
    ~ChessBoard();

    Piece* getPiece(const Position& pos) const;
    void setPiece(const Position& pos, Piece* piece);

    const Position* getEnPassantTarget() const;
    void setEnPassantTarget(const Position* target);

    Position getKingPosition(Color color) const;

    bool makeMove(Move& move);
    void undoMove(Move& move);

    int getCastlingMask() const;

    bool isInCheck(Color color);
    bool wouldBeInCheck(Color color, const Position& from, const Position& to);

    std::vector<Move> getAllLegalMoves(Color color);
    void getAllLegalMoves(Color color, std::vector<Move>& legalMoves);

    // Generate only legal capture and promotion moves (for Quiescence Search)
    void generateCaptureMoves(Color color, std::vector<Move>& captureMoves);

    bool isCheckmate(Color color);
    bool isStalemate(Color color);

    void display() const;
    void setFromFEN(const std::string& fen);

private:
    Piece* board[120]; // 120-square mailbox
    Position* enPassantTarget;
    std::unordered_map<int, Position> kingPositions; // Color ordinal -> Position

    Piece* createPiece(Color color, PieceType type);
    void updateCastlingRights();
    bool isSquareUnderAttack(const Position& pos, Color defendingColor);
    bool isLegalMove(Move& move, Color color);
};
