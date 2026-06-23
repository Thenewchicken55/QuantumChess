#include "pawn.h"
#include "board.h"
#include <utility>
#include <vector>

Pawn::Pawn(SquareColor color, Pos position, Board* board) : Piece(color, position, board) {

}

std::vector<Pos> Pawn::getValidMoves(){
    int directionMultiplier = (color == White) ? 1 : -1;
    std::vector<Pos> moves;

    Pos forward = {pos.row + 1*directionMultiplier, pos.column};
    if (forward.row >= 0 && forward.row < 8 && board->isEmpty(forward))
        moves.push_back(forward);

    int startRow = (color == White) ? 1 : 6;
    Pos forwardTwo = {pos.row + 2*directionMultiplier, pos.column};
    if (pos.row == startRow && board->isEmpty(forward) && board->isEmpty(forwardTwo))
        moves.push_back(forwardTwo);

    Pos diagonalRight = {pos.row + 1*directionMultiplier, pos.column + 1};
    if (diagonalRight.row >= 0 && diagonalRight.row < 8 && diagonalRight.column >= 0 && diagonalRight.column < 8)
        if (!board->isEmpty(diagonalRight) && getPieceColor(board->getPieceID(diagonalRight)) != color)
            moves.push_back(diagonalRight);

    Pos diagonalLeft = {pos.row + 1*directionMultiplier, pos.column - 1};
    if (diagonalLeft.row >= 0 && diagonalLeft.row < 8 && diagonalLeft.column >= 0 && diagonalLeft.column < 8)
        if (!board->isEmpty(diagonalLeft) && getPieceColor(board->getPieceID(diagonalLeft)) != color)
            moves.push_back(diagonalLeft);

    // En passant
    Pos enPassantTarget = board->getEnPassantTarget();
    if (enPassantTarget.row != -1) {
        // En passant capture to the right
        Pos epRight = {pos.row + 1*directionMultiplier, pos.column + 1};
        if (epRight.row == enPassantTarget.row && epRight.column == enPassantTarget.column)
            moves.push_back(epRight);

        // En passant capture to the left
        Pos epLeft = {pos.row + 1*directionMultiplier, pos.column - 1};
        if (epLeft.row == enPassantTarget.row && epLeft.column == enPassantTarget.column)
            moves.push_back(epLeft);
    }

    return moves;
}

PieceID Pawn::getType() const{
    return (color == White) ? WPawn : BPawn;
}
