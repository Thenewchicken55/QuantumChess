#ifndef TYPES_H
#define TYPES_H

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
    int row;
    int column;

    bool operator==(const Pos& rightSide) const {
        return row == rightSide.row && column == rightSide.column;
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
    Pos pos1;
    Pos pos2;
    int numPositions = 0;
    PieceID pieceType = InvalidPiece;
    SquareColor color = InvalidColor;
};

enum States {
    selectPiece,
    selectDest1,
    selectDest2,
    opponentPickPiece,
    opponentPickDest,
    gameEnded
};

#endif
