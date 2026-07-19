# QuantumChess

Chess with a quantum twist — pieces exist in **superposition** and their true
position is hidden until a **critical event** forces the quantum state to collapse.

## The Quantum Mechanic

Unlike standard chess where every move is deterministic, QuantumChess lets you put
pieces into **superposition**:

1. **Select a piece** — click one of your pieces
2. **Pick destinations** — choose **one or two** target squares for that piece
3. **Superposition** — if you pick two squares, your piece exists on **both squares
   simultaneously**, shown as a ghosted overlay
4. **Collapse** — neither player knows which square the piece is actually on until
   a critical event reveals it:
   - An opponent tries to **capture** the piece
   - The piece is used to **capture** another piece
   - The owning player **collapses it manually** by clicking a ghost square
   - Both states collapse randomly (50/50) into one position

This creates uncertainty, bluffing opportunities, and entirely new tactical depth.

### Example
Your knight at e4 can move to either f6 or g5. You select both. The knight now
appears faded on **both** f6 and g5. If your opponent moves a pawn to attack f6 —
the knight collapses: if it was at f6, it's captured; if at g5, it survives and the
opponent wasted a move.

## Current Status

All core features are implemented:

- Full chess rules (all piece movements, castling, en passant, **pawn promotion
  with piece choice**, check/checkmate/stalemate detection)
- Quantum superposition with hidden state until capture/collapse
- Local, hot-seat, and TCP network multiplayer
- Sound effects (move, capture, quantum enter/collapse, check, checkmate, miss)
- Move history panel (algebraic notation), captured-pieces tray, coordinate labels
- Board flip (Black's perspective), debug overlay, audio mute toggle
- raylib 5.5 rendering

See `docs/REVIEW.md` for a full code review and `docs/plan.md` for the design plan.

## Building

### Prerequisites
- CMake 3.30+
- C++17 compiler
- Raylib 5.5 (fetched automatically via FetchContent)

### Build & Run
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target QuantumChess -j 4
./build/QuantumChess
```

Or via the convenience target (runs from the project root so `assets/` resolve):
```bash
cmake --build build --target run
```

### Assets
Sprites (`assets/*.png`) and sounds (`assets/*.wav`) are loaded via relative paths,
so run the binary from the project root (the `run` target does this for you).

### Build types
`CMAKE_BUILD_TYPE` defaults to `Release`. Pass `-DCMAKE_BUILD_TYPE=Debug` for a
debug build with assertions. Compiler warnings are always on via the `qc_warnings`
interface target.

## Controls

| Input | Action |
|-------|--------|
| Left click | Select piece / destination / dialog button |
| `Q` | Toggle quantum (superposition) vs normal move mode |
| `F` | Flip board perspective |
| `R` | Restart game (local / hot-seat only) |
| `M` | Toggle audio mute |
| `F1` | Toggle debug overlay (FPS, state, coords) |
| `L` / `H` / `O` / `J` | Title-screen shortcuts: Local / Hot-seat / hOst / Join |
| Escape | Back / quit (context-sensitive) |

## Game Modes

- **Local**: one player, both sides visible (good for analysis).
- **Hot-Seat**: pass-and-play with a "pass to [player]" blind between turns.
- **Host / Join**: TCP multiplayer. Host listens on port 5555; the client enters
  the host's IP. Both sides run the board independently and sync moves over the
  text protocol (see `docs/REVIEW.md`).

## License

See `LICENSE`.
