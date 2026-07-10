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
    InitWindow(900, 700, "Quantum Chess");
    SetTargetFPS(30);
    screenWidth = 900; screenHeight = 700;
    int mon = GetCurrentMonitor();
    SetWindowPosition(GetMonitorWidth(mon)/2-screenWidth/2,
                      GetMonitorHeight(mon)/2-screenHeight/2);
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
    DrawText("Quantum Chess", w/2-MeasureText("Quantum Chess",60)/2, h/4, 60, DARKGRAY);

    const int bw=240, bh=45;
    int ypos[] = {h/2-80, h/2-10, h/2+60, h/2+130};
    const char* labels[] = {"Local Game", "Hot-Seat", "Host Game", "Join Game"};
    for (int i=0;i<4;i++) {
        Rectangle r={(float)(w/2-bw/2),(float)(ypos[i]-bh/2),(float)bw,(float)bh};
        DrawRectangleRec(r,DARKGRAY);
        DrawText(labels[i], (int)(r.x+(bw-MeasureText(labels[i],24))/2), (int)(r.y+12), 24, WHITE);
    }

    if (net.getRole()==NetworkRole::Host) {
        DrawText("Hosting port 5555...", w/2-120, h/2+200,18,DARKGRAY);
        if (net.isConnected()) DrawText("Opponent connected!", w/2-110, h/2+230,20,GREEN);
    }

    DrawText("ESC to quit", w/2-MeasureText("ESC to quit",16)/2, h-30,16,GRAY);
}

void Window::renderSetupScreen() {
    int w=GetScreenWidth(), h=GetScreenHeight();
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
        DrawRectangleRec(jb,DARKGRAY); DrawText("Connect",jb.x+50,jb.y+8,22,WHITE);
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
        DrawRectangleRec(sb,DARKGRAY); DrawText("Start Game",sb.x+30,sb.y+8,22,WHITE);
        DrawText("Click name, then type", w/2-120, h/2+140,16,GRAY);
    }
    Rectangle bb={(float)(w/2-110),(float)(h-90),220,50};
    DrawRectangleRec(bb,DARKGRAY); DrawText("Back",bb.x+80,bb.y+12,26,WHITE);
    DrawText("ESC=Back  ENTER=Start", w/2-MeasureText("ESC=Back  ENTER=Start",18)/2, h-25,18,GRAY);
    if (!networkStatus.empty()) DrawText(networkStatus.c_str(),w/2-MeasureText(networkStatus.c_str(),18)/2,h-120,18,RED);
}

void Window::renderGame() {
    BeginDrawing();
    ClearBackground(RAYWHITE);
    drawBoard();
    if (lastMove.start.row!=-1) {
        highlightSquare(lastMove.start, Fade(YELLOW,0.3f));
        highlightSquare(lastMove.end, Fade(YELLOW,0.3f));
    }
    for (auto cur : game.getPieces())
        drawPiece(cur.second, getSquarePosition(cur.first));
    drawSuperpositionGhosts();
        highlightSquare(getSquare(getMouse()));
    highlightMovesSelected();
    highlightCheckedKing();

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
        std::string pm = "Pass to "+((currentPlayer==White)?playerNameWhite:playerNameBlack)+" \u2014 Click";
        DrawText(pm.c_str(),GetScreenWidth()/2-MeasureText(pm.c_str(),36)/2,GetScreenHeight()/2-18,36,WHITE);
    }
    EndDrawing();
}

void Window::render() {
    if (currentScreen==SCREEN_TITLE) { BeginDrawing(); ClearBackground(RAYWHITE); renderTitleScreen(); EndDrawing(); }
    else if (currentScreen==SCREEN_SETUP) { BeginDrawing(); ClearBackground(RAYWHITE); renderSetupScreen(); EndDrawing(); }
    else renderGame();
}

Vector2 Window::getMouse() {
    Vector2 m = GetMousePosition();
    m.y += 22.0f;
    return m;
}

void Window::handleTitleClick() {
    Vector2 m = getMouse();
    int w=GetScreenWidth(), h=GetScreenHeight();
    const int bw=240, bh=45;
    int ypos[] = {h/2-80, h/2-10, h/2+60, h/2+130};
    for (int i=0;i<4;i++) {
        Rectangle r={(float)(w/2-bw/2),(float)(ypos[i]-bh/2),(float)bw,(float)bh};
        if (CheckCollisionPointRec(m, r)) {
            if (i==0) gameMode=MODE_LOCAL;
            else if (i==1) gameMode=MODE_HOTSEAT;
            else if (i==2) gameMode=MODE_NETWORK_HOST;
            else if (i==3) gameMode=MODE_NETWORK_CLIENT;
            currentScreen=SCREEN_SETUP;
            audio.playButtonClick();
            if (gameMode==MODE_NETWORK_HOST && net.startHost()) networkStatus="";
            return;
        }
    }
}

void Window::handleSetupClick() {
    Vector2 m = getMouse();
    int w=GetScreenWidth(), h=GetScreenHeight();

    if (gameMode==MODE_NETWORK_CLIENT) {
        if (isInside(m,{(float)(w/2-84),(float)(h/2-46),208,38})) { ipInputActive=0; return; }
        if (isInside(m,{(float)(w/2-84),(float)(h/2+14),208,38})) { ipInputActive=1; return; }
        if (isInside(m,{(float)(w/2-100),(float)(h/2+80),200,40})) {
            if (net.connectToHost(ipInput.c_str())) { networkStatus=""; playerNameBlack=playerNameWhite; playerNameWhite="Host"; startGame(); }
            else networkStatus="Failed connect";
            return;
        }
    } else if (gameMode==MODE_NETWORK_HOST) {
        if (isInside(m,{(float)(w/2-84),(float)(h/2-14),208,38})) { nameInputActive=0; return; }
        if (net.isConnected()) startGame();
        return;
    } else {
        if (isInside(m,{(float)(w/2-80),(float)(h/2-42),200,30})) { nameInputActive=0; return; }
        if (isInside(m,{(float)(w/2-80),(float)(h/2+18),200,30})) { nameInputActive=1; return; }
        // Any other click starts the game (works around coordinate offset)
        startGame();
        return;
    }
}

void Window::handleSetupKeyInput() {
    auto f = [](std::string& s, int mx){ int k; while((k=GetCharPressed())>0) if(k>=32&&k<=126&&(int)s.size()<mx) s.push_back((char)k); if(IsKeyPressed(KEY_BACKSPACE)&&!s.empty()) s.pop_back(); };
    if (gameMode==MODE_NETWORK_CLIENT) { if(ipInputActive==0) f(ipInput,20); else f(playerNameWhite,16); if(IsKeyPressed(KEY_ENTER)&&net.connectToHost(ipInput.c_str())){networkStatus="";playerNameBlack=playerNameWhite;playerNameWhite="Host";startGame();} else if(IsKeyPressed(KEY_ENTER)) networkStatus="Failed"; }
    else if (gameMode==MODE_NETWORK_HOST) { f(playerNameWhite,16); if(IsKeyPressed(KEY_ENTER)&&net.isConnected()) startGame(); }
    else { f(nameInputActive==0?playerNameWhite:playerNameBlack,16); if(IsKeyPressed(KEY_ENTER)) startGame(); }
}

void Window::startGame() {
    if (playerNameWhite.empty()) playerNameWhite="White";
    if (playerNameBlack.empty()) playerNameBlack="Black";
    game.resetBoard(); gameState=selectPiece; currentPlayer=White; gameOver=false; gameOverMessage="";
    validMovePositions.clear(); moves={}; lastMove={{-1,-1},{-1,-1}}; passClickRequired=false;
    waitingForOpponent=(gameMode==MODE_NETWORK_CLIENT); quantumMode=true; currentScreen=SCREEN_PLAYING;
}

void Window::sendNetworkMove(Move m, NetMsgType t) {
    if (!isNetworkGame()) return;
    NetMessage msg;
    msg.type = t;
    msg.data[0] = m.start.row; msg.data[1] = m.start.column;
    msg.data[2] = m.end.row; msg.data[3] = m.end.column;
    net.sendMessage(msg);
}

void Window::endTurn() {
    currentPlayer = (currentPlayer==White)?Black:White;
    checkGameEnd();
    if (gameOver) audio.playCheckmate();
    else if (game.isInCheck(currentPlayer)) audio.playCheck();
    if (gameMode==MODE_HOTSEAT&&!gameOver) passClickRequired=true;
    gameState = selectPiece;
}

void Window::pollEvents() {
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (currentScreen==SCREEN_PLAYING) { net.disconnect(); waitingForOpponent=false; gameMode=MODE_LOCAL; currentScreen=SCREEN_TITLE; }
        else if (currentScreen==SCREEN_TITLE) { net.disconnect(); CloseWindow(); }
        else { net.disconnect(); currentScreen=SCREEN_TITLE; }
    }
    if (IsWindowResized()) resizedWindow();
    bool mouseDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    bool mouseReleased = mousePressedLastFrame && !mouseDown;
    mousePressedLastFrame = mouseDown;
    if (currentScreen==SCREEN_TITLE) {
        if (mouseReleased) handleTitleClick();
        if (net.getRole()==NetworkRole::Host&&net.isConnected()) currentScreen=SCREEN_SETUP;
    } else if (currentScreen==SCREEN_SETUP) {
        handleSetupKeyInput();
        if (mouseReleased) handleSetupClick();
    } else if (currentScreen==SCREEN_PLAYING) {
        if (IsKeyPressed(KEY_R)&&!isNetworkGame()) restartGame();
        if (IsKeyPressed(KEY_Q)&&!gameOver&&!waitingForOpponent) { quantumMode=!quantumMode; audio.playButtonClick(); }
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
            game.enterSuperposition(o,d,game.getPieceID(o),side);
            currentPlayer=side; waitingForOpponent=false;
            break;
        }
        case NetMsgType::Move: {
            Move mv={{msg.data[0],msg.data[1]},{msg.data[2],msg.data[3]}};
            bool wc=!game.isEmpty(mv.end);
            game.movePiece(mv); lastMove=mv;
            if(wc) audio.playCapture(); else audio.playMove();
            currentPlayer=(currentPlayer==White)?Black:White;
            checkGameEnd();
            if(gameOver) audio.playCheckmate(); else if(game.isInCheck(currentPlayer)) audio.playCheck();
            waitingForOpponent=false;
            break;
        }
        case NetMsgType::Miss: waitingForOpponent=false; checkGameEnd(); if(gameOver) audio.playCheckmate(); break;
        case NetMsgType::Disconnect: gameOver=true; gameOverMessage="Opponent disconnected"; waitingForOpponent=false; break;
        default: break;
        }
    }
}

void Window::handleLeftMouseDown() {
    if (gameOver||passClickRequired||waitingForOpponent) return;
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
            if (game.isSuperpositionSquare(moves.m1.end)&&game.hasActiveSuperposition())
                { game.collapseToPosition(moves.m1.end); lastMove=moves.m1; sendNetworkMove(moves.m1, NetMsgType::Move); }
            else { bool wc=!game.isEmpty(moves.m1.end); game.movePiece(moves.m1); lastMove=moves.m1; sendNetworkMove(moves.m1, NetMsgType::Move); if(wc) audio.playCapture(); else audio.playMove(); }
            endTurn();
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
    game.resetBoard(); gameState=selectPiece; currentPlayer=White; gameOver=false; gameOverMessage="";
    validMovePositions.clear(); moves={}; lastMove={{-1,-1},{-1,-1}}; passClickRequired=false; waitingForOpponent=false;
    quantumMode=true; currentScreen=SCREEN_SETUP;
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

void Window::run(){ while(!WindowShouldClose()){ PollInputEvents(); pollEvents(); render(); } }

Pos Window::getSquare(Vector2 cp) {
    if (cp.x<boardStart.x||cp.x>boardEnd.x||cp.y<boardStart.y||cp.y>boardEnd.y) return {-1,-1};
    float sq=boardWidth/8;
    return {(int)((cp.y-boardStart.y)/sq),(int)((cp.x-boardStart.x)/sq)};
}

Vector2 Window::getSquarePosition(Pos s){ float sq=boardWidth/8; return {boardStart.x+s.column*sq,boardStart.y+s.row*sq}; }

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
    sprites[WPawn]=LoadTexture("assets/wp.png"); sprites[WKnight]=LoadTexture("assets/wn.png");
    sprites[WBishop]=LoadTexture("assets/wb.png"); sprites[WRook]=LoadTexture("assets/wr.png");
    sprites[WQueen]=LoadTexture("assets/wq.png"); sprites[WKing]=LoadTexture("assets/wk.png");
    sprites[BPawn]=LoadTexture("assets/bp.png"); sprites[BKnight]=LoadTexture("assets/bn.png");
    sprites[BBishop]=LoadTexture("assets/bb.png"); sprites[BRook]=LoadTexture("assets/br.png");
    sprites[BQueen]=LoadTexture("assets/bq.png"); sprites[BKing]=LoadTexture("assets/bk.png");
}
