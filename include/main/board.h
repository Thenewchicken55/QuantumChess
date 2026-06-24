/*
    Board.h

    Board class for QuantumChess Project

    ZipCode
*/

#ifndef BOARD_H
#define BOARD_H

#include "piece.h"
#include <memory>
#include <vector>
#include <functional>

class Board{
private:
    std::unique_ptr<Piece> pieces[8][8];

    Pos enPassantTarget = {-1, -1};

    SuperpositionState superposition;

    // returns a reference to the piece at the given position
    std::unique_ptr<Piece>& getPiece(Pos pos);

    // returns raw valid moves from the piece (without legality filtering)
    std::vector<Pos> getRawPieceMoves(Pos piecePos);

public:
    Board();

    void resetBoard();

    void getPiecesMoves(Pos piecePos, std::vector<Pos>& movesAvailable);

    // Returns a list of pieces on the Board. Stores position and type of each piece
    std::vector<std::pair<Pos, PieceID>> getPieces();

    // Returns pieces including superposition ghost positions
    std::vector<std::pair<Pos, PieceID>> getAllVisiblePieces();

    PieceID getPieceID(Pos square);

    void movePiece(Move move);

    // Checks if the square on board is empty (considering superposition as occupied)
    bool isEmpty(Pos pos);

    // Check if the given color's king is in check
    bool isInCheck(SquareColor color);

    // Check if the given square is attacked by any piece of byColor
    bool isSquareAttacked(Pos square, SquareColor byColor);

    // Check if making this move would leave the current player's king in check
    bool isLegalMove(Move move);

    // Find the king position for the given color
    Pos findKing(SquareColor color);

    // En passant target accessors
    Pos getEnPassantTarget() const;

    // Castling checks
    bool canCastleKingSide(Pos kingPos, SquareColor color);
    bool canCastleQueenSide(Pos kingPos, SquareColor color);

    // Check if the given color is in checkmate
    bool isCheckmate(SquareColor color);

    // Check if the given color is in stalemate
    bool isStalemate(SquareColor color);

    // Superposition methods
    void enterSuperposition(Pos original, const std::vector<Pos>& dests, PieceID type, SquareColor color);
    void addSuperpositionPositions(const std::vector<Pos>& newDests);
    Pos collapseSuperposition();
    void collapseToPosition(Pos pos);
    bool hasActiveSuperposition() const;
    const SuperpositionState& getSuperposition() const;
    void clearSuperposition();
    bool isSuperpositionSquare(Pos pos) const;
    bool isSuperpositionOriginal(Pos pos) const;
    int getSuperpositionProbability() const;
};

#endif
