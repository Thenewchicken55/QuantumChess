# QuantumChess — Implementation Plan

## Game Design

QuantumChess is standard chess with one twist: when you move a piece, you may put it
into **quantum superposition** by selecting **two possible destinations**. The piece
exists on **both squares** simultaneously, shown as a ghosted overlay, and its true
position is hidden from **both players** until a **critical event** forces it to
collapse.

This creates bluffing, uncertainty, and tactical depth:
- Opponent might waste a turn attacking a square the piece isn't actually on
- You might get lucky and survive a seemingly inevitable capture
- You might get unlucky and lose a piece you thought was safe

---

## Phase 1: Foundation ✅ COMPLETE

- [x] Chess movement rules (all pieces, bounds, blocking)
- [x] Standard chess rules (castling, en passant, promotion)
- [x] Check / checkmate / stalemate detection
- [x] Move validation (can't leave king in check)
- [x] Turn management
- [x] Basic raylib rendering with sprite assets
- [x] Resizable window
- [x] Game-over state + restart

---

## Phase 2: True Quantum Superposition ✅ COMPLETE

### 2a. Data Model — `Board` changes ✅

- [x] `SuperpositionState` struct added to `types.h` (includes `Pos originalPos`, `Pos pos1`, `Pos pos2`, `PieceID pieceType`, `SquareColor color`)
- [x] `SuperpositionState` member added to `Board` (in `board.h`)
- [x] Accessors: `hasActiveSuperposition()`, `getSuperposition()`, `clearSuperposition()`, `isSuperpositionSquare()`, `isSuperpositionOriginal()`
- [x] `enterSuperposition(Pos original, Pos dest1, Pos dest2, PieceID type, SquareColor color)` 
- [x] `collapseSuperposition()` → returns `Pos` of actual position (50/50 using `std::mt19937`)

### 2b. Board occupation rules for superposition ✅

- [x] Ghost positions count as occupied (`isEmpty()` returns `false` for pos1/pos2)
- [x] Original position treated as empty (`isEmpty()` returns `true`, `getPieceID()` returns `InvalidPiece`)
- [x] Enemy may target EITHER ghost position for capture (via `getPieceID()` returning piece type for ghosts)
- [x] Collapse triggers:
  - Opponent attempts capture on either superposition square
  - The owning player's next turn after opponent's normal move (auto-collapse)

### 2c. Collapse logic ✅

- [x] `collapseSuperposition()` called on critical events
- [x] 50/50 random selection between `pos1` and `pos2`
- [x] If opponent was capturing the collapsed square → capture succeeds (via `movePiece`)
- [x] If opponent was capturing the other square → capture fails (move not executed, opponent loses turn)
- [x] Ghost piece removed from board after collapse
- [x] `collapseSuperposition()` physically moves the piece from `originalPos` to chosen position via `movePiece`

### 2d. State machine — `Window` changes ✅

Replaced the old states with:

```
New states:
  quantumPickFirst → quantumPickDest1 → quantumPickSecond → quantumPickDest2
    → opponentPickPiece → opponentPickDest
    → if destination is superposition square:
        collapseSuperposition → resolveCaptureOrMiss → quantumPickFirst (next player)
    → if normal move + superposition active:
        movePiece → collapseSuperposition (auto) → quantumPickFirst (next player)
    → if normal move only:
        movePiece → quantumPickFirst (next player)
```

New state enum: `quantumPickFirst`, `quantumPickDest1`, `quantumPickSecond`, `quantumPickDest2`, `opponentPickPiece`, `opponentPickDest`, `superpositionResolve`, `gameEnded`

### 2e. Rendering — superposition display ✅

- [x] Draw ghosted piece (alpha 0.3) on original position
- [x] Draw ghosted piece (alpha 0.4) on BOTH superposition squares
- [x] Entanglement line drawn between the two superposition squares (`DrawLineEx` with `Fade(PURPLE, 0.5f)`)
- [x] Superposition squares highlighted with purple tint
- [x] `getPieces()` skips the original position during superposition (rendered separately as ghost)
- [x] Collapse animation and particle effects deferred: Phase 5 (visual polish)
- [x] "Miss!" text deferred: currently handled implicitly via turn text change

---

## Phase 3: Sound Effects

### 3a. Setup

- [ ] Initialize raylib audio device (`InitAudioDevice()`)
- [ ] Load sound assets at startup
- [ ] Audio cleanup in destructor

### 3b. Sound assets (generate or use placeholder tones)

- [ ] `move.wav` — piece movement
- [ ] `capture.wav` — piece capture
- [ ] `quantum_enter.wav` — entering superposition (ethereal tone)
- [ ] `quantum_collapse.wav` — superposition collapse (impact + shimmer)
- [ ] `check.wav` — check notification
- [ ] `checkmate.wav` — game over fanfare
- [ ] `miss.wav` — capture miss sound
- [ ] `button_click.wav` — UI interaction
- [ ] `move_invalid.wav` — invalid move buzz

### 3c. Sound triggers

- [ ] Play on piece select, move execute, capture, collapse, check, etc.
- [ ] Volume control option in settings

---

## Phase 4: Multiplayer

### 4a. Hot-seat (local pass-and-play)

- [ ] Turn prompt: "Pass the computer to [player name]" overlay
- [ ] Optional: blindfold mode where screens hide during opponent's turn
- [ ] Player name entry screen

### 4b. Network multiplayer

- [ ] Choose: host (server) or join (client)
- [ ] Game state sync protocol (JSON over TCP or flat structs):
  - Moves: `{type: "move", piece, from, to, superposition}`
  - Collapse events: `{type: "collapse", piece, resulting_pos}`
  - Game state: `{type: "state", board, turn, superpositions}`
- [ ] Server: accepts connection, manages game, validates moves
- [ ] Client: sends moves, receives state updates
- [ ] Disconnection handling + reconnection
- [ ] Matchmaking lobby UI (simple: enter IP to connect)

---

## Phase 5: Visual Polish

### 5a. Animations

- [ ] Smooth piece movement (lerp between squares, ~200ms)
- [ ] Capture animation (attacker moves in, captured piece fades out)
- [ ] Collapse animation (ghosts merge, particle burst, screen shake)
- [ ] Check highlight pulse
- [ ] Turn transition fade

### 5b. UI

- [ ] Styled buttons with hover/press states (raygui or custom)
- [ ] Game info panel (move history, captured pieces, timer)
- [ ] Move confirmation dialog
- [ ] Settings menu (sound volume, board orientation, theme)
- [ ] Title screen with "Play", "Multiplayer", "Settings"
- [ ] Win/loss screen with stats

### 5c. Effects

- [ ] Quantum glow shader (pulsing color on superposition pieces)
- [ ] Particle system for collapses and captures
- [ ] Screen shake on captures / collapses
- [ ] Board theme selector (wood, marble, dark mode)
- [ ] Piece animation on select (slight bob/glow)

---

## Architecture Map

```
src/
├── main.cpp           — entry point
├── board.cpp/.h       — game state, rules, quantum mechanics
├── types.cpp/.h       — enums, structs, utility
├── window.cpp/.h      — rendering, input, UI, state machine
├── pieces/
│   ├── piece.cpp/.h   — base class
│   ├── pawn.cpp/.h
│   ├── rook.cpp/.h
│   ├── knight.cpp/.h
│   ├── bishop.cpp/.h
│   ├── queen.cpp/.h
│   └── king.cpp/.h
├── audio.cpp/.h       — sound loading and playback    [NEW]
├── particles.cpp/.h   — particle effects              [NEW]
├── network.cpp/.h     — multiplayer networking        [NEW]
├── menu.cpp/.h        — UI screens                    [NEW]
├── animation.cpp/.h   — animation system              [NEW]
└── shaders/           — GLSL shader files             [NEW]
```

### New directory layout
```
include/
├── main/         — existing: board.h, types.h, window.h
├── pieces/       — existing: all piece headers
└── quantum/      — NEW: audio.h, particles.h, network.h, menu.h, animation.h
assets/
├── *.png         — existing piece sprites
└── sounds/       — NEW: .wav sound files
```

---

## File-by-file change summary

| File | Phase | Change |
|------|-------|--------|
| `board.h` | 2 | Add `SuperpositionState`, accessors, collapse logic |
| `board.cpp` | 2 | Implement superposition occupation, collapse triggers |
| `types.h` | 2 | Add new `States` values for superposition flow |
| `window.h` | 2 | Restructure state machine, add superposition member |
| `window.cpp` | 2, 3, 5 | New state machine, rendering, animations, sound calls |
| `CMakeLists.txt` | 3+ | Add new source files and include dirs |
| `audio.h/cpp` | 3 | Sound system |
| `particles.h/cpp` | 5 | Particle effects |
| `network.h/cpp` | 4 | Multiplayer |
| `menu.h/cpp` | 5 | Menu screens |
| `animation.h/cpp` | 5 | Animation system |
