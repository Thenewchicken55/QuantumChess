#include "queen.h"
#include "board.h"
#include <utility>
#include <vector>

Queen::Queen(SquareColor color, Pos position, Board *board) : Piece(color, position, board) {

}

std::vector<Pos> Queen::getValidMoves(){
    std::vector<Pos> validMoves;
    int directions[8][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}, {1,1}, {1,-1}, {-1,1}, {-1,-1}};
    for (auto& dir : directions) {
        for (int i = 1; i <= 7; ++i) {
            Pos newPos = {pos.row + dir[0]*i, pos.column + dir[1]*i};
            if (newPos.row < 0 || newPos.row >= 8 || newPos.column < 0 || newPos.column >= 8)
                break;
            if (board->isEmpty(newPos)) {
                validMoves.push_back(newPos);
            } else {
                if (getPieceColor(board->getPieceID(newPos)) != color)
                    validMoves.push_back(newPos);
                break;
            }
        }
    }
    return validMoves;
}

PieceID Queen::getType() const{
    return (color == White) ? WQueen : BQueen;
}
