#include "knight.h"
#include "board.h"
#include <utility>
#include <vector>

Knight::Knight(SquareColor color, Pos position, Board *board) : Piece(color, position, board) {

}

std::vector<Pos> Knight::getValidMoves(){
    std::vector<Pos> validMoves;
    int offsets[8][2] = {{2,1}, {2,-1}, {-2,1}, {-2,-1}, {1,2}, {1,-2}, {-1,2}, {-1,-2}};
    for (auto& off : offsets) {
        Pos newPos = {pos.row + off[0], pos.column + off[1]};
        if (newPos.row < 0 || newPos.row >= 8 || newPos.column < 0 || newPos.column >= 8)
            continue;
        if (board->isEmpty(newPos) || getPieceColor(board->getPieceID(newPos)) != color)
            validMoves.push_back(newPos);
    }
    return validMoves;
}

PieceID Knight::getType() const{
    return (color == White) ? WKnight : BKnight;
}
