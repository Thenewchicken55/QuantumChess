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
#include <random>
#include <chrono>

Board::Board(){
    std::random_device rd;
    rng.seed(rd());
    resetBoard();
}

// Returns a list of pieces on the Board. Stores position and type of each piece
std::vector<std::pair<Pos, PieceID>> Board::getPieces() {
    std::vector<std::pair<Pos, PieceID>> pieceList;
    for (int i = 0; i < 8; ++i){
        for (int j = 0; j < 8; ++j){
            if (pieces[i][j] != nullptr){
                // Skip superposition original position (rendered separately as ghost)
                if (superposition.active &&
                    i == superposition.originalPos.row &&
                    j == superposition.originalPos.column)
                    continue;
                pieceList.push_back(std::make_pair<Pos, PieceID>(pieces[i][j].get()->getPosition(), pieces[i][j].get()->getType()));
            }
        }
    }
    return pieceList;
}

std::vector<std::pair<Pos, PieceID>> Board::getAllVisiblePieces() {
    std::vector<std::pair<Pos, PieceID>> pieceList = getPieces();
    if (superposition.active) {
        for (auto& p : superposition.positions)
            pieceList.push_back({p, superposition.pieceType});
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
    superposition.active = false;
    for (int i = 0; i < 8; ++i){
        for (int j = 0; j < 8; ++j){
            SquareColor color = (i == 7 || i == 6) ? Black : White;
            if (i == 1 || i==6){
                pieces[i][j] = std::make_unique<Pawn>(color, Pos{i, j}, this);
            }
            else if (i == 0 || i == 7){
                if (j == 0 || j == 7)
                    pieces[i][j] = std::make_unique<Rook>(color, Pos{i, j}, this);
                else if (j == 1 || j == 6)
                    pieces[i][j] = std::make_unique<Knight>(color, Pos{i, j}, this);
                else if (j == 2 || j == 5)
                    pieces[i][j] = std::make_unique<Bishop>(color, Pos{i, j}, this);
                else if (j == 4)
                    pieces[i][j] = std::make_unique<King>(color, Pos{i, j}, this);
                else if (j == 3)
                    pieces[i][j] = std::make_unique<Queen>(color, Pos{i, j}, this);
            }
            else {
                pieces[i][j] = nullptr;
            }
        }
    }
}

void Board::movePiece(Move move, PieceID promoteTo) {
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
        move.end.row == enPassantTarget.row && move.end.column == enPassantTarget.column &&
        move.end.column != move.start.column) {
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
        pieces[capturedPawnPos.row][capturedPawnPos.column].reset();
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
        PieceID pt = promoteTo;
        if (pt == InvalidPiece) pt = (movingColor == White) ? WQueen : BQueen;
        SquareColor pc = getPieceColor(pt);
        switch (pt) {
            case WQueen:  case BQueen:  pieces[move.end.row][move.end.column] = std::make_unique<Queen>(pc,  move.end, this); break;
            case WRook:   case BRook:   pieces[move.end.row][move.end.column] = std::make_unique<Rook>(pc,   move.end, this); break;
            case WBishop: case BBishop: pieces[move.end.row][move.end.column] = std::make_unique<Bishop>(pc, move.end, this); break;
            case WKnight: case BKnight: pieces[move.end.row][move.end.column] = std::make_unique<Knight>(pc, move.end, this); break;
            default: pieces[move.end.row][move.end.column] = std::make_unique<Queen>(movingColor, move.end, this); break;
        }
    }

    // Set en passant target for double pawn push
    if (isDoublePawnPush) {
        enPassantTarget = {move.start.row + directionMultiplier, move.start.column};
    }
}

PieceID Board::getPieceID(Pos square) const {
    if (square.row < 0 || square.row >= 8 || square.column < 0 || square.column >= 8)
        return InvalidPiece;
    if (pieces[square.row][square.column] != nullptr) {
        if (superposition.active &&
            square.row == superposition.originalPos.row &&
            square.column == superposition.originalPos.column)
            return InvalidPiece;
        return pieces[square.row][square.column]->getType();
    }
    if (superposition.active) {
        for (auto& p : superposition.positions)
            if (square.row == p.row && square.column == p.column)
                return superposition.pieceType;
    }
    return InvalidPiece;
}

bool Board::isEmpty(Pos pos) const {
    if (pos.row < 0 || pos.row >= 8 || pos.column < 0 || pos.column >= 8)
        return false;
    if (pieces[pos.row][pos.column] != nullptr) {
        if (superposition.active &&
            pos.row == superposition.originalPos.row &&
            pos.column == superposition.originalPos.column)
            return true;
        return false;
    }
    if (superposition.active) {
        for (auto& p : superposition.positions)
            if (pos.row == p.row && pos.column == p.column)
                return false;
    }
    return true;
}

std::vector<Pos> Board::getRawPieceMoves(Pos piecePos) {
    if (!piecePos.isValid()) return {};
    if (pieces[piecePos.row][piecePos.column] == nullptr) return {};
    return pieces[piecePos.row][piecePos.column]->getValidMoves();
}

void Board::getPiecesMoves(Pos piecePos, std::vector<Pos>& movesAvailable){
    movesAvailable.clear();
    if (!piecePos.isValid()) return;
    if (pieces[piecePos.row][piecePos.column] == nullptr) return;
    std::vector<Pos> rawMoves = getRawPieceMoves(piecePos);
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
    int row = move.start.row;

    // Detect castling
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

    // Detect en passant
    bool isEnPassant = false;
    Pos epCapturedPos = {-1, -1};
    if ((movingType == WPawn || movingType == BPawn) &&
        move.end.row == enPassantTarget.row && move.end.column == enPassantTarget.column) {
        isEnPassant = true;
        epCapturedPos = {move.start.row, move.end.column};
    }

    // Move piece
    auto captured = std::move(pieces[move.end.row][move.end.column]);
    pieces[move.end.row][move.end.column] = std::move(pieces[move.start.row][move.start.column]);
    pieces[move.end.row][move.end.column]->setPosition(move.end);
    pieces[move.start.row][move.start.column] = nullptr;

    // Remove en passant captured pawn in simulation
    std::unique_ptr<Piece> savedEnPassantCapture;
    if (isEnPassant) {
        savedEnPassantCapture = std::move(pieces[epCapturedPos.row][epCapturedPos.column]);
    }

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

    // Restore en passant captured pawn
    if (isEnPassant) {
        pieces[epCapturedPos.row][epCapturedPos.column] = std::move(savedEnPassantCapture);
    }

    // Undo piece move
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

            // Use getAttackSquares() so pawns only "attack" diagonally — this
            // matters for castling-through-check and king-into-check detection
            // when the transit square is empty.
            std::vector<Pos> attacks = pieces[i][j]->getAttackSquares();
            for (auto& atk : attacks) {
                if (atk.row == square.row && atk.column == square.column) {
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
    PieceID kingID  = (color == White) ? WKing  : BKing;
    PieceID rookID  = (color == White) ? WRook  : BRook;

    // King must be in starting position
    if (kingPos.column != 4) return false;

    // King must be present, unmoved, and actually be a king of the right color
    if (pieces[row][4] == nullptr || pieces[row][4]->getHasMoved()) return false;
    if (pieces[row][4]->getType() != kingID) return false;

    // Rook must be present, unmoved, and actually be a rook of the right color
    if (pieces[row][7] == nullptr || pieces[row][7]->getHasMoved()) return false;
    if (pieces[row][7]->getType() != rookID) return false;

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
    PieceID kingID  = (color == White) ? WKing  : BKing;
    PieceID rookID  = (color == White) ? WRook  : BRook;

    // King must be in starting position
    if (kingPos.column != 4) return false;

    // King must be present, unmoved, and actually be a king of the right color
    if (pieces[row][4] == nullptr || pieces[row][4]->getHasMoved()) return false;
    if (pieces[row][4]->getType() != kingID) return false;

    // Rook must be present, unmoved, and actually be a rook of the right color
    if (pieces[row][0] == nullptr || pieces[row][0]->getHasMoved()) return false;
    if (pieces[row][0]->getType() != rookID) return false;

    // Squares between must be empty (b1, c1, d1 for white)
    if (!isEmpty({row, 1}) || !isEmpty({row, 2}) || !isEmpty({row, 3})) return false;

    // King must not be in check
    if (isInCheck(color)) return false;

    // King must not pass through check (squares d1, c1 for white)
    if (isSquareAttacked({row, 3}, (color == White) ? Black : White)) return false;

    return true;
}

void Board::enterSuperposition(Pos original, const std::vector<Pos>& dests, PieceID type, SquareColor color) {
    superposition.active = true;
    superposition.originalPos = original;
    superposition.positions = dests;
    superposition.pieceType = type;
    superposition.color = color;
}

void Board::addSuperpositionPositions(const std::vector<Pos>& newDests) {
    if (!superposition.active) return;
    for (auto& p : newDests) {
        bool dup = false;
        for (auto& existing : superposition.positions)
            if (existing.row == p.row && existing.column == p.column) { dup = true; break; }
        if (!dup)
            superposition.positions.push_back(p);
    }
}

void Board::collapseToPosition(Pos pos) {
    if (!superposition.active) return;
    Move move = {superposition.originalPos, pos};
    movePiece(move);
    superposition.active = false;
}

int Board::getSuperpositionProbability() const {
    if (!superposition.active || superposition.positions.empty()) return 0;
    return static_cast<int>(std::lround(100.0 / static_cast<double>(superposition.positions.size())));
}

float Board::getSuperpositionProbabilityF() const {
    if (!superposition.active || superposition.positions.empty()) return 0.0f;
    return 100.0f / static_cast<float>(superposition.positions.size());
}

Pos Board::collapseSuperposition() {
    if (!superposition.active || superposition.positions.empty()) return {-1, -1};

    std::uniform_int_distribution<int> dist(0, static_cast<int>(superposition.positions.size()) - 1);
    Pos chosen = superposition.positions[static_cast<size_t>(dist(rng))];

    Move move = {superposition.originalPos, chosen};
    movePiece(move);

    superposition.active = false;
    return chosen;
}

bool Board::hasActiveSuperposition() const {
    return superposition.active;
}

const SuperpositionState& Board::getSuperposition() const {
    return superposition;
}

void Board::clearSuperposition() {
    superposition.active = false;
    superposition.positions.clear();
}

bool Board::isSuperpositionSquare(Pos pos) const {
    if (!superposition.active) return false;
    for (auto& p : superposition.positions)
        if (pos.row == p.row && pos.column == p.column) return true;
    return false;
}

bool Board::isSuperpositionOriginal(Pos pos) const {
    if (!superposition.active) return false;
    return pos.row == superposition.originalPos.row && pos.column == superposition.originalPos.column;
}
