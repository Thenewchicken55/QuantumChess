# AGENTS.md — QuantumChess

Project-specific instructions for AI coding agents working on QuantumChess.
Extends the parent `/Users/eliaa/Code/AGENTS.md`.

## Build & Run

```bash
# Configure (first time, or after changing CMakeLists.txt). Defaults to Release.
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build the binary only (raylib is already fetched after first configure)
cmake --build build --target QuantumChess -j 4

# Run from the project root (assets/ are resolved relative to CWD)
./build/QuantumChess

# Or via the convenience target (runs from ${CMAKE_SOURCE_DIR})
cmake --build build --target run
```

There is no test suite yet (see `docs/REVIEW.md` §5.3 for the plan). To verify
your changes: build with the target above (warnings are on) and smoke-test the
binary manually.

## Lint / Typecheck

- Compiler warnings are enabled via the `qc_warnings` interface target
  (`-Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor -Woverloaded-virtual`).
  A clean build should produce **no warnings from our code** (raylib's own
  headers are noisy — that's expected and ignored).
- `compile_commands.json` is generated in `build/` for clangd.

## Architecture

```
src/
  main/main.cpp      — entry point (creates Window, calls run())
  main/board.cpp     — game state, chess rules, quantum superposition mechanics
  main/types.cpp     — enums (SquareColor, PieceID, States) and Pos helpers
  main/window.cpp    — raylib rendering, input handling, UI, state machine, networking glue
  main/audio.cpp     — sound loading/playback (AudioManager)
  main/network.cpp   — TCP host/client multiplayer (NetworkManager)
  pieces/*.cpp       — Piece subclasses (Pawn/Rook/Knight/Bishop/Queen/King)
include/
  main/  pieces/     — matching headers
assets/              — PNG sprites + WAV sounds (loaded via relative paths)
docs/                — REVIEW.md (code review), plan.md (design plan)
```

### Key invariants
- `Board` owns pieces in `std::unique_ptr<Piece> pieces[8][8]`.
- During superposition, the piece stays at `originalPos` but `isEmpty(originalPos)`
  returns `true` and `getPieceID(originalPos)` returns `InvalidPiece`. The ghost
  squares are virtual — `isEmpty(ghost)` returns `false`, `getPieceID(ghost)`
  returns the piece type.
- `Piece::getAttackSquares()` (default = `getValidMoves()`, overridden in Pawn)
  is used for check/castling-through-check detection. Don't use `getValidMoves()`
  for attack detection — pawn forward moves aren't attacks.
- `Window` is flip-aware: `squareToView`/`viewToSquare` convert between logical
  board coords and screen coords. All rendering and click detection go through
  `getSquarePosition`/`getSquare`, which apply the flip automatically.
- The network protocol is text-based, newline-delimited:
  `MOVE|sr|sc|er|ec|pt` (pt=promoteTo, optional), `QUANTUM|sr|sc|er1|ec1|er2|ec2`,
  `MISS`, `DISCONNECT`. See `network.cpp`.

## Coding conventions

- C++17. Prefer `static_cast<T>` over C-style casts (existing C-style casts in
  `window.cpp` are pre-existing raylib-interop cruft — don't add more, but don't
  churn through converting them either without a dedicated PR).
- Initialize members with default values in the class body (e.g. `int x = -1;`).
- Mark read-only `Board`/`Window` methods `const`.
- Use `std::make_unique` for pieces; never `new`/`delete`.
- No `using namespace std;` in headers.

## Before committing

1. `cmake --build build --target QuantumChess -j 4` — must succeed with no
   warnings from our code.
2. Smoke-test the binary: launch, start a local game, make a few moves, try a
   quantum superposition, trigger a collapse.
3. If you change the network protocol, update both `sendMessage` and
   `receiveMessage`, and document the change in `docs/REVIEW.md`.
4. If you touch `Board` rules, double-check check/castling/promotion detection.

## Known issues / future work

See `docs/REVIEW.md` §7 "Remaining / future work". The biggest open item is the
`enum class` conversion (§2.1) — coordinate with the maintainers before starting.
