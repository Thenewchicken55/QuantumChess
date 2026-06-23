# QuantumChess

A chess game with a quantum twist — when you move, you pick **two** possible moves and one is chosen at random.

## How to Play

1. **Select your first move**: Click a piece, then click a valid destination (highlighted in red).
2. **Select your second move**: Click another piece (or same piece), then click a valid destination.
3. **The quantum coin flip**: One of your two selected moves is chosen randomly and executed.
4. Turn passes to the opponent.

Special moves supported: castling, en passant, pawn promotion (auto-queen).

## Building

### Prerequisites
- CMake 3.30+
- C++ compiler with C++17 support
- Raylib (fetched automatically by CMake)

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
- **Left click**: Select pieces and squares
- **R**: Restart game after checkmate/stalemate
- **Escape**: Quit
