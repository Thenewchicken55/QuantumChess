/*
    window.cpp

    window class for QuantumChess Project

    ZipCode
*/

#include "window.h"
#include "board.h"
#include "raymath.h"
#include "raylib.h"
#include <iostream>
#include <random>
#include <algorithm>
#include <chrono>

/*
    Initializer for window class
*/
Window::Window()
    : rng(std::chrono::steady_clock::now().time_since_epoch().count()) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "Quantum Chess");

    SetTargetFPS(30);

    createBoardTexture();
    loadSprites();
}

/*
    Destructor for Window class
*/
Window::~Window(){
    if(board.texture.id != 0)
        UnloadRenderTexture(board);

    for (auto& cur : sprites){
        if (cur.id != 0)
            UnloadTexture(cur);
    }

    CloseWindow();
}

/*
    Render window
*/
void Window::render(){
    BeginDrawing();

        ClearBackground(RAYWHITE);

        DrawTextureV(board.texture, { 0.0F, 0.0F }, WHITE);

        // Draw last move highlight
        if (lastMove.start.row != -1) {
            highlightSquare(lastMove.start, ColorAlpha(YELLOW, 0.3f));
            highlightSquare(lastMove.end, ColorAlpha(YELLOW, 0.3f));
        }

        auto pieces = game.getPieces();
        for (auto cur : pieces){
            drawPiece(cur.second, getSquarePosition(cur.first));
        }

        // highlighting cursor square and valid move targets
        highlightSquare(getSquare(GetMousePosition()));
        highlightMovesSelected();
        highlightCheckedKing();

        if (gameOver) {
            DrawText(gameOverMessage.c_str(), 20, 20, 40, DARKGRAY);
            DrawText("Press R to restart", 20, 70, 30, DARKGRAY);
        } else {
            std::string playerTurnString;
            if (currentPlayer == White)
                playerTurnString = "White's Turn";
            else
                playerTurnString = "Black's Turn";
            DrawText(playerTurnString.c_str(), 20, 20, 40, BLACK);

            SquareColor opponent = (currentPlayer == White) ? Black : White;
            if (game.isInCheck(opponent)) {
                DrawText("Check!", 20, 70, 30, RED);
            }
        }

    EndDrawing();
}

void Window::highlightCheckedKing() {
    SquareColor opponent = (currentPlayer == White) ? Black : White;
    if (game.isInCheck(opponent)) {
        Pos kingPos = game.findKing(opponent);
        if (kingPos.row != -1) {
            highlightSquare(kingPos, RED);
        }
    }
}

void Window::highlightMovesSelected(){

    if (gameState == pickSquareFirst) {
        for (auto& cur : validMovePositions){
            highlightSquare(cur, RED);
        }
    } else if (gameState == pickSquareSecond) {
        for (auto& cur : validMovePositions){
            highlightSquare(cur, RED);
        }
    }

    switch (gameState)
    {
    case pickSquareSecond:
        highlightSquare(moves.m2.start, PURPLE);
    case pickPieceSecond:
        highlightSquare(moves.m1.end, PURPLE);
    case pickSquareFirst:
        highlightSquare(moves.m1.start, PURPLE);
    default:
        break;
    }
}

/*
    poll events for
*/
void Window::pollEvents(){
    if(IsKeyPressed(KEY_ESCAPE)) {
        CloseWindow();
    }
    if (IsWindowResized()){
        resizedWindow();
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        handleLeftMouseDown();
    }
    if (IsKeyPressed(KEY_R)) {
        restartGame();
    }

}

void Window::restartGame() {
    game.resetBoard();
    gameState = pickPieceFirst;
    currentPlayer = White;
    gameOver = false;
    gameOverMessage = "";
    validMovePositions.clear();
    moves = {};
    lastMove = {{-1,-1}, {-1,-1}};
}

void Window::handleLeftMouseDown(){
    if (gameOver) return;

    auto squarePicked = getSquare(GetMousePosition());
    if (squarePicked.row == -1) return;

    switch (gameState)
    {
    case pickPieceFirst:
        if (game.isEmpty(squarePicked) || getPieceColor(game.getPieceID(squarePicked)) != currentPlayer){
            break;
        }
        game.getPiecesMoves(squarePicked, validMovePositions);
        moves.m1.start = squarePicked;
        gameState = pickSquareFirst;
        break;

    case pickSquareFirst:
        // Allow re-selecting a different own piece
        if (!game.isEmpty(squarePicked) && getPieceColor(game.getPieceID(squarePicked)) == currentPlayer) {
            game.getPiecesMoves(squarePicked, validMovePositions);
            moves.m1.start = squarePicked;
            break;
        }
        moves.m1.end = squarePicked;
        if (std::find(validMovePositions.begin(), validMovePositions.end(), moves.m1.end) == validMovePositions.end())
            break;
        gameState = pickPieceSecond;
        break;

    case pickPieceSecond:
        if (game.isEmpty(squarePicked) || getPieceColor(game.getPieceID(squarePicked)) != currentPlayer){
            break;
        }
        game.getPiecesMoves(squarePicked, validMovePositions);
        if (validMovePositions.empty()) break;
        moves.m2.start = squarePicked;
        gameState = pickSquareSecond;
        break;

    case pickSquareSecond:
        moves.m2.end = squarePicked;
        if (std::find(validMovePositions.begin(), validMovePositions.end(), moves.m2.end) == validMovePositions.end())
            break;

        gameState = pickPieceFirst;
        if (moves.m1 == moves.m2){
            gameState = pickPieceSecond;
            break;
        }

        std::uniform_int_distribution<int> dist(0, 1);
        if (dist(rng) == 0) {
            game.movePiece(moves.m1);
            lastMove = moves.m1;
        } else {
            game.movePiece(moves.m2);
            lastMove = moves.m2;
        }

        if (currentPlayer == White)
            currentPlayer = Black;
        else
            currentPlayer = White;

        // Check for game over
        if (game.isCheckmate(currentPlayer)) {
            std::string winner = (currentPlayer == White) ? "Black" : "White";
            gameOverMessage = "Checkmate! " + winner + " wins!";
            gameOver = true;
        } else if (game.isStalemate(currentPlayer)) {
            gameOverMessage = "Stalemate! It's a draw!";
            gameOver = true;
        }

        validMovePositions.clear();
        break;

    default:
        break;
    }
}

/*
    Main loop for program
*/
void Window::run(){
    while(!WindowShouldClose()){
        pollEvents();
        render();
    }
}

/*
    Get corresponding square by cursor position

    @param cursorPosition 2d vector representing cursor position
    @return index of cell clicked
*/
Pos Window::getSquare(Vector2 cursorPosition){
    int squareSize = boardWidth/8;

    // Check if cursor is within the board
    if (cursorPosition.x < boardStart.x || cursorPosition.x > boardEnd.x || cursorPosition.y < boardStart.y || cursorPosition.y > boardEnd.y){
        return {-1, -1};
    }

    // Get the square
    int cursorSquareY = (cursorPosition.y - boardStart.y) / squareSize;
    int cursorSquareX = (cursorPosition.x - boardStart.x) / squareSize;

    return {cursorSquareY, cursorSquareX};
}

/*
    get the position in the window of a square by index
*/
Vector2 Window::getSquarePosition(Pos square){
    return {
        ( square.column) * boardWidth/8 + boardStart.x,
        ( square.row) * boardWidth/8 + boardStart.y
    };
}

/*
    Handle resizing of the window
*/
void Window::resizedWindow(){
    screenWidth = GetScreenWidth();
    screenHeight = GetScreenHeight();
    createBoardTexture();
}

/*
    Creates board texture
*/
void Window::createBoardTexture(){
    if (board.texture.id != 0)
        UnloadRenderTexture(board);

    int w = GetScreenWidth();
    int h = GetScreenHeight();
    board = LoadRenderTexture(w, h);

    boardWidth = ((w < h) ? w : h) * 0.95;
    boardStart = {(float)(w - boardWidth) / 2, (float)(h - boardWidth) / 2};
    boardEnd = {boardStart.x + boardWidth, boardStart.y + boardWidth};

    BeginTextureMode(board);
        ClearBackground(RAYWHITE);
        float squareSize = boardWidth/8;
        for(int j = 0; j < 8; ++j){
            for (int k = 0; k < 8; ++k){
                DrawRectangleV(
                    Vector2Add(boardStart, {j*squareSize, k*squareSize}),
                    {squareSize, squareSize},
                    ((j + k) % 2 == 0) ? BROWN : BEIGE
                );
            }
        }
    EndTextureMode();
}

/*
    Draws a specific piece on the board

    @param[in] pieceKey key for the corresponding piece
*/
void Window::drawPiece(int pieceKey, Vector2 pos, bool center){
    Vector2 origin = (center ? (Vector2){boardWidth/16, boardWidth/16} : (Vector2){0, 0});
    Rectangle destination = (Rectangle){pos.x, pos.y, boardWidth/8, boardWidth/8};

    DrawTexturePro(
        sprites[pieceKey],
        (Rectangle){0, 0, spriteWidth, spriteHeight},
        destination,
        origin,
        0.0f,
        WHITE
    );
}

void Window::highlightSquare(Pos pos, Color color){
    if (pos.row != -1 && pos.column != -1) {
        Vector2 squarePos = getSquarePosition(pos);
        DrawRectangleV(squarePos, {boardWidth/8, boardWidth/8}, Fade(color, 0.3f));
    }
}

void Window::loadSprites(){
    sprites.resize(12);

    sprites[WPawn] = LoadTexture("../assets/wp.png");
    sprites[WKnight] = LoadTexture("../assets/wn.png");
    sprites[WBishop] = LoadTexture("../assets/wb.png");
    sprites[WRook] = LoadTexture("../assets/wr.png");
    sprites[WQueen] = LoadTexture("../assets/wq.png");
    sprites[WKing] = LoadTexture("../assets/wk.png");
    sprites[BPawn] = LoadTexture("../assets/bp.png");
    sprites[BKnight] = LoadTexture("../assets/bn.png");
    sprites[BBishop] = LoadTexture("../assets/bb.png");
    sprites[BRook] = LoadTexture("../assets/br.png");
    sprites[BQueen] = LoadTexture("../assets/bq.png");
    sprites[BKing] = LoadTexture("../assets/bk.png");
}

void Window::updateBoard() {
    render();
}
