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

struct MoveRecord {
    std::string san;        // algebraic notation, e.g. "Nf3" or "exd5"
    SquareColor color;      // who played it
};

class Window{
private:
    int screenWidth = 900;
    int screenHeight = 700;

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
    int nameInputActive = -1;
    int ipInputActive = -1;
    bool passClickRequired = false;

    NetworkManager net;
    bool waitingForOpponent = false;
    std::string networkStatus;
    std::string ipInput = "127.0.0.1";

    AudioManager audio;
    std::mt19937 rng;

    // --- new tunable / debug state ---
    bool shouldClose = false;
    bool boardFlipped = false;     // render board from Black's perspective
    bool showDebug = false;        // F1 overlay
    bool audioEnabled = true;      // M toggles
    bool promotionPending = false;
    SquareColor promotionColor = White;
    Pos promotionSquare = {-1, -1};
    Move promotionMove = {{-1,-1}, {-1,-1}};

    // move history & captured pieces
    std::vector<MoveRecord> moveHistory;
    std::vector<PieceID> capturedByWhite;
    std::vector<PieceID> capturedByBlack;

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
    Pos getSquarePositionFlipped(Pos square);
    void drawPiece(int pieceKey, Vector2 pos, float alpha = 1.0f);
    void highlightSquare(Pos pos, Color color = {0, 121, 241, 255});
    void highlightMovesSelected();
    void highlightCheckedKing();
    void drawSuperpositionGhosts();
    void drawCoordinateLabels();
    void drawMoveHistoryPanel();
    void drawCapturedPiecesPanel();
    void drawDebugOverlay();
    void drawPromotionDialog();
    void startGame();
    void restartGame();
    void checkGameEnd();
    bool isNetworkGame() const;
    void sendNetworkMove(Move m, NetMsgType t, int promoteTo = -1);
    bool isPromotionMove(const Move& m) const;
    void executeNormalMove(Move m);
    Vector2 getMouse();
    Pos squareToView(Pos s) const;     // applies flip
    Pos viewToSquare(Pos s) const;     // inverse of squareToView
    std::string moveToSAN(const Move& m, PieceID movedType, bool capture, bool check, bool mate);
    void recordMove(const Move& m, PieceID movedType, bool capture);
    void performPromotion(PieceID chosen);

public:
    Window();
    ~Window();
    void run();
    Pos getSquare(Vector2 cursorPosition);
    void updateBoard();
};

#endif
