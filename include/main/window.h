#ifndef WINDOW_H
#define WINDOW_H

#include "board.h"
#include "audio.h"
#include "network.h"
#include "raylib.h"
#include <vector>
#include <random>
#include <string>
#include <queue>

enum Screen {
    SCREEN_TITLE,
    SCREEN_SETUP,
    SCREEN_PLAYING
};

enum GameMode {
    MODE_LOCAL,
    MODE_HOTSEAT,
    MODE_NETWORK_HOST,
    MODE_NETWORK_CLIENT
};

class Window{
private:
    int screenWidth = 1200;
    int screenHeight = 900;

    std::vector<Texture2D> sprites;
    const float spriteWidth = 300;
    const float spriteHeight = 300;

    Vector2 boardStart;
    Vector2 boardEnd;
    float boardWidth;

    Screen currentScreen = SCREEN_TITLE;
    GameMode gameMode = MODE_LOCAL;

    Board game;
    States gameState = States::selectPiece;
    MovePair moves;
    SquareColor currentPlayer = White;
    bool quantumMode = true;

    std::vector<Pos> validMovePositions;

    bool gameOver = false;
    std::string gameOverMessage;

    Move lastMove = {{-1,-1}, {-1,-1}};

    std::string playerNameWhite = "White";
    std::string playerNameBlack = "Black";
    int nameInputActive = 0;
    int ipInputActive = 0;
    bool passClickRequired = false;

    NetworkManager net;
    bool waitingForOpponent = false;
    std::string networkStatus = "";
    std::string ipInput = "127.0.0.1";

    AudioManager audio;
    std::mt19937 rng;

    void loadSprites();
    void drawBoard();
    void render();
    void renderTitleScreen();
    void renderSetupScreen();
    void renderGame();
    void pollEvents();
    void handleTitleClick();
    void handleSetupClick();
    void handleSetupKeyInput();
    void handleLeftMouseDown();
    void processNetworkMessages();
    void resizedWindow();
    void endTurn();
    Vector2 getSquarePosition(Pos square);
    void drawPiece(int pieceKey, Vector2 pos, float alpha = 1.0f);
    void highlightSquare(Pos pos, Color color = {0, 121, 241, 255});
    void highlightMovesSelected();
    void highlightCheckedKing();
    void drawSuperpositionGhosts();
    void startGame();
    void restartGame();
    void checkGameEnd();
    bool isNetworkGame() const;
    Vector2 getMouse();
    bool mousePressedLastFrame = false;

public:
    Window();
    ~Window();
    void run();
    Pos getSquare(Vector2 cursorPosition);
    void updateBoard();
};

#endif
