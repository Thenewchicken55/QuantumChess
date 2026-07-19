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

// Shared button geometry helper. Draws a button with hover/press feedback and
// returns true when the user clicked it this frame. `mouse` is the cursor pos
// and `pressed` is true on the frame the left button was released.
static bool drawButton(const char* label, Rectangle r, Vector2 mouse, bool clicked) {
    bool hover = isInside(mouse, r);
    Color bg = hover ? GRAY : DARKGRAY;
    DrawRectangleRec(r, bg);
    DrawRectangleLines(r.x, r.y, r.width, r.height, BLACK);
    DrawText(label, (int)(r.x + (r.width - MeasureText(label, 22)) / 2),
             (int)(r.y + (r.height - 22) / 2), 22, WHITE);
    return hover && clicked;
}

// Title-screen button layout so render and click handlers stay in sync.
struct TitleButton { const char* label; int dy; };
static const TitleButton kTitleButtons[] = {
    {"Local Game", -80},
    {"Hot-Seat",   -10},
    {"Host Game",   60},
    {"Join Game",  130},
};
static Rectangle titleButtonRect(int index, int w, int h) {
    const int bw = 240, bh = 50;
    return {(float)(w/2 - bw/2), (float)(h/2 + kTitleButtons[index].dy - bh/2),
            (float)bw, (float)bh};
}

Window::Window()
    : rng(std::chrono::steady_clock::now().time_since_epoch().count()) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(screenWidth, screenHeight, "Quantum Chess");
    SetExitKey(0); // handle ESC ourselves so it can navigate screens
    SetTargetFPS(60);
    int mon = GetCurrentMonitor();
    SetWindowPosition(GetMonitorWidth(mon)/2 - screenWidth/2,
                      GetMonitorHeight(mon)/2 - screenHeight/2);
    InitAudioDevice();
    audio.init();
    updateBoard();
    loadSprites();
}

Window::~Window(){
    net.disconnect();
    for (auto& cur : sprites)
        if (cur.id != 0) UnloadTexture(cur);
    CloseAudioDevice();
    CloseWindow();
}

bool Window::isNetworkGame() const {
    return gameMode == MODE_NETWORK_HOST || gameMode == MODE_NETWORK_CLIENT;
}

void Window::drawBoard() {
    float sq = boardWidth / 8;
    for (int c = 0; c < 8; ++c)
        for (int r = 0; r < 8; ++r)
            DrawRectangleV({boardStart.x + c*sq, boardStart.y + r*sq}, {sq, sq},
                ((r+c)%2==0) ? Color{181,136,99,255} : Color{240,217,181,255});
}

void Window::highlightCheckedKing() {
    if (game.isInCheck(White)) { Pos k = game.findKing(White); if (k.row!=-1) highlightSquare(k,RED); }
    if (game.isInCheck(Black)) { Pos k = game.findKing(Black); if (k.row!=-1) highlightSquare(k,RED); }
}

void Window::highlightMovesSelected() {
    if (gameState == selectDest1 || gameState == selectDest2)
        for (auto& cur : validMovePositions) highlightSquare(cur, RED);
    switch (gameState) {
    case selectDest2: highlightSquare(moves.m1.end, PURPLE);
    case selectDest1: highlightSquare(moves.m1.start, PURPLE);
    default: break;
    }
}

void Window::renderTitleScreen() {
    int w=GetScreenWidth(), h=GetScreenHeight();
    Vector2 mouse = getMouse();
    DrawText("Quantum Chess", w/2-MeasureText("Quantum Chess",60)/2, h/4, 60, DARKGRAY);

    bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    for (int i=0;i<4;i++) {
        Rectangle r = titleButtonRect(i, w, h);
        if (drawButton(kTitleButtons[i].label, r, mouse, clicked)) {
            // Handled in handleTitleClick; nothing to do here but we still draw
            // so hover feedback is visible before the screen transitions.
        }
    }
    DrawText("Click a button or press L/H/O/J",
             w/2-MeasureText("Click a button or press L/H/O/J",18)/2,
             h-50, 18, GRAY);

    if (net.getRole()==NetworkRole::Host) {
        Rectangle r = titleButtonRect(2, w, h);
        DrawText("Hosting port 5555...", (int)r.x, (int)(r.y+r.height+20), 18, DARKGRAY);
        if (net.isConnected()) DrawText("Opponent connected!", (int)r.x, (int)(r.y+r.height+45), 20, GREEN);
    }

    DrawText("ESC to quit", w/2-MeasureText("ESC to quit",16)/2, h-20, 16, GRAY);
}

void Window::renderSetupScreen() {
    int w=GetScreenWidth(), h=GetScreenHeight();
    Vector2 mouse = getMouse();
    const char* title = (gameMode==MODE_NETWORK_HOST||gameMode==MODE_NETWORK_CLIENT)?"Network Setup":"Player Setup";
    DrawText(title, w/2-MeasureText(title,50)/2, h/4-40, 50, DARKGRAY);

    if (gameMode==MODE_NETWORK_CLIENT) {
        DrawText("Host IP:", w/2-200, h/2-40,24,DARKGRAY);
        DrawText("Your Name:", w/2-200, h/2+20,24,DARKGRAY);
        DrawRectangle(w/2-84,h/2-46,208,38,ipInputActive==0?LIGHTGRAY:WHITE);
        DrawRectangleLines(w/2-84,h/2-46,208,38,DARKGRAY);
        DrawText(ipInput.c_str(), w/2-80, h/2-40,22,BLACK);
        DrawRectangle(w/2-84,h/2+14,208,38,ipInputActive==1?LIGHTGRAY:WHITE);
        DrawRectangleLines(w/2-84,h/2+14,208,38,DARKGRAY);
        DrawText(playerNameWhite.c_str(), w/2-80, h/2+20,22,BLACK);
        Rectangle jb={(float)(w/2-100),(float)(h/2+80),200,40};
        drawButton("Connect", jb, mouse, false);
        if (!networkStatus.empty()) DrawText(networkStatus.c_str(),w/2-MeasureText(networkStatus.c_str(),18)/2,h/2+140,18,RED);
    } else if (gameMode==MODE_NETWORK_HOST) {
        DrawText("Your Name (White):", w/2-200, h/2-10,24,DARKGRAY);
        DrawRectangle(w/2-84,h/2-14,208,38,LIGHTGRAY);
        DrawRectangleLines(w/2-84,h/2-14,208,38,DARKGRAY);
        DrawText(playerNameWhite.c_str(), w/2-80, h/2+2,22,BLACK);
        if (!networkStatus.empty()) DrawText(networkStatus.c_str(),w/2-MeasureText(networkStatus.c_str(),18)/2,h/2+60,18,RED);
    } else {
        DrawText("White:", w/2-200, h/2-40,24,DARKGRAY);
        DrawText("Black:", w/2-200, h/2+20,24,DARKGRAY);
        DrawRectangle(w/2-80,h/2-42,200,30,nameInputActive==0?LIGHTGRAY:WHITE);
        DrawRectangle(w/2-80,h/2+18,200,30,nameInputActive==1?LIGHTGRAY:WHITE);
        DrawRectangleLines(w/2-80,h/2-42,200,30,DARKGRAY);
        DrawRectangleLines(w/2-80,h/2+18,200,30,DARKGRAY);
        DrawText(playerNameWhite.c_str(), w/2-76, h/2-38,22,BLACK);
        DrawText(playerNameBlack.c_str(), w/2-76, h/2+22,22,BLACK);
        Rectangle sb={(float)(w/2-100),(float)(h/2+80),200,40};
        drawButton("Start Game", sb, mouse, false);
        DrawText("Click name, then type", w/2-120, h/2+140,16,GRAY);
    }
    Rectangle bb={(float)(w/2-110),(float)(h-90),220,50};
    drawButton("Back", bb, mouse, false);
    DrawText("ESC=Back  ENTER=Start", w/2-MeasureText("ESC=Back  ENTER=Start",18)/2, h-25,18,GRAY);
    if (!networkStatus.empty()) DrawText(networkStatus.c_str(),w/2-MeasureText(networkStatus.c_str(),18)/2,h-120,18,RED);
}

void Window::renderGame() {
    BeginDrawing();
    ClearBackground(RAYWHITE);
    drawBoard();
    drawCoordinateLabels();
    if (lastMove.start.row!=-1) {
        highlightSquare(lastMove.start, Fade(YELLOW,0.3f));
        highlightSquare(lastMove.end, Fade(YELLOW,0.3f));
    }
    for (const auto& cur : game.getPieces())
        drawPiece(cur.second, getSquarePosition(cur.first));
    drawSuperpositionGhosts();
    highlightSquare(getSquare(getMouse()));
    highlightMovesSelected();
    highlightCheckedKing();
    drawMoveHistoryPanel();
    drawCapturedPiecesPanel();

    if (gameOver) {
        DrawText(gameOverMessage.c_str(),20,20,40,DARKGRAY);
        DrawText(isNetworkGame()?"ESC to quit":"R to restart",20,70,30,DARKGRAY);
    } else if (waitingForOpponent) {
        DrawText("Waiting for opponent...",20,20,30,DARKGRAY);
    } else {
        auto name = (currentPlayer==White)?playerNameWhite:playerNameBlack;
        DrawText((name+"'s turn").c_str(),20,20,30,BLACK);
        std::string m = quantumMode?"[Q] Quantum: ON":"[Q] Quantum: OFF";
        DrawText(m.c_str(),GetScreenWidth()-MeasureText(m.c_str(),22)-20,20,22,quantumMode?PURPLE:GRAY);
        if (game.isInCheck(White)) DrawText("White in check",20,55,25,RED);
        if (game.isInCheck(Black)) DrawText("Black in check",20,85,25,RED);
    }
    if (passClickRequired && !isNetworkGame()) {
        DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),Fade(BLACK,0.5f));
        std::string pm = "Pass to "+((currentPlayer==White)?playerNameWhite:playerNameBlack)+" - Click";
        DrawText(pm.c_str(),GetScreenWidth()/2-MeasureText(pm.c_str(),36)/2,GetScreenHeight()/2-18,36,WHITE);
    }
    if (showDebug) drawDebugOverlay();
    drawPromotionDialog();
    EndDrawing();
}

void Window::render() {
    if (currentScreen==SCREEN_TITLE) { BeginDrawing(); ClearBackground(RAYWHITE); renderTitleScreen(); EndDrawing(); }
    else if (currentScreen==SCREEN_SETUP) { BeginDrawing(); ClearBackground(RAYWHITE); renderSetupScreen(); EndDrawing(); }
    else renderGame();
}

Vector2 Window::getMouse() {
    // raylib's GetMousePosition() already returns client-area coordinates
    // (relative to the window's drawable region, excluding the title bar).
    // The previous +22 Y offset was a macOS-specific workaround for a
    // SetWindowPosition quirk; it broke input on other platforms and masked
    // the real issue. If a macOS offset is needed again, gate it behind
    // __APPLE__ and GetWindowPosition() rather than hardcoding 22.
    return GetMousePosition();
}

void Window::handleTitleClick() {
    Vector2 m = getMouse();
    int w=GetScreenWidth(), h=GetScreenHeight();
    for (int i=0;i<4;i++) {
        if (!isInside(m, titleButtonRect(i, w, h))) continue;
        audio.playButtonClick();
        switch (i) {
            case 0: gameMode = MODE_LOCAL;          currentScreen = SCREEN_SETUP; return;
            case 1: gameMode = MODE_HOTSEAT;        currentScreen = SCREEN_SETUP; return;
            case 2: gameMode = MODE_NETWORK_HOST;
                    if (net.startHost()) networkStatus.clear();
                    currentScreen = SCREEN_SETUP; return;
            case 3: gameMode = MODE_NETWORK_CLIENT; currentScreen = SCREEN_SETUP; return;
        }
    }
}

void Window::handleSetupClick() {
    Vector2 m = getMouse();
    int w=GetScreenWidth(), h=GetScreenHeight();

    // BACK button is common to all setup modes.
    Rectangle bb={(float)(w/2-110),(float)(h-90),220,50};
    if (isInside(m, bb)) {
        net.disconnect();
        networkStatus.clear();
        currentScreen = SCREEN_TITLE;
        audio.playButtonClick();
        return;
    }

    if (gameMode==MODE_NETWORK_CLIENT) {
        if (isInside(m,{(float)(w/2-84),(float)(h/2-46),208,38})) { ipInputActive=0; nameInputActive=-1; return; }
        if (isInside(m,{(float)(w/2-84),(float)(h/2+14),208,38})) { ipInputActive=1; nameInputActive=-1; return; }
        if (isInside(m,{(float)(w/2-100),(float)(h/2+80),200,40})) {
            if (net.connectToHost(ipInput.c_str())) {
                networkStatus.clear();
                playerNameBlack=playerNameWhite; playerNameWhite="Host";
                startGame();
            } else networkStatus="Failed to connect";
            return;
        }
    } else if (gameMode==MODE_NETWORK_HOST) {
        if (isInside(m,{(float)(w/2-84),(float)(h/2-14),208,38})) { nameInputActive=0; ipInputActive=-1; return; }
        if (net.isConnected()) { startGame(); return; }
    } else {
        if (isInside(m,{(float)(w/2-80),(float)(h/2-42),200,30})) { nameInputActive=0; ipInputActive=-1; return; }
        if (isInside(m,{(float)(w/2-80),(float)(h/2+18),200,30})) { nameInputActive=1; ipInputActive=-1; return; }
        Rectangle sb={(float)(w/2-100),(float)(h/2+80),200,40};
        if (isInside(m, sb)) { startGame(); return; }
    }
}

void Window::handleSetupKeyInput() {
    auto f = [](std::string& s, int mx){ int k; while((k=GetCharPressed())>0) if(k>=32&&k<=126&&(int)s.size()<mx) s.push_back((char)k); if(IsKeyPressed(KEY_BACKSPACE)&&!s.empty()) s.pop_back(); };
    if (gameMode==MODE_NETWORK_CLIENT) {
        if(ipInputActive==0) f(ipInput,20);
        else if(ipInputActive==1) f(playerNameWhite,16);
        if(IsKeyPressed(KEY_ENTER)&&net.connectToHost(ipInput.c_str())) {
            networkStatus.clear(); playerNameBlack=playerNameWhite; playerNameWhite="Host"; startGame();
        } else if(IsKeyPressed(KEY_ENTER)) networkStatus="Failed to connect";
    } else if (gameMode==MODE_NETWORK_HOST) {
        if(nameInputActive==0) f(playerNameWhite,16);
        if(IsKeyPressed(KEY_ENTER)&&net.isConnected()) startGame();
    } else {
        if(nameInputActive==0) f(playerNameWhite,16);
        else if(nameInputActive==1) f(playerNameBlack,16);
        if(IsKeyPressed(KEY_ENTER)) startGame();
    }
}

void Window::startGame() {
    if (playerNameWhite.empty()) playerNameWhite="White";
    if (playerNameBlack.empty()) playerNameBlack="Black";
    game.resetBoard(); gameState=selectPiece; currentPlayer=White; gameOver=false; gameOverMessage.clear();
    validMovePositions.clear(); moves={}; lastMove={{-1,-1},{-1,-1}}; passClickRequired=false;
    waitingForOpponent=(gameMode==MODE_NETWORK_CLIENT); quantumMode=true; currentScreen=SCREEN_PLAYING;
    moveHistory.clear(); capturedByWhite.clear(); capturedByBlack.clear();
    promotionPending=false; boardFlipped=false;
}

void Window::sendNetworkMove(Move m, NetMsgType t, int promoteTo) {
    if (!isNetworkGame()) return;
    NetMessage msg;
    msg.type = t;
    msg.data[0] = m.start.row; msg.data[1] = m.start.column;
    msg.data[2] = m.end.row; msg.data[3] = m.end.column;
    msg.data[4] = promoteTo;
    net.sendMessage(msg);
}

bool Window::isPromotionMove(const Move& m) const {
    PieceID t = game.getPieceID(m.start);
    if (t != WPawn && t != BPawn) return false;
    return (m.end.row == 0 || m.end.row == 7);
}

void Window::executeNormalMove(Move m) {
    PieceID movedType = game.getPieceID(m.start);
    bool wasCapture = !game.isEmpty(m.end);
    PieceID capturedPiece = wasCapture ? game.getPieceID(m.end) : InvalidPiece;
    // en passant: pawn moves diagonally onto an empty (en-passant) target square
    if ((movedType==WPawn||movedType==BPawn) && m.end.column!=m.start.column && game.isEmpty(m.end)) {
        Pos ep = game.getEnPassantTarget();
        if (m.end.row==ep.row && m.end.column==ep.column) {
            wasCapture = true;
            capturedPiece = (currentPlayer==White) ? BPawn : WPawn;
        }
    }
    if (isPromotionMove(m)) {
        // Defer execution until the player picks a piece in the dialog.
        promotionPending = true;
        promotionColor = currentPlayer;
        promotionSquare = m.end;
        promotionMove = m;
        return;
    }
    game.movePiece(m);
    lastMove = m;
    if (wasCapture) {
        if (currentPlayer == White) capturedByWhite.push_back(capturedPiece);
        else capturedByBlack.push_back(capturedPiece);
        audio.playCapture();
    } else {
        audio.playMove();
    }
    recordMove(m, movedType, wasCapture);
    sendNetworkMove(m, NetMsgType::Move);
    endTurn();
}

void Window::endTurn() {
    currentPlayer = (currentPlayer==White)?Black:White;
    checkGameEnd();
    if (gameOver) audio.playCheckmate();
    else if (game.isInCheck(currentPlayer)) audio.playCheck();
    if (gameMode==MODE_HOTSEAT&&!gameOver) passClickRequired=true;
    // In a network game the player who just moved must wait for the opponent.
    // Without this, the local side could move the opponent's pieces between
    // sending the move and receiving the reply.
    if (isNetworkGame() && !gameOver) waitingForOpponent = true;
    gameState = selectPiece;
}

void Window::pollEvents() {
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (currentScreen==SCREEN_PLAYING) {
            net.disconnect(); waitingForOpponent=false;
            gameMode=MODE_LOCAL; currentScreen=SCREEN_TITLE;
        } else if (currentScreen==SCREEN_SETUP) {
            net.disconnect(); networkStatus.clear();
            currentScreen=SCREEN_TITLE;
        } else {
            // Defer the actual close to WindowShouldClose() so the destructor
            // doesn't double-close the window.
            net.disconnect();
            shouldClose = true;
        }
    }
    if (IsWindowResized()) resizedWindow();
    if (IsKeyPressed(KEY_F1)) showDebug = !showDebug;
    if (currentScreen==SCREEN_TITLE) {
        if (IsKeyPressed(KEY_L)) { gameMode=MODE_LOCAL; currentScreen=SCREEN_SETUP; audio.playButtonClick(); }
        else if (IsKeyPressed(KEY_H)) { gameMode=MODE_HOTSEAT; currentScreen=SCREEN_SETUP; audio.playButtonClick(); }
        else if (IsKeyPressed(KEY_O)) { gameMode=MODE_NETWORK_HOST; if (net.startHost()) networkStatus.clear(); currentScreen=SCREEN_SETUP; audio.playButtonClick(); }
        else if (IsKeyPressed(KEY_J)) { gameMode=MODE_NETWORK_CLIENT; currentScreen=SCREEN_SETUP; audio.playButtonClick(); }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) handleTitleClick();
        if (net.getRole()==NetworkRole::Host&&net.isConnected()&&gameMode==MODE_NETWORK_HOST) currentScreen=SCREEN_SETUP;
    } else if (currentScreen==SCREEN_SETUP) {
        handleSetupKeyInput();
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) handleSetupClick();
    } else if (currentScreen==SCREEN_PLAYING) {
        if (IsKeyPressed(KEY_R)&&!isNetworkGame()) restartGame();
        if (IsKeyPressed(KEY_Q)&&!gameOver&&!waitingForOpponent) { quantumMode=!quantumMode; audio.playButtonClick(); }
        if (IsKeyPressed(KEY_F)&&!gameOver&&!waitingForOpponent&&!isNetworkGame()) { boardFlipped=!boardFlipped; audio.playButtonClick(); }
        if (IsKeyPressed(KEY_M)) { audioEnabled=!audioEnabled; audio.setVolume(audioEnabled?1.0f:0.0f); }
        if (isNetworkGame()) processNetworkMessages();
        if (waitingForOpponent) return;
        if (passClickRequired) { if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) passClickRequired=false; return; }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) handleLeftMouseDown();
    }
}

void Window::processNetworkMessages() {
    while (true) {
        NetMessage msg = net.receiveMessage();
        if (msg.type==NetMsgType::None) break;
        switch (msg.type) {
        case NetMsgType::Quantum: {
            Pos o={msg.data[0],msg.data[1]};
            std::vector<Pos> d={{msg.data[2],msg.data[3]},{msg.data[4],msg.data[5]}};
            auto side = (currentPlayer==White)?Black:White;
            // If the opponent is re-entering superposition on an already-active
            // superposition piece, we must add positions rather than overwrite,
            // and use the existing pieceType (getPieceID(originalPos) returns
            // InvalidPiece because the original square is masked).
            if (game.hasActiveSuperposition() && game.isSuperpositionOriginal(o)) {
                game.addSuperpositionPositions(d);
            } else {
                PieceID pt = game.getPieceID(o);
                if (pt == InvalidPiece) pt = game.getSuperposition().pieceType;
                game.enterSuperposition(o, d, pt, side);
            }
            audio.playQuantumEnter();
            currentPlayer = side; waitingForOpponent=false;
            break;
        }
        case NetMsgType::Move: {
            Move mv={{msg.data[0],msg.data[1]},{msg.data[2],msg.data[3]}};
            int promoteTo = msg.data[4];
            // Detect capture: destination occupied OR en passant
            bool wasCapture = !game.isEmpty(mv.end);
            PieceID capturedPiece = wasCapture ? game.getPieceID(mv.end) : InvalidPiece;
            bool isEp = false;
            PieceID movedType = game.getPieceID(mv.start);
            if ((movedType==WPawn||movedType==BPawn) && mv.end.column!=mv.start.column && game.isEmpty(mv.end)) {
                Pos ep = game.getEnPassantTarget();
                if (mv.end.row==ep.row && mv.end.column==ep.column) {
                    wasCapture = true; isEp = true;
                    capturedPiece = (currentPlayer==White) ? BPawn : WPawn;
                }
            }
            PieceID promotePiece = (promoteTo >= 0 && promoteTo <= 11) ? (PieceID)promoteTo : InvalidPiece;
            game.movePiece(mv, promotePiece); lastMove=mv;
            if (wasCapture) {
                if (currentPlayer == White) capturedByWhite.push_back(capturedPiece);
                else capturedByBlack.push_back(capturedPiece);
                audio.playCapture();
            } else {
                audio.playMove();
            }
            (void)isEp;
            currentPlayer=(currentPlayer==White)?Black:White;
            checkGameEnd();
            if(gameOver) audio.playCheckmate(); else if(game.isInCheck(currentPlayer)) audio.playCheck();
            waitingForOpponent=false;
            break;
        }
        case NetMsgType::Miss:
            // Opponent attempted to capture a superposition ghost and missed.
            // Their turn ends; it is now our turn.
            audio.playMiss();
            currentPlayer=(currentPlayer==White)?Black:White;
            waitingForOpponent=false;
            checkGameEnd();
            if(gameOver) audio.playCheckmate();
            break;
        case NetMsgType::Disconnect: gameOver=true; gameOverMessage="Opponent disconnected"; waitingForOpponent=false; break;
        default: break;
        }
    }
}

void Window::handleLeftMouseDown() {
    if (gameOver||passClickRequired||waitingForOpponent) return;
    if (promotionPending) return;  // promotion dialog handles its own input
    auto sp = getSquare(getMouse());
    if (sp.row==-1) return;

    switch (gameState) {

    case selectPiece: {
        // Opponent ghost → collapse attempt
        if (game.isSuperpositionSquare(sp)&&game.hasActiveSuperposition()&&game.getSuperposition().color!=currentPlayer) {
            Pos orig=game.getSuperposition().originalPos;
            Pos result=game.collapseSuperposition();
            lastMove={orig,result};
            if (result==sp) {
                Move mv={moves.m1.start,result}; game.movePiece(mv); lastMove=mv; audio.playCapture();
                sendNetworkMove(mv, NetMsgType::Move);
            } else {
                audio.playMiss();
                sendNetworkMove({{-1,-1},{-1,-1}}, NetMsgType::Miss);
            }
            endTurn(); break;
        }
        // Own ghost → collapse
        if (game.isSuperpositionSquare(sp)&&game.hasActiveSuperposition()&&game.getSuperposition().color==currentPlayer) {
            Pos orig=game.getSuperposition().originalPos;
            game.collapseToPosition(sp);
            lastMove={orig,sp};
            sendNetworkMove({orig,sp}, NetMsgType::Move);
            endTurn(); break;
        }
        // Select own piece
        bool sel = !game.isEmpty(sp)&&getPieceColor(game.getPieceID(sp))==currentPlayer;
        if (!sel&&game.isSuperpositionOriginal(sp)) sel=game.getSuperposition().color==currentPlayer;
        if (!sel) { audio.playMoveInvalid(); break; }
        game.getPiecesMoves(sp,validMovePositions);
        if (game.isSuperpositionOriginal(sp)&&game.hasActiveSuperposition())
            for (auto& p : game.getSuperposition().positions)
                if (std::find(validMovePositions.begin(),validMovePositions.end(),p)==validMovePositions.end())
                    validMovePositions.push_back(p);
        if (validMovePositions.empty()) { audio.playMoveInvalid(); validMovePositions.clear(); break; }
        moves.m1.start=sp; moves.m2.start=sp;
        audio.playButtonClick();
        gameState=selectDest1;
        break;
    }

    case selectDest1: {
        // Re-select own piece
        bool sel = !game.isEmpty(sp)&&getPieceColor(game.getPieceID(sp))==currentPlayer;
        if (!sel&&game.isSuperpositionOriginal(sp)) sel=game.getSuperposition().color==currentPlayer;
        if (sel) {
            game.getPiecesMoves(sp,validMovePositions);
            if (game.isSuperpositionOriginal(sp)&&game.hasActiveSuperposition())
                for (auto& p : game.getSuperposition().positions)
                    if (std::find(validMovePositions.begin(),validMovePositions.end(),p)==validMovePositions.end())
                        validMovePositions.push_back(p);
            if (validMovePositions.empty()) { audio.playMoveInvalid(); validMovePositions.clear(); break; }
            moves.m1.start=sp; moves.m2.start=sp; audio.playButtonClick(); break;
        }
        // Validate
        if (std::find(validMovePositions.begin(),validMovePositions.end(),sp)==validMovePositions.end()) { audio.playMoveInvalid(); break; }
        // Opponent ghost → capture attempt
        if (game.isSuperpositionSquare(sp)&&game.hasActiveSuperposition()&&game.getSuperposition().color!=currentPlayer) {
            Pos orig=game.getSuperposition().originalPos; Pos result=game.collapseSuperposition();
            lastMove={orig,result};
            if (result==sp) {
                Move mv={moves.m1.start,result}; game.movePiece(mv); lastMove=mv; audio.playCapture();
                sendNetworkMove(mv, NetMsgType::Move);
            } else {
                audio.playMiss();
                sendNetworkMove({{-1,-1},{-1,-1}}, NetMsgType::Miss);
            }
            validMovePositions.clear(); endTurn(); break;
        }
        if (!game.isEmpty(sp)&&!game.isSuperpositionSquare(sp)) { audio.playMoveInvalid(); break; }
        moves.m1.end=sp;
        if (quantumMode) { audio.playButtonClick(); gameState=selectDest2; }
        else {
            validMovePositions.clear();
            if (game.isSuperpositionSquare(moves.m1.end)&&game.hasActiveSuperposition()) {
                game.collapseToPosition(moves.m1.end); lastMove=moves.m1;
                sendNetworkMove(moves.m1, NetMsgType::Move);
                endTurn();
            } else {
                executeNormalMove(moves.m1);
            }
        }
        break;
    }

    case selectDest2: {
        if (sp.row==moves.m1.end.row&&sp.column==moves.m1.end.column) { moves.m1.end={-1,-1}; gameState=selectDest1; break; }
        if (std::find(validMovePositions.begin(),validMovePositions.end(),sp)==validMovePositions.end()) { audio.playMoveInvalid(); break; }
        if (!game.isEmpty(sp)&&!game.isSuperpositionSquare(sp)) { audio.playMoveInvalid(); break; }
        moves.m2.end=sp;
        if (moves.m1.end==moves.m2.end) { audio.playMoveInvalid(); break; }
        validMovePositions.clear();
        audio.playQuantumEnter();
        std::vector<Pos> nd = {moves.m1.end, moves.m2.end};
        if (game.isSuperpositionOriginal(moves.m1.start)&&game.hasActiveSuperposition())
            game.addSuperpositionPositions(nd);
        else
            game.enterSuperposition(moves.m1.start,nd,game.getPieceID(moves.m1.start),currentPlayer);
        if (isNetworkGame()) {
            NetMessage qm; qm.type=NetMsgType::Quantum;
            qm.data[0]=moves.m1.start.row; qm.data[1]=moves.m1.start.column;
            qm.data[2]=moves.m1.end.row; qm.data[3]=moves.m1.end.column;
            qm.data[4]=moves.m2.end.row; qm.data[5]=moves.m2.end.column;
            net.sendMessage(qm);
        }
        endTurn();
        break;
    }

    default: break;
    }
}

void Window::checkGameEnd() {
    if (game.isCheckmate(currentPlayer)) {
        gameOverMessage="Checkmate! "+((currentPlayer==White)?playerNameBlack:playerNameWhite)+" wins!";
        gameOver=true;
    } else if (game.isStalemate(currentPlayer)) {
        gameOverMessage="Stalemate! It's a draw!";
        gameOver=true;
    }
}

void Window::restartGame() {
    game.resetBoard(); gameState=selectPiece; currentPlayer=White; gameOver=false; gameOverMessage.clear();
    validMovePositions.clear(); moves={}; lastMove={{-1,-1},{-1,-1}}; passClickRequired=false; waitingForOpponent=false;
    quantumMode=true; currentScreen=SCREEN_SETUP;
    moveHistory.clear(); capturedByWhite.clear(); capturedByBlack.clear();
    promotionPending=false; boardFlipped=false;
}

void Window::drawSuperpositionGhosts() {
    if (!game.hasActiveSuperposition()) return;
    auto& sup = game.getSuperposition();
    float sq = boardWidth/8;
    for (auto& p : sup.positions) {
        drawPiece(sup.pieceType, getSquarePosition(p), 0.4f);
        highlightSquare(p, Fade(PURPLE,0.2f));
    }
    for (size_t i=0; i<sup.positions.size(); ++i)
        for (size_t j=i+1; j<sup.positions.size(); ++j) {
            Vector2 a=getSquarePosition(sup.positions[i]), b=getSquarePosition(sup.positions[j]);
            a.x+=sq/2; a.y+=sq/2; b.x+=sq/2; b.y+=sq/2;
            DrawLineEx(a,b,3,Fade(PURPLE,0.5f));
        }
    int prob = game.getSuperpositionProbability();
    std::string pct = std::to_string(prob)+"%";
    int fs = std::max(14,(int)(sq/5));
    for (auto& p : sup.positions) {
        Vector2 pos = getSquarePosition(p);
        DrawText(pct.c_str(), (int)(pos.x+sq/2-MeasureText(pct.c_str(),fs)/2), (int)(pos.y+sq/2-fs/2), fs, WHITE);
    }
}

void Window::run(){
    while(!WindowShouldClose() && !shouldClose){
        pollEvents();
        render();
    }
}

Pos Window::getSquare(Vector2 cp) {
    if (cp.x<boardStart.x||cp.x>boardEnd.x||cp.y<boardStart.y||cp.y>boardEnd.y) return {-1,-1};
    float sq=boardWidth/8;
    int vr=(int)((cp.y-boardStart.y)/sq);
    int vc=(int)((cp.x-boardStart.x)/sq);
    if (vr<0||vr>=8||vc<0||vc>=8) return {-1,-1};
    return viewToSquare({vr, vc});
}

Pos Window::squareToView(Pos s) const {
    if (!boardFlipped) return s;
    return {7 - s.row, 7 - s.column};
}

Pos Window::viewToSquare(Pos v) const {
    // Flip is its own inverse.
    return squareToView(v);
}

Vector2 Window::getSquarePosition(Pos s){
    Pos v = squareToView(s);
    float sq=boardWidth/8;
    return {boardStart.x+v.column*sq,boardStart.y+v.row*sq};
}

Pos Window::getSquarePositionFlipped(Pos s){
    return squareToView(s);
}

void Window::resizedWindow(){ screenWidth=GetScreenWidth(); screenHeight=GetScreenHeight(); updateBoard(); }

void Window::updateBoard() {
    int w=GetScreenWidth(), h=GetScreenHeight();
    boardWidth=((w<h)?w:h)*0.95f;
    boardStart={(float)(w-boardWidth)/2,(float)(h-boardWidth)/2};
    boardEnd={boardStart.x+boardWidth,boardStart.y+boardWidth};
}

void Window::drawPiece(int p, Vector2 pos, float alpha){
    float sq=boardWidth/8; Color t=WHITE; t.a=(unsigned char)(alpha*255);
    DrawTexturePro(sprites[p],{0,0,spriteWidth,spriteHeight},{pos.x,pos.y,sq,sq},{0,0},0,t);
}

void Window::highlightSquare(Pos p, Color c){
    if(p.row==-1||p.column==-1) return; float sq=boardWidth/8;
    DrawRectangleV(getSquarePosition(p),{sq,sq},Fade(c,0.3f));
}

void Window::loadSprites(){
    sprites.resize(12);
    const char* paths[12] = {
        "assets/wk.png", "assets/wq.png", "assets/wb.png",
        "assets/wn.png", "assets/wr.png", "assets/wp.png",
        "assets/bk.png", "assets/bq.png", "assets/bb.png",
        "assets/bn.png", "assets/br.png", "assets/bp.png"
    };
    const PieceID order[12] = {WKing,WQueen,WBishop,WKnight,WRook,WPawn,
                               BKing,BQueen,BBishop,BKnight,BRook,BPawn};
    for (int i=0;i<12;i++) {
        sprites[order[i]] = LoadTexture(paths[i]);
        if (sprites[order[i]].id == 0) {
            TraceLog(LOG_WARNING, "Failed to load texture: %s", paths[i]);
        }
    }
}

void Window::drawCoordinateLabels() {
    float sq = boardWidth/8;
    int fs = std::max(10, (int)(sq/7));
    const char* files = "abcdefgh";
    const char* ranks = "87654321";
    for (int i=0;i<8;i++) {
        Pos vFile = squareToView({(boardFlipped?7:0), i});
        Vector2 fp = getSquarePosition(vFile);
        DrawText(&files[i], (int)(fp.x + sq - fs - 2), (int)(fp.y + sq - fs - 2), fs, Fade(GRAY, 0.8f));
        Pos vRank = squareToView({i, (boardFlipped?7:0)});
        Vector2 rp = getSquarePosition(vRank);
        DrawText(&ranks[i], (int)(rp.x + 2), (int)(rp.y + 2), fs, Fade(GRAY, 0.8f));
    }
}

void Window::drawMoveHistoryPanel() {
    if (moveHistory.empty()) return;
    int w = GetScreenWidth();
    int panelX = w - 230;
    if (panelX < boardEnd.x + 10) return;
    int panelW = 220;
    int lineH = 18;
    int maxLines = std::min((int)moveHistory.size(), 24);
    int panelH = maxLines * lineH + 30;
    DrawRectangle(panelX, 60, panelW, panelH, Fade(LIGHTGRAY, 0.9f));
    DrawRectangleLines(panelX, 60, panelW, panelH, DARKGRAY);
    DrawText("Moves", panelX + 8, 66, 16, DARKGRAY);
    int start = std::max(0, (int)moveHistory.size() - maxLines);
    for (int i = start; i < (int)moveHistory.size(); ++i) {
        int y = 88 + (i - start) * lineH;
        std::string line = std::to_string(i/2 + 1) + ". " + moveHistory[i].san;
        DrawText(line.c_str(), panelX + 8, y, 14, BLACK);
    }
}

void Window::drawCapturedPiecesPanel() {
    float sq = boardWidth/8;
    int iconSize = (int)(sq/3);
    int y = (int)boardEnd.y + 8;
    int x = (int)boardStart.x;
    if (y + iconSize > GetScreenHeight()) return;
    DrawText("Captured:", x, y, 14, DARKGRAY);
    int xw = x + 80;
    for (PieceID p : capturedByWhite) {
        DrawTexturePro(sprites[p], {0,0,spriteWidth,spriteHeight},
                       {(float)xw, (float)y, (float)iconSize, (float)iconSize},
                       {0,0}, 0, WHITE);
        xw += iconSize - 4;
    }
    int xb = x + 80;
    int yb = y + iconSize + 4;
    if (yb + iconSize > GetScreenHeight()) return;
    DrawText("by Black:", x, yb, 14, DARKGRAY);
    xb = x + 80;
    for (PieceID p : capturedByBlack) {
        DrawTexturePro(sprites[p], {0,0,spriteWidth,spriteHeight},
                       {(float)xb, (float)yb, (float)iconSize, (float)iconSize},
                       {0,0}, 0, WHITE);
        xb += iconSize - 4;
    }
}

void Window::drawDebugOverlay() {
    int fs = 14;
    int y = 110;
    DrawText(("FPS: " + std::to_string(GetFPS())).c_str(), 20, y, fs, GREEN); y += fs+2;
    DrawText(("State: " + std::to_string((int)gameState)).c_str(), 20, y, fs, GREEN); y += fs+2;
    Vector2 m = getMouse();
    Pos sq = getSquare(m);
    std::string ms = "Mouse: (" + std::to_string((int)m.x) + "," + std::to_string((int)m.y) + ") sq=(" +
                     std::to_string(sq.row) + "," + std::to_string(sq.column) + ")";
    DrawText(ms.c_str(), 20, y, fs, GREEN); y += fs+2;
    DrawText(("Player: " + std::string(currentPlayer==White?"White":"Black")).c_str(), 20, y, fs, GREEN); y += fs+2;
    DrawText(("Quantum: " + std::string(quantumMode?"ON":"OFF")).c_str(), 20, y, fs, GREEN); y += fs+2;
    DrawText(("Flipped: " + std::string(boardFlipped?"Y":"N")).c_str(), 20, y, fs, GREEN); y += fs+2;
    DrawText(("Net: " + std::string(isNetworkGame()?"Y":"N") + " wait=" + std::string(waitingForOpponent?"Y":"N")).c_str(), 20, y, fs, GREEN); y += fs+2;
    if (game.hasActiveSuperposition()) {
        DrawText(("Super: active prob=" + std::to_string(game.getSuperpositionProbability()) + "%").c_str(), 20, y, fs, PURPLE);
    }
}

void Window::drawPromotionDialog() {
    if (!promotionPending) return;
    int w = GetScreenWidth(), h = GetScreenHeight();
    DrawRectangle(0, 0, w, h, Fade(BLACK, 0.6f));
    const char* title = "Promote pawn to:";
    int tw = MeasureText(title, 28);
    DrawText(title, w/2 - tw/2, h/2 - 100, 28, WHITE);

    const PieceID choices[4] = {
        promotionColor==White?WQueen:BQueen,
        promotionColor==White?WRook:BRook,
        promotionColor==White?WBishop:BBishop,
        promotionColor==White?WKnight:BKnight
    };
    const char* labels[4] = {"Queen", "Rook", "Bishop", "Knight"};
    int bw = 110, bh = 110;
    int total = 4*bw + 3*12;
    int startX = w/2 - total/2;
    Vector2 mouse = getMouse();
    bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    for (int i=0;i<4;i++) {
        Rectangle r{(float)(startX + i*(bw+12)), (float)(h/2 - bh/2), (float)bw, (float)bh};
        bool hover = isInside(mouse, r);
        DrawRectangleRec(r, hover ? GRAY : DARKGRAY);
        DrawRectangleLines(r.x, r.y, r.width, r.height, BLACK);
        DrawTexturePro(sprites[choices[i]], {0,0,spriteWidth,spriteHeight},
                       {r.x+5, r.y+5, r.width-10, r.height-30}, {0,0}, 0, WHITE);
        DrawText(labels[i], (int)(r.x + (bw - MeasureText(labels[i],14))/2), (int)(r.y + bh - 22), 14, WHITE);
        if (hover && clicked) performPromotion(choices[i]);
    }
}

void Window::performPromotion(PieceID chosen) {
    promotionPending = false;
    bool wasCapture = !game.isEmpty(promotionMove.end);
    game.movePiece(promotionMove, chosen);
    lastMove = promotionMove;
    if (wasCapture) audio.playCapture(); else audio.playMove();
    recordMove(promotionMove, chosen, wasCapture);
    // Encode the chosen piece so the network peer promotes to the same type.
    sendNetworkMove(promotionMove, NetMsgType::Move, (int)chosen);
    endTurn();
}

std::string Window::moveToSAN(const Move& m, PieceID movedType, bool capture, bool check, bool mate) {
    if (movedType==WKing || movedType==BKing) {
        if (std::abs(m.end.column - m.start.column) == 2) {
            return (m.end.column == 6 ? "O-O" : "O-O-O");
        }
    }
    std::string s;
    char pieceLetter = ' ';
    switch (movedType) {
        case WKing: case BKing: pieceLetter='K'; break;
        case WQueen: case BQueen: pieceLetter='Q'; break;
        case WRook: case BRook: pieceLetter='R'; break;
        case WBishop: case BBishop: pieceLetter='B'; break;
        case WKnight: case BKnight: pieceLetter='N'; break;
        case WPawn: case BPawn: pieceLetter=' '; break;
        default: break;
    }
    if (pieceLetter != ' ') s += pieceLetter;
    if (capture) {
        if (pieceLetter == ' ') s += 'a' + m.start.column;
        s += 'x';
    }
    s += 'a' + m.end.column;
    s += '8' - m.end.row;
    if (mate) s += '#';
    else if (check) s += '+';
    return s;
}

void Window::recordMove(const Move& m, PieceID movedType, bool capture) {
    bool check = game.isInCheck(currentPlayer == White ? Black : White);
    // checkGameEnd already ran or will run; we approximate mate via checkmate() of the opponent-to-be
    bool mate = false;
    SquareColor movedColor = (currentPlayer == White) ? White : Black;
    SquareColor opp = (movedColor == White) ? Black : White;
    if (game.isCheckmate(opp)) mate = true;
    std::string san = moveToSAN(m, movedType, capture, check, mate);
    moveHistory.push_back({san, movedColor});
}
