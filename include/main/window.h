/*
    window.h

    window class for QuantumChess Project

    ZipCode
*/

#ifndef WINDOW_H
#define WINDOW_H

#include "board.h"
#include "raylib.h"
#include <vector>
#include <random>
#include <string>

class Window{
private:
    int screenWidth = 1600;
    int screenHeight = 1000;

    std::vector<Texture2D> sprites;
    const float spriteWidth = 300;
    const float spriteHeight = 300;

    RenderTexture2D board;
    Vector2 boardStart;
    Vector2 boardEnd;
    float boardWidth;

    Board game;
    States gameState = States::pickPieceFirst;
    MovePair moves;
    SquareColor currentPlayer = White;

    std::vector<Pos> validMovePositions;

    bool gameOver = false;
    std::string gameOverMessage;

    std::mt19937 rng;

    void loadSprites();

    void handleLeftMouseDown();

    void render();

    void pollEvents();

    void resizedWindow();

    Vector2 getSquarePosition(Pos square);

    void createBoardTexture();

    void drawPiece(int pieceKey, Vector2 pos, bool center = false);

    void highlightSquare(Pos pos, Color color = BLUE);

    void highlightMovesSelected();

    void restartGame();

public:
    Window();

    ~Window();

    void run();

    Pos getSquare(Vector2 cursorPosition);

    void updateBoard();

};

#endif
