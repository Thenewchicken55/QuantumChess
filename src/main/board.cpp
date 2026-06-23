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
#include <cstdlib>

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
    enPassantTarget = {-1, -1};
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
        return;
    }
    if (pieces[move.start.row][move.start.column] == nullptr) {
        return;
    }

    PieceID movingType = pieces[move.start.row][move.start.column]->getType();
    SquareColor movingColor = getPieceColor(movingType);
    int directionMultiplier = (movingColor == White) ? 1 : -1;

    bool isEnPassantCapture = false;
    bool isDoublePawnPush = false;

    // Check for en passant capture
    if ((movingType == WPawn || movingType == BPawn) &&
        move.end.row == enPassantTarget.row && move.end.column == enPassantTarget.column) {
        isEnPassantCapture = true;
    }

    // Check for double pawn push
    if ((movingType == WPawn || movingType == BPawn) &&
        abs(move.end.row - move.start.row) == 2) {
        isDoublePawnPush = true;
    }

    // Clear en passant target before the move
    enPassantTarget = {-1, -1};

    // Handle en passant: remove the captured pawn
    if (isEnPassantCapture) {
        Pos capturedPawnPos = {move.start.row, move.end.column};
        pieces[capturedPawnPos.row][capturedPawnPos.column] = nullptr;
    }

    // Perform the move
    pieces[move.end.row][move.end.column] = std::move(pieces[move.start.row][move.start.column]);
    pieces[move.end.row][move.end.column]->setPosition(move.end);
    pieces[move.start.row][move.start.column] = nullptr;

    // Handle castling: move the rook
    if (movingType == WKing || movingType == BKing) {
        if (abs(move.end.column - move.start.column) == 2) {
            int row = move.start.row;
            if (move.end.column == 6) {
                // King-side: rook from col 7 to col 5
                pieces[row][5] = std::move(pieces[row][7]);
                pieces[row][5]->setPosition({row, 5});
            } else if (move.end.column == 2) {
                // Queen-side: rook from col 0 to col 3
                pieces[row][3] = std::move(pieces[row][0]);
                pieces[row][3]->setPosition({row, 3});
            }
        }
    }

    // Handle pawn promotion
    if ((movingType == WPawn || movingType == BPawn) &&
        (move.end.row == 0 || move.end.row == 7)) {
        pieces[move.end.row][move.end.column] = std::make_unique<Queen>(movingColor, move.end, this);
    }

    // Set en passant target for double pawn push
    if (isDoublePawnPush) {
        enPassantTarget = {move.start.row + directionMultiplier, move.start.column};
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
    PieceID pieceType = pieces[piecePos.row][piecePos.column]->getType();

    // Add castling moves for the king (not part of raw attack moves)
    if (pieceType == WKing || pieceType == BKing) {
        if (canCastleKingSide(piecePos, pieceColor))
            rawMoves.push_back({piecePos.row, piecePos.column + 2});
        if (canCastleQueenSide(piecePos, pieceColor))
            rawMoves.push_back({piecePos.row, piecePos.column - 2});
    }

    for (auto& endPos : rawMoves) {
        Move trialMove = {piecePos, endPos};
        if (isLegalMove(trialMove)) {
            movesAvailable.push_back(endPos);
        }
    }
}

bool Board::isLegalMove(Move move) {
    SquareColor movingColor = pieces[move.start.row][move.start.column]->getColor();
    PieceID movingType = pieces[move.start.row][move.start.column]->getType();

    // Detect castling and track rook movement for simulation
    bool isCastling = false;
    int rookStartCol = -1, rookEndCol = -1;
    if ((movingType == WKing || movingType == BKing) && abs(move.end.column - move.start.column) == 2) {
        isCastling = true;
        if (move.end.column == 6) {
            rookStartCol = 7; rookEndCol = 5;
        } else if (move.end.column == 2) {
            rookStartCol = 0; rookEndCol = 3;
        }
    }

    int row = move.start.row;

    // Move king
    auto captured = std::move(pieces[move.end.row][move.end.column]);
    pieces[move.end.row][move.end.column] = std::move(pieces[move.start.row][move.start.column]);
    pieces[move.end.row][move.end.column]->setPosition(move.end);
    pieces[move.start.row][move.start.column] = nullptr;

    // Move rook during castling simulation
    std::unique_ptr<Piece> capturedRookSquare;
    if (isCastling) {
        capturedRookSquare = std::move(pieces[row][rookEndCol]);
        pieces[row][rookEndCol] = std::move(pieces[row][rookStartCol]);
        pieces[row][rookEndCol]->setPosition({row, rookEndCol});
    }

    bool inCheck = isInCheck(movingColor);

    // Undo rook
    if (isCastling) {
        pieces[row][rookStartCol] = std::move(pieces[row][rookEndCol]);
        pieces[row][rookStartCol]->setPosition({row, rookStartCol});
        pieces[row][rookEndCol] = std::move(capturedRookSquare);
    }

    // Undo king
    pieces[move.start.row][move.start.column] = std::move(pieces[move.end.row][move.end.column]);
    pieces[move.start.row][move.start.column]->setPosition(move.start);
    pieces[move.end.row][move.end.column] = std::move(captured);

    return !inCheck;
}

Pos Board::findKing(SquareColor color) {
    PieceID kingID = (color == White) ? WKing : BKing;
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            if (pieces[i][j] != nullptr && pieces[i][j]->getType() == kingID) {
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

Pos Board::getEnPassantTarget() const {
    return enPassantTarget;
}

bool Board::canCastleKingSide(Pos kingPos, SquareColor color) {
    int row = kingPos.row;
    // King must be in starting position
    if (kingPos.column != 4) return false;

    // King must not have moved
    if (pieces[row][4] == nullptr || pieces[row][4]->getHasMoved()) return false;

    // Rook must not have moved
    if (pieces[row][7] == nullptr || pieces[row][7]->getHasMoved()) return false;

    // Squares between must be empty
    if (!isEmpty({row, 5}) || !isEmpty({row, 6})) return false;

    // King must not be in check
    if (isInCheck(color)) return false;

    // King must not pass through check (squares f1, g1 for white)
    if (isSquareAttacked({row, 5}, (color == White) ? Black : White)) return false;
    if (isSquareAttacked({row, 6}, (color == White) ? Black : White)) return false;

    return true;
}

bool Board::isCheckmate(SquareColor color) {
    if (!isInCheck(color)) return false;
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            if (pieces[i][j] == nullptr) continue;
            if (getPieceColor(pieces[i][j]->getType()) != color) continue;
            std::vector<Pos> moves;
            getPiecesMoves({i, j}, moves);
            if (!moves.empty()) return false;
        }
    }
    return true;
}

bool Board::isStalemate(SquareColor color) {
    if (isInCheck(color)) return false;
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            if (pieces[i][j] == nullptr) continue;
            if (getPieceColor(pieces[i][j]->getType()) != color) continue;
            std::vector<Pos> moves;
            getPiecesMoves({i, j}, moves);
            if (!moves.empty()) return false;
        }
    }
    return true;
}

bool Board::canCastleQueenSide(Pos kingPos, SquareColor color) {
    int row = kingPos.row;
    // King must be in starting position
    if (kingPos.column != 4) return false;

    // King must not have moved
    if (pieces[row][4] == nullptr || pieces[row][4]->getHasMoved()) return false;

    // Rook must not have moved
    if (pieces[row][0] == nullptr || pieces[row][0]->getHasMoved()) return false;

    // Squares between must be empty (b1, c1, d1 for white)
    if (!isEmpty({row, 1}) || !isEmpty({row, 2}) || !isEmpty({row, 3})) return false;

    // King must not be in check
    if (isInCheck(color)) return false;

    // King must not pass through check (squares d1, c1 for white)
    if (isSquareAttacked({row, 3}, (color == White) ? Black : White)) return false;

    return true;
}
