#include "window.h"
#include "board.h"
#include "raymath.h"
#include "raylib.h"
#include <iostream>
#include <random>
#include <algorithm>
#include <chrono>
#include <string>

static bool isInside(Vector2 pos, Rectangle r) {
    return pos.x >= r.x && pos.x <= r.x + r.width &&
           pos.y >= r.y && pos.y <= r.y + r.height;
}

Window::Window()
    : rng(std::chrono::steady_clock::now().time_since_epoch().count()) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "Quantum Chess");
    SetTargetFPS(30);
    updateBoard();
    loadSprites();
}

Window::~Window(){
    for (auto& cur : sprites){
        if (cur.id != 0)
            UnloadTexture(cur);
    }
    CloseWindow();
}

void Window::renderTitleScreen() {
    int w = GetScreenWidth();
    int h = GetScreenHeight();

    DrawText("Quantum Chess", w/2 - MeasureText("Quantum Chess", 60)/2, h/4, 60, DARKGRAY);

    Rectangle btn1 = {(float)(w/2 - 120), (float)(h/2 - 30), 240, 50};
    Rectangle btn2 = {(float)(w/2 - 120), (float)(h/2 + 50), 240, 50};

    DrawRectangleRec(btn1, DARKGRAY);
    DrawRectangleRec(btn2, DARKGRAY);
    DrawText("Local Game", btn1.x + 60, btn1.y + 12, 24, WHITE);
    DrawText("Hot-Seat", btn2.x + 68, btn2.y + 12, 24, WHITE);
}

void Window::renderSetupScreen() {
    int w = GetScreenWidth();
    int h = GetScreenHeight();

    DrawText("Player Setup", w/2 - MeasureText("Player Setup", 50)/2, h/4 - 40, 50, DARKGRAY);

    int labelX = w/2 - 200;
    int fieldX = w/2 - 80;
    int y1 = h/2 - 40;
    int y2 = h/2 + 20;

    DrawText("White:", labelX, y1, 24, DARKGRAY);
    DrawText("Black:", labelX, y2, 24, DARKGRAY);

    Color col1 = (nameInputActive == 0) ? LIGHTGRAY : WHITE;
    Color col2 = (nameInputActive == 1) ? LIGHTGRAY : WHITE;
    DrawRectangle(fieldX, y1 - 2, 200, 30, col1);
    DrawRectangle(fieldX, y2 - 2, 200, 30, col2);
    DrawRectangleLines(fieldX, y1 - 2, 200, 30, DARKGRAY);
    DrawRectangleLines(fieldX, y2 - 2, 200, 30, DARKGRAY);

    DrawText(playerNameWhite.c_str(), fieldX + 4, y1, 22, BLACK);
    DrawText(playerNameBlack.c_str(), fieldX + 4, y2, 22, BLACK);

    Rectangle startBtn = {(float)(w/2 - 100), (float)(h/2 + 80), 200, 40};
    DrawRectangleRec(startBtn, DARKGRAY);
    DrawText("Start Game", startBtn.x + 30, startBtn.y + 8, 22, WHITE);

    DrawText("Click a name field, then type on keyboard", w/2 - 200, h/2 + 140, 16, GRAY);
}

void Window::renderPassOverlay() {
    float o = Fade(BLACK, 0.5f);
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), o);

    std::string msg;
    if (gameMode == MODE_HOTSEAT) {
        msg = "Pass to " + ((currentPlayer == White) ? playerNameWhite : playerNameBlack);
    } else {
        msg = (currentPlayer == White) ? "White's Turn" : "Black's Turn";
    }
    msg += " — Click anywhere";

    int w = GetScreenWidth();
    int h = GetScreenHeight();
    DrawText(msg.c_str(), w/2 - MeasureText(msg.c_str(), 36)/2, h/2 - 18, 36, WHITE);
}

void Window::renderGame() {
    BeginDrawing();
        ClearBackground(RAYWHITE);
        drawBoard();

        if (lastMove.start.row != -1) {
            highlightSquare(lastMove.start, Fade(YELLOW, 0.3f));
            highlightSquare(lastMove.end, Fade(YELLOW, 0.3f));
        }

        auto pieces = game.getPieces();
        for (auto cur : pieces)
            drawPiece(cur.second, getSquarePosition(cur.first));

        drawSuperpositionGhosts();

        highlightSquare(getSquare(GetMousePosition()));
        highlightMovesSelected();
        highlightCheckedKing();

        if (gameOver) {
            DrawText(gameOverMessage.c_str(), 20, 20, 40, DARKGRAY);
            DrawText("Press R to restart", 20, 70, 30, DARKGRAY);
        } else {
            std::string turnText;
            if (gameState == quantumPickFirst || gameState == quantumPickDest1 ||
                gameState == quantumPickSecond || gameState == quantumPickDest2)
                turnText = (currentPlayer == White ? playerNameWhite : playerNameBlack) + " (Quantum)";
            else if (gameState == opponentPickPiece || gameState == opponentPickDest)
                turnText = (currentPlayer == White ? playerNameWhite : playerNameBlack) + " (Opponent)";

            DrawText(turnText.c_str(), 20, 20, 30, BLACK);

            if (game.isInCheck(White))
                DrawText("White in check", 20, 60, 25, RED);
            if (game.isInCheck(Black))
                DrawText("Black in check", 20, 90, 25, RED);
        }

        if (passClickRequired)
            renderPassOverlay();

    EndDrawing();
}

void Window::render() {
    if (currentScreen == SCREEN_TITLE) {
        BeginDrawing();
            ClearBackground(RAYWHITE);
            renderTitleScreen();
        EndDrawing();
    } else if (currentScreen == SCREEN_SETUP) {
        BeginDrawing();
            ClearBackground(RAYWHITE);
            renderSetupScreen();
        EndDrawing();
    } else {
        renderGame();
    }
}

void Window::handleTitleClick() {
    Vector2 mouse = GetMousePosition();
    int w = GetScreenWidth();
    int h = GetScreenHeight();

    Rectangle btn1 = {(float)(w/2 - 120), (float)(h/2 - 30), 240, 50};
    Rectangle btn2 = {(float)(w/2 - 120), (float)(h/2 + 50), 240, 50};

    audio.playButtonClick();

    if (isInside(mouse, btn1)) {
        gameMode = MODE_LOCAL;
        currentScreen = SCREEN_SETUP;
    } else if (isInside(mouse, btn2)) {
        gameMode = MODE_HOTSEAT;
        currentScreen = SCREEN_SETUP;
    }
}

void Window::handleSetupClick() {
    Vector2 mouse = GetMousePosition();
    int w = GetScreenWidth();
    int h = GetScreenHeight();

    int fieldX = w/2 - 80;
    int y1 = h/2 - 42;
    int y2 = h/2 + 18;

    audio.playButtonClick();

    if (isInside(mouse, {fieldX - 2 - 4, (float)(y1 - 2 - 4), 208, 38})) {
        nameInputActive = 0;
    } else if (isInside(mouse, {fieldX - 2 - 4, (float)(y2 - 2 - 4), 208, 38})) {
        nameInputActive = 1;
    }

    Rectangle startBtn = {(float)(w/2 - 100), (float)(h/2 + 80), 200, 40};
    if (isInside(mouse, startBtn)) {
        startGame();
    }
}

void Window::handleSetupKeyInput() {
    int key = GetCharPressed();
    while (key > 0) {
        if (key >= 32 && key <= 126) {
            std::string* name = (nameInputActive == 0) ? &playerNameWhite : &playerNameBlack;
            if (name->size() < 16)
                name->push_back((char)key);
        }
        key = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE)) {
        std::string* name = (nameInputActive == 0) ? &playerNameWhite : &playerNameBlack;
        if (!name->empty())
            name->pop_back();
    }
    if (IsKeyPressed(KEY_ENTER)) {
        startGame();
    }
}

void Window::startGame() {
    if (playerNameWhite.empty()) playerNameWhite = "White";
    if (playerNameBlack.empty()) playerNameBlack = "Black";
    game.resetBoard();
    gameState = quantumPickFirst;
    currentPlayer = White;
    gameOver = false;
    gameOverMessage = "";
    validMovePositions.clear();
    moves = {};
    lastMove = {{-1,-1}, {-1,-1}};
    captureMissed = false;
    passClickRequired = false;
    currentScreen = SCREEN_PLAYING;
}

void Window::pollEvents(){
    if(IsKeyPressed(KEY_ESCAPE)) {
        if (currentScreen == SCREEN_PLAYING) {
            currentScreen = SCREEN_TITLE;
        } else {
            CloseWindow();
        }
    }
    if (IsWindowResized())
        resizedWindow();

    if (currentScreen == SCREEN_TITLE) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            handleTitleClick();
    } else if (currentScreen == SCREEN_SETUP) {
        handleSetupKeyInput();
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            handleSetupClick();
    } else if (currentScreen == SCREEN_PLAYING) {
        if (IsKeyPressed(KEY_R))
            restartGame();
        if (passClickRequired) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                passClickRequired = false;
            return;
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            handleLeftMouseDown();
    }
}

void Window::handleLeftMouseDown(){
    if (gameOver || passClickRequired) return;

    auto squarePicked = getSquare(GetMousePosition());
    if (squarePicked.row == -1) return;

    switch (gameState) {

    case quantumPickFirst: {
        if (game.isEmpty(squarePicked) || getPieceColor(game.getPieceID(squarePicked)) != currentPlayer) {
            audio.playMoveInvalid();
            break;
        }
        game.getPiecesMoves(squarePicked, validMovePositions);
        if (validMovePositions.empty()) {
            audio.playMoveInvalid();
            validMovePositions.clear();
            break;
        }
        moves.m1.start = squarePicked;
        audio.playButtonClick();
        gameState = quantumPickDest1;
        break;
    }

    case quantumPickDest1: {
        if (!game.isEmpty(squarePicked) && getPieceColor(game.getPieceID(squarePicked)) == currentPlayer) {
            game.getPiecesMoves(squarePicked, validMovePositions);
            if (validMovePositions.empty()) {
                audio.playMoveInvalid();
                validMovePositions.clear();
                break;
            }
            moves.m1.start = squarePicked;
            audio.playButtonClick();
            break;
        }
        moves.m1.end = squarePicked;
        if (std::find(validMovePositions.begin(), validMovePositions.end(), moves.m1.end) == validMovePositions.end()) {
            audio.playMoveInvalid();
            break;
        }
        if (!game.isEmpty(squarePicked)) {
            audio.playMoveInvalid();
            break;
        }
        audio.playButtonClick();
        gameState = quantumPickSecond;
        break;
    }

    case quantumPickSecond: {
        if (game.isEmpty(squarePicked) || getPieceColor(game.getPieceID(squarePicked)) != currentPlayer) {
            audio.playMoveInvalid();
            break;
        }
        game.getPiecesMoves(squarePicked, validMovePositions);
        if (validMovePositions.empty()) {
            audio.playMoveInvalid();
            break;
        }
        moves.m2.start = squarePicked;
        audio.playButtonClick();
        gameState = quantumPickDest2;
        break;
    }

    case quantumPickDest2: {
        if (!game.isEmpty(squarePicked) && getPieceColor(game.getPieceID(squarePicked)) == currentPlayer) {
            game.getPiecesMoves(squarePicked, validMovePositions);
            if (validMovePositions.empty()) break;
            moves.m2.start = squarePicked;
            audio.playButtonClick();
            gameState = quantumPickDest2;
            break;
        }
        moves.m2.end = squarePicked;
        if (std::find(validMovePositions.begin(), validMovePositions.end(), moves.m2.end) == validMovePositions.end()) {
            audio.playMoveInvalid();
            break;
        }
        if (!game.isEmpty(squarePicked)) {
            audio.playMoveInvalid();
            break;
        }

        if (moves.m1 == moves.m2) {
            audio.playMoveInvalid();
            gameState = quantumPickSecond;
            break;
        }

        validMovePositions.clear();
        audio.playQuantumEnter();
        game.enterSuperposition(
            moves.m1.start, moves.m1.end, moves.m2.end,
            game.getPieceID(moves.m1.start), currentPlayer
        );

        SquareColor opponent = (currentPlayer == White) ? Black : White;
        currentPlayer = opponent;
        gameState = opponentPickPiece;

        if (gameMode == MODE_HOTSEAT)
            passClickRequired = true;
        break;
    }

    case opponentPickPiece: {
        if (game.isEmpty(squarePicked) || getPieceColor(game.getPieceID(squarePicked)) != currentPlayer) {
            audio.playMoveInvalid();
            break;
        }
        game.getPiecesMoves(squarePicked, validMovePositions);
        if (validMovePositions.empty()) {
            audio.playMoveInvalid();
            break;
        }
        moves.m1.start = squarePicked;
        audio.playButtonClick();
        gameState = opponentPickDest;
        break;
    }

    case opponentPickDest: {
        if (!game.isEmpty(squarePicked) && getPieceColor(game.getPieceID(squarePicked)) == currentPlayer) {
            game.getPiecesMoves(squarePicked, validMovePositions);
            if (validMovePositions.empty()) break;
            moves.m1.start = squarePicked;
            audio.playButtonClick();
            break;
        }
        moves.m1.end = squarePicked;
        if (std::find(validMovePositions.begin(), validMovePositions.end(), moves.m1.end) == validMovePositions.end()) {
            audio.playMoveInvalid();
            break;
        }

        validMovePositions.clear();

        if (game.isSuperpositionSquare(squarePicked)) {
            bool wasCapture = !game.isEmpty(squarePicked);
            Move attemptedMove = moves.m1;
            audio.playQuantumCollapse();
            Pos result = game.collapseSuperposition();

            if (result.row == attemptedMove.end.row && result.column == attemptedMove.end.column) {
                game.movePiece(attemptedMove);
                lastMove = attemptedMove;
                if (wasCapture) audio.playCapture();
                else audio.playMove();
            } else {
                audio.playMiss();
            }
            captureMissed = !(result.row == attemptedMove.end.row && result.column == attemptedMove.end.column);

            SquareColor nextPlayer = (currentPlayer == White) ? Black : White;
            currentPlayer = nextPlayer;

            bool wasCheck = game.isInCheck(currentPlayer);
            checkGameEnd();
            if (gameOver) {
                audio.playCheckmate();
            } else if (wasCheck) {
                audio.playCheck();
            }

            if (gameMode == MODE_HOTSEAT && !gameOver)
                passClickRequired = true;
            gameState = quantumPickFirst;
            break;
        }

        {
            bool wasCapture = !game.isEmpty(moves.m1.end);
            game.movePiece(moves.m1);
            lastMove = moves.m1;
            if (wasCapture) audio.playCapture();
            else audio.playMove();
        }

        SquareColor nextPlayer = (currentPlayer == White) ? Black : White;
        currentPlayer = nextPlayer;

        if (game.hasActiveSuperposition()) {
            audio.playQuantumCollapse();
            game.collapseSuperposition();
        }

        checkGameEnd();
        if (gameOver) {
            audio.playCheckmate();
        } else if (game.isInCheck(currentPlayer)) {
            audio.playCheck();
        }

        if (gameMode == MODE_HOTSEAT && !gameOver)
            passClickRequired = true;
        gameState = quantumPickFirst;
        break;
    }

    default:
        break;
    }
}

void Window::checkGameEnd() {
    if (game.isCheckmate(currentPlayer)) {
        std::string winner = (currentPlayer == White) ? playerNameBlack : playerNameWhite;
        gameOverMessage = "Checkmate! " + winner + " wins!";
        gameOver = true;
    } else if (game.isStalemate(currentPlayer)) {
        gameOverMessage = "Stalemate! It's a draw!";
        gameOver = true;
    }
}

void Window::restartGame() {
    game.resetBoard();
    gameState = quantumPickFirst;
    currentPlayer = White;
    gameOver = false;
    gameOverMessage = "";
    validMovePositions.clear();
    moves = {};
    lastMove = {{-1,-1}, {-1,-1}};
    captureMissed = false;
    passClickRequired = false;
    currentScreen = SCREEN_SETUP;
}

void Window::drawSuperpositionGhosts() {
    if (!game.hasActiveSuperposition()) return;

    const auto& sup = game.getSuperposition();

    float squareSize = boardWidth / 8;

    drawPiece(sup.pieceType, getSquarePosition(sup.originalPos), 0.3f);

    Vector2 p1 = getSquarePosition(sup.pos1);
    Vector2 p2 = getSquarePosition(sup.pos2);
    p1.x += squareSize / 2;
    p1.y += squareSize / 2;
    p2.x += squareSize / 2;
    p2.y += squareSize / 2;
    DrawLineEx(p1, p2, 3.0f, Fade(PURPLE, 0.5f));

    highlightSquare(sup.pos1, Fade(PURPLE, 0.2f));
    highlightSquare(sup.pos2, Fade(PURPLE, 0.2f));

    drawPiece(sup.pieceType, getSquarePosition(sup.pos1), 0.4f);
    drawPiece(sup.pieceType, getSquarePosition(sup.pos2), 0.4f);
}

void Window::run(){
    while(!WindowShouldClose()){
        pollEvents();
        render();
    }
}

Pos Window::getSquare(Vector2 cursorPosition){
    if (cursorPosition.x < boardStart.x || cursorPosition.x > boardEnd.x ||
        cursorPosition.y < boardStart.y || cursorPosition.y > boardEnd.y)
        return {-1, -1};

    float squareSize = boardWidth / 8;
    int column = (cursorPosition.x - boardStart.x) / squareSize;
    int row = (cursorPosition.y - boardStart.y) / squareSize;
    return {row, column};
}

Vector2 Window::getSquarePosition(Pos square){
    float squareSize = boardWidth / 8;
    return {
        boardStart.x + square.column * squareSize,
        boardStart.y + square.row * squareSize
    };
}

void Window::resizedWindow(){
    screenWidth = GetScreenWidth();
    screenHeight = GetScreenHeight();
    updateBoard();
}

void Window::updateBoard() {
    int w = GetScreenWidth();
    int h = GetScreenHeight();
    boardWidth = ((w < h) ? w : h) * 0.95;
    boardStart = {(float)(w - boardWidth) / 2, (float)(h - boardWidth) / 2};
    boardEnd = {boardStart.x + boardWidth, boardStart.y + boardWidth};
}

void Window::drawPiece(int pieceKey, Vector2 pos, float alpha){
    float squareSize = boardWidth / 8;
    Rectangle destination = {pos.x, pos.y, squareSize, squareSize};
    Color tint = WHITE;
    tint.a = static_cast<unsigned char>(alpha * 255);
    DrawTexturePro(
        sprites[pieceKey],
        {0, 0, spriteWidth, spriteHeight},
        destination,
        {0, 0},
        0.0f,
        tint
    );
}

void Window::highlightSquare(Pos pos, Color color){
    if (pos.row != -1 && pos.column != -1) {
        Vector2 squarePos = getSquarePosition(pos);
        float squareSize = boardWidth / 8;
        DrawRectangleV(squarePos, {squareSize, squareSize}, Fade(color, 0.3f));
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
