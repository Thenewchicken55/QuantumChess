/*
    Piece.h

    Piece class for QuantumChess Project

    ZipCode
*/

#ifndef PIECE_H
#define PIECE_H

#include "types.h"
#include <utility>
#include <vector>

class Board;

class Piece{
protected:
    // Assuming this is the piece's color
    SquareColor color;
    Pos pos;
    Board *board;
    bool hasMoved = false;

public:
    Piece(SquareColor color, Pos pos, Board *board);
    virtual ~Piece() = default;

    Pos getPosition() const;

    SquareColor getColor() const;

    // Moves the piece can make this turn (filtered by occupancy / en passant).
    virtual std::vector<Pos> getValidMoves() = 0;

    // Squares this piece attacks regardless of occupancy. Equal to getValidMoves()
    // for every piece except the pawn, whose forward move is not an attack.
    // Used for check / castling-through-check detection.
    virtual std::vector<Pos> getAttackSquares() { return getValidMoves(); }

    virtual PieceID getType() const = 0;

    void setPosition(Pos newPos);

    bool getHasMoved() const;
};

#endif
