#ifndef TYPES_H
#define TYPES_H

#include <vector>

enum SquareColor{
    White,
    Black,
    InvalidColor
};

enum PieceID{
    WKing,
    WQueen,
    WBishop,
    WKnight,
    WRook,
    WPawn,
    BKing,
    BQueen,
    BBishop,
    BKnight,
    BRook,
    BPawn,
    InvalidPiece
};

SquareColor getPieceColor(PieceID ID);

struct Pos{
    int row = -1;
    int column = -1;

    bool operator==(const Pos& rightSide) const {
        return row == rightSide.row && column == rightSide.column;
    }

    bool operator!=(const Pos& rightSide) const {
        return !(*this == rightSide);
    }

    // A position is valid if it falls inside the 8x8 board.
    bool isValid() const {
        return row >= 0 && row < 8 && column >= 0 && column < 8;
    }
};

struct Move{
    Pos start;
    Pos end;

    bool operator==(const Move& rightSide) const {
        return start == rightSide.start && end == rightSide.end;
    }
};

struct MovePair{
    Move m1;
    Move m2;
};

struct SuperpositionState {
    bool active = false;
    Pos originalPos;
    std::vector<Pos> positions;
    PieceID pieceType = InvalidPiece;
    SquareColor color = InvalidColor;
};

enum States {
    selectPiece,
    selectDest1,
    selectDest2,
    gameEnded
};

#endif
