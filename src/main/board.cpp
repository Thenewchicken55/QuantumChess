/*
    Board.cpp

    Board class for QuantumChess Project

    ZipCode
*/

#include "types.h"
#include "board.h"
#include "pawn.h"
#include "bishop.h"
#include "rook.h"
#include "knight.h"
#include "queen.h"
#include "king.h"
#include <optional>
#include <functional>
#include <stdexcept>

Board::Board(){
    resetBoard();
}

// Returns a list of pieces on the Board. Stores position and type of each piece
std::vector<std::pair<Pos, PieceID>> Board::getPieces() {
    std::vector<std::pair<Pos, PieceID>> pieceList;
    for (int i = 0; i < 8; ++i){
        for (int j = 0; j < 8; ++j){
            if (pieces[i][j] != nullptr){
                pieceList.push_back(std::make_pair<Pos, PieceID>(pieces[i][j].get()->getPosition(), pieces[i][j].get()->getType()));
            }
        }
    }
    return pieceList;
}

std::unique_ptr<Piece>& Board::getPiece(Pos pos) {
    if (pos.row < 0 || pos.row >= 8 || pos.column < 0 || pos.column >= 8) {
        throw std::out_of_range("Position out of bounds");
    }
    return pieces[pos.row][pos.column];
}

void Board::resetBoard(){
    for (int i = 0; i < 8; ++i){
        for (int j = 0; j < 8; ++j){
            SquareColor color = (i == 7 || i == 6) ? Black : White;
            if (i == 1 || i==6){
                pieces[i][j] = std::make_unique<Pawn>(color, (Pos){i, j}, this);
            }
            else if (i == 0 || i == 7){
                if (j == 0 || j == 7)
                    pieces[i][j] = std::make_unique<Rook>(color, (Pos){i, j}, this);
                else if (j == 1 || j == 6)
                    pieces[i][j] = std::make_unique<Knight>(color, (Pos){i, j}, this);
                else if (j == 2 || j == 5)
                    pieces[i][j] = std::make_unique<Bishop>(color, (Pos){i, j}, this);
                else if (j == 4)
                    pieces[i][j] = std::make_unique<King>(color, (Pos){i, j}, this);
                else if (j == 3)
                    pieces[i][j] = std::make_unique<Queen>(color, (Pos){i, j}, this);
            }
            else {
                pieces[i][j] = nullptr;
            }
        }
    }
}

void Board::movePiece(Move move) {
    if(move.start.row < 0 || move.start.row >= 8 || move.start.column < 0 || move.start.column >= 8 ||
        move.end.row < 0 || move.end.row >= 8 || move.end.column < 0 || move.end.column >= 8) {
        return; // Invalid move
    }
    if (pieces[move.start.row][move.start.column] == nullptr) {
        return; // No piece at the source position
    }

    if (pieces[move.start.row][move.start.column] != nullptr) {
        pieces[move.end.row][move.end.column] = std::move(pieces[move.start.row][move.start.column]);
        pieces[move.end.row][move.end.column]->setPosition(move.end);
        pieces[move.start.row][move.start.column] = nullptr;
    }
}

PieceID Board::getPieceID(Pos square){
    if (pieces[square.row][square.column] == nullptr)
        return InvalidPiece;
    else
        return pieces[square.row][square.column]->getType();
}

bool Board::isEmpty(Pos pos) {
    return pieces[pos.row][pos.column] == nullptr;
}

std::vector<Pos> Board::getRawPieceMoves(Pos piecePos) {
    return pieces[piecePos.row][piecePos.column]->getValidMoves();
}

void Board::getPiecesMoves(Pos piecePos, std::vector<Pos>& movesAvailable){
    std::vector<Pos> rawMoves = getRawPieceMoves(piecePos);
    movesAvailable.clear();
    SquareColor pieceColor = pieces[piecePos.row][piecePos.column]->getColor();

    for (auto& endPos : rawMoves) {
        Move trialMove = {piecePos, endPos};
        if (isLegalMove(trialMove)) {
            movesAvailable.push_back(endPos);
        }
    }
}

bool Board::isLegalMove(Move move) {
    SquareColor movingColor = pieces[move.start.row][move.start.column]->getColor();

    auto captured = std::move(pieces[move.end.row][move.end.column]);
    pieces[move.end.row][move.end.column] = std::move(pieces[move.start.row][move.start.column]);
    pieces[move.end.row][move.end.column]->setPosition(move.end);
    pieces[move.start.row][move.start.column] = nullptr;

    bool inCheck = isInCheck(movingColor);

    pieces[move.start.row][move.start.column] = std::move(pieces[move.end.row][move.end.column]);
    pieces[move.start.row][move.start.column]->setPosition(move.start);
    pieces[move.end.row][move.end.column] = std::move(captured);

    return !inCheck;
}

Pos Board::findKing(SquareColor color) {
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            if (pieces[i][j] != nullptr && pieces[i][j]->getType() == (color == White ? WKing : BKing)) {
                return {i, j};
            }
        }
    }
    return {-1, -1};
}

bool Board::isSquareAttacked(Pos square, SquareColor byColor) {
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            if (pieces[i][j] == nullptr) continue;
            if (getPieceColor(pieces[i][j]->getType()) != byColor) continue;

            std::vector<Pos> rawMoves = pieces[i][j]->getValidMoves();
            for (auto& move : rawMoves) {
                if (move.row == square.row && move.column == square.column) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool Board::isInCheck(SquareColor color) {
    Pos kingPos = findKing(color);
    if (kingPos.row == -1) return false;
    SquareColor enemyColor = (color == White) ? Black : White;
    return isSquareAttacked(kingPos, enemyColor);
}
