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
    net.disconnect();
    for (auto& cur : sprites){
        if (cur.id != 0) UnloadTexture(cur);
    }
    CloseWindow();
}

bool Window::isNetworkGame() const {
    return gameMode == MODE_NETWORK_HOST || gameMode == MODE_NETWORK_CLIENT;
}

void Window::drawBoard() {
    float squareSize = boardWidth / 8;
    for (int col = 0; col < 8; ++col) {
        for (int row = 0; row < 8; ++row) {
            Vector2 pos = {boardStart.x + col * squareSize, boardStart.y + row * squareSize};
            Color shade = ((row + col) % 2 == 0) ? Color{181, 136, 99, 255} : Color{240, 217, 181, 255};
            DrawRectangleV(pos, {squareSize, squareSize}, shade);
        }
    }
}

void Window::highlightCheckedKing() {
    if (game.isInCheck(White)) {
        Pos kingPos = game.findKing(White);
        if (kingPos.row != -1) highlightSquare(kingPos, RED);
    }
    if (game.isInCheck(Black)) {
        Pos kingPos = game.findKing(Black);
        if (kingPos.row != -1) highlightSquare(kingPos, RED);
    }
}

void Window::highlightMovesSelected(){
    if (gameState == quantumPickDest1 || gameState == quantumPickDest2 || gameState == opponentPickDest) {
        for (auto& cur : validMovePositions)
            highlightSquare(cur, RED);
    }

    switch (gameState) {
    case quantumPickDest2:
        highlightSquare(moves.m2.start, PURPLE);
    case quantumPickSecond:
        highlightSquare(moves.m1.end, PURPLE);
    case opponentPickDest:
    case quantumPickDest1:
        highlightSquare(moves.m1.start, PURPLE);
    default:
        break;
    }
}

void Window::renderTitleScreen() {
    int w = GetScreenWidth();
    int h = GetScreenHeight();

    DrawText("Quantum Chess", w/2 - MeasureText("Quantum Chess", 60)/2, h/4, 60, DARKGRAY);

    Rectangle btn1 = {(float)(w/2 - 120), (float)(h/2 - 80), 240, 50};
    Rectangle btn2 = {(float)(w/2 - 120), (float)(h/2 - 10), 240, 50};
    Rectangle btn3 = {(float)(w/2 - 120), (float)(h/2 + 60), 240, 50};
    Rectangle btn4 = {(float)(w/2 - 120), (float)(h/2 + 130), 240, 50};

    DrawRectangleRec(btn1, DARKGRAY);
    DrawText("Local Game", btn1.x + 60, btn1.y + 12, 24, WHITE);

    DrawRectangleRec(btn2, DARKGRAY);
    DrawText("Hot-Seat", btn2.x + 68, btn2.y + 12, 24, WHITE);

    DrawRectangleRec(btn3, DARKGRAY);
    DrawText("Host Game", btn3.x + 60, btn3.y + 12, 24, WHITE);

    DrawRectangleRec(btn4, (net.getRole() == NetworkRole::None) ? DARKGRAY : GRAY);
    DrawText("Join Game", btn4.x + 60, btn4.y + 12, 24, WHITE);

    if (net.getRole() == NetworkRole::Host) {
        DrawText("Hosting on port 5555... waiting for opponent", w/2 - 240, h/2 + 200, 18, DARKGRAY);
        if (net.isConnected())
            DrawText("Opponent connected!", w/2 - 100, h/2 + 230, 20, GREEN);
    }

    if (!networkStatus.empty())
        DrawText(networkStatus.c_str(), w/2 - MeasureText(networkStatus.c_str(), 20)/2, h/2 + 260, 20, RED);
}

void Window::renderSetupScreen() {
    int w = GetScreenWidth();
    int h = GetScreenHeight();

    const char* title = (gameMode == MODE_NETWORK_HOST || gameMode == MODE_NETWORK_CLIENT)
        ? "Network Setup" : "Player Setup";
    DrawText(title, w/2 - MeasureText(title, 50)/2, h/4 - 40, 50, DARKGRAY);

    int labelX = w/2 - 200;
    int fieldX = w/2 - 80;
    int y1 = h/2 - 40;
    int y2 = h/2 + 20;

    if (gameMode == MODE_NETWORK_CLIENT) {
        DrawText("Host IP:", labelX, y1, 24, DARKGRAY);
        DrawText("Your Name:", labelX, y2, 24, DARKGRAY);

        Rectangle ipRect = {(float)(fieldX - 4), (float)(y1 - 6), 208, 38};
        Color ipCol = (ipInputActive == 0) ? LIGHTGRAY : WHITE;
        DrawRectangleRec(ipRect, ipCol);
        DrawRectangleLines(ipRect.x, ipRect.y, ipRect.width, ipRect.height, DARKGRAY);
        DrawText(ipInput.c_str(), fieldX, y1, 22, BLACK);

        Rectangle nameRect = {(float)(fieldX - 4), (float)(y2 - 6), 208, 38};
        Color nmCol = (ipInputActive == 1) ? LIGHTGRAY : WHITE;
        DrawRectangleRec(nameRect, nmCol);
        DrawRectangleLines(nameRect.x, nameRect.y, nameRect.width, nameRect.height, DARKGRAY);
        DrawText(playerNameWhite.c_str(), fieldX, y2, 22, BLACK);

        Rectangle joinBtn = {(float)(w/2 - 100), (float)(h/2 + 80), 200, 40};
        DrawRectangleRec(joinBtn, DARKGRAY);
        DrawText("Connect", joinBtn.x + 50, joinBtn.y + 8, 22, WHITE);

        if (!networkStatus.empty())
            DrawText(networkStatus.c_str(), w/2 - MeasureText(networkStatus.c_str(), 18)/2, h/2 + 140, 18, RED);
    } else if (gameMode == MODE_NETWORK_HOST) {
        DrawText("Your Name (White):", labelX, h/2 - 10, 24, DARKGRAY);
        Rectangle nameRect = {(float)(fieldX - 4), (float)(h/2 - 14), 208, 38};
        Color nmCol = LIGHTGRAY;
        DrawRectangleRec(nameRect, nmCol);
        DrawRectangleLines(nameRect.x, nameRect.y, nameRect.width, nameRect.height, DARKGRAY);
        DrawText(playerNameWhite.c_str(), fieldX, h/2 + 2, 22, BLACK);

        if (!networkStatus.empty())
            DrawText(networkStatus.c_str(), w/2 - MeasureText(networkStatus.c_str(), 18)/2, h/2 + 60, 18, RED);
    } else {
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

    DrawText("Press ESC to go back", w/2 - 90, h - 40, 18, GRAY);
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
            if (!isNetworkGame())
                DrawText("Press R to restart", 20, 70, 30, DARKGRAY);
            else
                DrawText("Press ESC to quit", 20, 70, 30, DARKGRAY);
        } else if (waitingForOpponent) {
            DrawText("Waiting for opponent...", 20, 20, 30, DARKGRAY);
        } else {
            std::string turnText;
            if (isNetworkGame()) {
                turnText = (currentPlayer == White ? playerNameWhite : playerNameBlack) + "'s Turn";
            } else if (gameState == quantumPickFirst || gameState == quantumPickDest1 ||
                gameState == quantumPickSecond || gameState == quantumPickDest2) {
                turnText = (currentPlayer == White ? playerNameWhite : playerNameBlack) + " (Quantum)";
            } else if (gameState == opponentPickPiece || gameState == opponentPickDest) {
                turnText = (currentPlayer == White ? playerNameWhite : playerNameBlack) + " (Opponent)";
            }

            DrawText(turnText.c_str(), 20, 20, 30, BLACK);

            if (game.isInCheck(White))
                DrawText("White in check", 20, 60, 25, RED);
            if (game.isInCheck(Black))
                DrawText("Black in check", 20, 90, 25, RED);
        }

        if (passClickRequired && !isNetworkGame())
            renderPassOverlay();

    EndDrawing();
}

void Window::renderPassOverlay() {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.5f));

    std::string msg;
    if (gameMode == MODE_HOTSEAT) {
        msg = "Pass to " + ((currentPlayer == White) ? playerNameWhite : playerNameBlack);
    } else {
        msg = (currentPlayer == White) ? "White's Turn" : "Black's Turn";
    }
    msg += " \u2014 Click anywhere";

    int w = GetScreenWidth();
    int h = GetScreenHeight();
    DrawText(msg.c_str(), w/2 - MeasureText(msg.c_str(), 36)/2, h/2 - 18, 36, WHITE);
}

void Window::renderNetworkStatus() {
    if (waitingForOpponent && !gameOver) {
        BeginDrawing();
            ClearBackground(RAYWHITE);
            drawBoard();
            auto pieces = game.getPieces();
            for (auto cur : pieces)
                drawPiece(cur.second, getSquarePosition(cur.first));
            drawSuperpositionGhosts();
            highlightCheckedKing();
            DrawText("Waiting for opponent...", 20, 20, 30, DARKGRAY);
        EndDrawing();
    }
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

    Rectangle btn1 = {(float)(w/2 - 120), (float)(h/2 - 80), 240, 50};
    Rectangle btn2 = {(float)(w/2 - 120), (float)(h/2 - 10), 240, 50};
    Rectangle btn3 = {(float)(w/2 - 120), (float)(h/2 + 60), 240, 50};
    Rectangle btn4 = {(float)(w/2 - 120), (float)(h/2 + 130), 240, 50};

    audio.playButtonClick();

    if (isInside(mouse, btn1)) {
        gameMode = MODE_LOCAL;
        currentScreen = SCREEN_SETUP;
    } else if (isInside(mouse, btn2)) {
        gameMode = MODE_HOTSEAT;
        currentScreen = SCREEN_SETUP;
    } else if (isInside(mouse, btn3)) {
        gameMode = MODE_NETWORK_HOST;
        if (net.startHost()) {
            networkStatus = "";
            currentScreen = SCREEN_SETUP;
        } else {
            networkStatus = "Failed to start host";
        }
    } else if (isInside(mouse, btn4)) {
        if (net.getRole() == NetworkRole::None) {
            gameMode = MODE_NETWORK_CLIENT;
            currentScreen = SCREEN_SETUP;
        }
    }
}

void Window::handleSetupClick() {
    Vector2 mouse = GetMousePosition();
    int w = GetScreenWidth();
    int h = GetScreenHeight();

    audio.playButtonClick();

    if (gameMode == MODE_NETWORK_HOST) {
        if (net.isConnected() && !gameOver) {
            startGame();
        }
    } else if (gameMode == MODE_NETWORK_CLIENT) {
        int fieldX = w/2 - 80;
        int y1 = h/2 - 40;
        int y2 = h/2 + 20;

        if (isInside(mouse, {(float)(fieldX - 4), (float)(y1 - 6), 208, 38}))
            ipInputActive = 0;
        else if (isInside(mouse, {(float)(fieldX - 4), (float)(y2 - 6), 208, 38}))
            ipInputActive = 1;

        Rectangle joinBtn = {(float)(w/2 - 100), (float)(h/2 + 80), 200, 40};
        if (isInside(mouse, joinBtn)) {
            if (net.connectToHost(ipInput.c_str())) {
                networkStatus = "";
                playerNameBlack = playerNameWhite;
                playerNameWhite = "Host";
                startGame();
            } else {
                networkStatus = "Failed to connect to " + ipInput;
            }
        }
    } else {
        int fieldX = w/2 - 80;
        int y1 = h/2 - 42;
        int y2 = h/2 + 18;

        if (isInside(mouse, {(float)(fieldX - 2 - 4), (float)(y1 - 2 - 4), 208, 38}))
            nameInputActive = 0;
        else if (isInside(mouse, {(float)(fieldX - 2 - 4), (float)(y2 - 2 - 4), 208, 38}))
            nameInputActive = 1;

        Rectangle startBtn = {(float)(w/2 - 100), (float)(h/2 + 80), 200, 40};
        if (isInside(mouse, startBtn))
            startGame();
    }
}

void Window::handleSetupKeyInput() {
    if (gameMode == MODE_NETWORK_CLIENT) {
        int key = GetCharPressed();
        while (key > 0) {
            if (key >= 32 && key <= 126) {
                if (ipInputActive == 0 && ipInput.size() < 20 &&
                    (key == '.' || (key >= '0' && key <= '9')))
                    ipInput.push_back((char)key);
                else if (ipInputActive == 1 && playerNameWhite.size() < 16)
                    playerNameWhite.push_back((char)key);
            }
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE)) {
            std::string* s = (ipInputActive == 0) ? &ipInput : &playerNameWhite;
            if (!s->empty()) s->pop_back();
        }
        if (IsKeyPressed(KEY_ENTER)) {
            if (net.connectToHost(ipInput.c_str())) {
                networkStatus = "";
                playerNameBlack = playerNameWhite;
                playerNameWhite = "Host";
                startGame();
            } else {
                networkStatus = "Failed to connect to " + ipInput;
            }
        }
    } else if (gameMode == MODE_NETWORK_HOST) {
        int key = GetCharPressed();
        while (key > 0) {
            if (key >= 32 && key <= 126 && playerNameWhite.size() < 16)
                playerNameWhite.push_back((char)key);
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && !playerNameWhite.empty())
            playerNameWhite.pop_back();
        if (IsKeyPressed(KEY_ENTER) && net.isConnected())
            startGame();
    } else {
        int key = GetCharPressed();
        while (key > 0) {
            if (key >= 32 && key <= 126) {
                std::string* name = (nameInputActive == 0) ? &playerNameWhite : &playerNameBlack;
                if (name->size() < 16) name->push_back((char)key);
            }
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE)) {
            std::string* name = (nameInputActive == 0) ? &playerNameWhite : &playerNameBlack;
            if (!name->empty()) name->pop_back();
        }
        if (IsKeyPressed(KEY_ENTER)) startGame();
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
    waitingForOpponent = (gameMode == MODE_NETWORK_CLIENT);
    currentScreen = SCREEN_PLAYING;
}

void Window::pollEvents(){
    if(IsKeyPressed(KEY_ESCAPE)) {
        if (currentScreen == SCREEN_PLAYING) {
            net.disconnect();
            waitingForOpponent = false;
            gameMode = MODE_LOCAL;
            currentScreen = SCREEN_TITLE;
        } else if (currentScreen == SCREEN_TITLE) {
            net.disconnect();
            CloseWindow();
        } else {
            net.disconnect();
            currentScreen = SCREEN_TITLE;
        }
    }
    if (IsWindowResized())
        resizedWindow();

    if (currentScreen == SCREEN_TITLE) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            handleTitleClick();

        if (net.getRole() == NetworkRole::Host && net.isConnected() && currentScreen == SCREEN_TITLE) {
            currentScreen = SCREEN_SETUP;
        }
    } else if (currentScreen == SCREEN_SETUP) {
        handleSetupKeyInput();
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            handleSetupClick();
        if (net.getRole() == NetworkRole::Host && net.isConnected() && currentScreen == SCREEN_SETUP) {
            gameMode = MODE_NETWORK_HOST;
        }
    } else if (currentScreen == SCREEN_PLAYING) {
        if (IsKeyPressed(KEY_R) && !isNetworkGame())
            restartGame();

        if (isNetworkGame())
            processNetworkMessages();

        if (waitingForOpponent)
            return;

        if (passClickRequired) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                passClickRequired = false;
            return;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            handleLeftMouseDown();
    }
}

void Window::processNetworkMessages() {
    while (true) {
        NetMessage msg = net.receiveMessage();
        if (msg.type == NetMsgType::None) break;

        switch (msg.type) {
        case NetMsgType::Quantum:
            applyRemoteQuantum(msg);
            break;
        case NetMsgType::Move:
            applyRemoteMove(msg);
            break;
        case NetMsgType::Collapse:
            applyRemoteCollapse(msg);
            break;
        case NetMsgType::Miss:
            waitingForOpponent = false;
            {
                bool wasCheck = game.isInCheck(currentPlayer);
                checkGameEnd();
                if (!gameOver && wasCheck) audio.playCheck();
                if (gameOver) audio.playCheckmate();
            }
            break;
        case NetMsgType::Disconnect:
            gameOver = true;
            gameOverMessage = "Opponent disconnected";
            waitingForOpponent = false;
            break;
        default:
            break;
        }
    }

    if (!waitingForOpponent && gameMode == MODE_NETWORK_HOST && !gameOver) {
        gameState = quantumPickFirst;
    }
}

void Window::sendCurrentMove() {
    if (!isNetworkGame()) return;

    NetMessage msg;
    msg.type = NetMsgType::Move;
    msg.data[0] = moves.m1.start.row;
    msg.data[1] = moves.m1.start.column;
    msg.data[2] = moves.m1.end.row;
    msg.data[3] = moves.m1.end.column;
    net.sendMessage(msg);
    waitingForOpponent = true;
}

void Window::sendQuantumMove() {
    if (!isNetworkGame()) return;

    NetMessage msg;
    msg.type = NetMsgType::Quantum;
    msg.data[0] = moves.m1.start.row;
    msg.data[1] = moves.m1.start.column;
    msg.data[2] = moves.m1.end.row;
    msg.data[3] = moves.m1.end.column;
    msg.data[4] = moves.m2.end.row;
    msg.data[5] = moves.m2.end.column;
    net.sendMessage(msg);
    waitingForOpponent = true;
}

void Window::applyRemoteQuantum(const NetMessage& msg) {
    Pos original = {msg.data[0], msg.data[1]};
    Pos dest1 = {msg.data[2], msg.data[3]};
    Pos dest2 = {msg.data[4], msg.data[5]};
    PieceID type = game.getPieceID(original);

    game.enterSuperposition(original, dest1, dest2, type,
                            (currentPlayer == White) ? Black : White);

    currentPlayer = (currentPlayer == White) ? Black : White;
    gameState = quantumPickFirst;
    waitingForOpponent = false;
}

void Window::applyRemoteMove(const NetMessage& msg) {
    Move m = {{msg.data[0], msg.data[1]}, {msg.data[2], msg.data[3]}};
    bool wasCapture = !game.isEmpty(m.end);
    game.movePiece(m);
    lastMove = m;
    if (wasCapture) audio.playCapture();
    else audio.playMove();

    currentPlayer = (currentPlayer == White) ? Black : White;

    if (game.hasActiveSuperposition()) {
        audio.playQuantumCollapse();
        game.collapseSuperposition();
    }

    bool wasCheck = game.isInCheck(currentPlayer);
    checkGameEnd();
    if (gameOver) {
        audio.playCheckmate();
    } else if (wasCheck) {
        audio.playCheck();
    }

    gameState = quantumPickFirst;
    waitingForOpponent = false;
}

void Window::applyRemoteCollapse(const NetMessage& msg) {
    Pos result = {msg.data[0], msg.data[1]};
    audio.playQuantumCollapse();

    collapseResult = result;
    game.clearSuperposition();
}

void Window::handleLeftMouseDown(){
    if (gameOver || passClickRequired || waitingForOpponent) return;

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
        game.enterSuperposition(moves.m1.start, moves.m1.end, moves.m2.end,
                                game.getPieceID(moves.m1.start), currentPlayer);

        if (isNetworkGame()) {
            sendQuantumMove();
            return;
        }

        SquareColor opponent = (currentPlayer == White) ? Black : White;
        currentPlayer = opponent;
        gameState = opponentPickPiece;
        if (gameMode == MODE_HOTSEAT) passClickRequired = true;
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
    waitingForOpponent = false;
    currentScreen = SCREEN_SETUP;
}

void Window::drawSuperpositionGhosts() {
    if (!game.hasActiveSuperposition()) return;

    const auto& sup = game.getSuperposition();
    float squareSize = boardWidth / 8;

    drawPiece(sup.pieceType, getSquarePosition(sup.originalPos), 0.3f);

    Vector2 p1 = getSquarePosition(sup.pos1);
    Vector2 p2 = getSquarePosition(sup.pos2);
    p1.x += squareSize / 2; p1.y += squareSize / 2;
    p2.x += squareSize / 2; p2.y += squareSize / 2;
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
    DrawTexturePro(sprites[pieceKey], {0, 0, spriteWidth, spriteHeight},
                   destination, {0, 0}, 0.0f, tint);
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
