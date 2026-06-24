# QuantumChess

Chess with a quantum twist — pieces exist in **superposition** and their true position is hidden until a **critical event** forces the quantum state to collapse.

## The Quantum Mechanic

Unlike standard chess where every move is deterministic, QuantumChess lets you put pieces into **superposition**:

1. **Select a piece** — click one of your pieces
2. **Pick destinations** — choose **one or two** target squares for that piece
3. **Superposition** — if you pick two squares, your piece exists on **both squares simultaneously**, shown as a ghosted overlay
4. **Collapse** — neither player knows which square the piece is actually on until a critical event reveals it:
   - An opponent tries to **capture** the piece
   - The piece is used to **capture** another piece
   - Both states collapse randomly (50/50) into one position

This creates uncertainty, bluffing opportunities, and entirely new tactical depth.

### Example
Your knight at e4 can move to either f6 or g5. You select both. The knight now appears faded on **both** f6 and g5. If your opponent moves a pawn to attack f6 — the knight collapses: if it was at f6, it's captured; if at g5, it survives and the opponent wasted a move.

## Current Status

```
Phase 1: Foundation      ✅ Complete
Phase 2: True Quantum    🔨 In Progress
Phase 3: Multiplayer     📝 Planned
Phase 4: Sound & FX      📝 Planned
Phase 5: Visual Polish   📝 Planned
```

### What's Implemented (Phase 1)
- Full chess rules (all piece movements, bounds/blocking)
- Check, checkmate, stalemate detection
- Castling, en passant, pawn promotion
- Turn management and game-over states
- Move validation (can't leave king in check)
- Basic raylib rendering with piece sprites

### What's Coming
- **True superposition** with hidden state until capture/collapse
- **Local hot-seat** and **online multiplayer**
- **Sound effects** (move, capture, quantum collapse, check/checkmate)
- **Visual polish** (animations, particle effects, quantum glow, styled UI)

## Building

### Prerequisites
- CMake 3.30+
- C++17 compiler
- Raylib (fetched automatically)

### Build & Run
```bash
cmake -B build
cmake --build build
./build/QuantumChess
```

Or use the convenience target:
```bash
cmake -B build
cmake --build build
make run
```

### Controls
| Input | Action |
|-------|--------|
| Left click | Select piece / destination |
| R | Restart game |
| Escape | Quit |
