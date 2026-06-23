#include "king.h"
#include "board.h"
#include <utility>
#include <vector>

King::King(SquareColor color, Pos position, Board *board) : Piece(color, position, board) {

}

std::vector<Pos> King::getValidMoves(){
    std::vector<Pos> validMoves;
    int directions[8][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}, {1,1}, {1,-1}, {-1,1}, {-1,-1}};
    for (auto& dir : directions) {
        Pos newPos = {pos.row + dir[0], pos.column + dir[1]};
        if (newPos.row < 0 || newPos.row >= 8 || newPos.column < 0 || newPos.column >= 8)
            continue;
        if (board->isEmpty(newPos) || getPieceColor(board->getPieceID(newPos)) != color)
            validMoves.push_back(newPos);
    }

    // Castling
    if (board->canCastleKingSide(pos, color))
        validMoves.push_back({pos.row, pos.column + 2});
    if (board->canCastleQueenSide(pos, color))
        validMoves.push_back({pos.row, pos.column - 2});

    return validMoves;
}

PieceID King::getType() const{
    return (color == White) ? WKing : BKing;
}
