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
    PieceID pieceType = InvalidPiece;
    SquareColor color = InvalidColor;
};

enum States {
    quantumPickFirst,
    quantumPickDest1,
    quantumPickSecond,
    quantumPickDest2,
    opponentPickPiece,
    opponentPickDest,
    superpositionResolve,
    gameEnded
};

#endif
