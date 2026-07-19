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
#include <random>

class Board{
private:
    std::unique_ptr<Piece> pieces[8][8];

    Pos enPassantTarget = {-1, -1};

    SuperpositionState superposition;

    // PRNG used for superposition collapse. Member (not static) so collapse
    // behaviour is deterministic given a seed — useful for tests.
    std::mt19937 rng;

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

    PieceID getPieceID(Pos square) const;

    // Execute a move. If the move is a pawn reaching the last rank, it promotes
    // to `promoteTo` (or to a Queen if InvalidPiece, for backward compat).
    void movePiece(Move move, PieceID promoteTo = InvalidPiece);

    // Checks if the square on board is empty (considering superposition as occupied)
    bool isEmpty(Pos pos) const;

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
    float getSuperpositionProbabilityF() const;
};

#endif
