# QuantumChess — Code Review & Improvement Plan

A thorough review of the codebase covering correctness, architecture, code quality,
build system, features, and maintainability. Each finding is tagged with severity:
**[CRITICAL]**, **[HIGH]**, **[MEDIUM]**, **[LOW]**.

---

## 1. Critical Bugs

### 1.1 [CRITICAL] Network: `endTurn()` never sets `waitingForOpponent`
`window.cpp:235-242` — after a host/client makes a move, `endTurn()` toggles
`currentPlayer` but never sets `waitingForOpponent = true` for network games. The
player who just moved can immediately move the *opponent's* pieces until the
opponent's network message arrives. This is a game-breaking exploit.

### 1.2 [CRITICAL] Network: `Miss` handler doesn't toggle `currentPlayer`
`window.cpp:298` — `case NetMsgType::Miss` sets `waitingForOpponent=false` but does
not toggle `currentPlayer`. The two clients desync: the player whose turn it now
should be can't actually move because locally their turn was never started.

### 1.3 [CRITICAL] Network: Quantum message uses `getPieceID(originalPos)` which returns `InvalidPiece`
`window.cpp:283` — when an opponent re-enters superposition (already in
superposition), the receiver calls `game.enterSuperposition(o, d, game.getPieceID(o), side)`,
but `getPieceID(originalPos)` returns `InvalidPiece` by design. This corrupts the
superposition state on the receiver.

### 1.4 [CRITICAL] `isSquareAttacked` is wrong for pawns
`board.cpp:302-317` iterates `pieces[i][j]->getValidMoves()`. For pawns,
`getValidMoves()` returns forward moves (only when empty) and diagonal captures
(only when an enemy piece is present). This means a pawn "attacks" a square only if
an enemy piece is already there — so castling-through-check detection silently fails
when the transit square is empty. The fix is a separate `getAttackSquares()` that for
pawns returns the two diagonal squares unconditionally.

### 1.5 [CRITICAL] Castling doesn't verify piece types
`board.cpp:330-403` — `canCastleKingSide`/`canCastleQueenSide` check `pieces[row][4]`
and `pieces[row][7]` exist and haven't moved, but never check they are actually the
King and Rook. A promoted rook or moved-then-returned piece would falsely allow
castling. Must verify `getType()` matches.

### 1.6 [CRITICAL] Title screen buttons are not clickable
`window.cpp:180-184` — `handleTitleClick()` unconditionally sets `gameMode =
MODE_LOCAL`. The four buttons ("Local", "Hot-Seat", "Host", "Join") rendered in
`renderTitleScreen` are purely decorative; only the keyboard shortcuts L/H/O/J work.
This is a major UX bug.

### 1.7 [HIGH] BACK button in setup screen is non-functional
`window.cpp:126-128` draws a BACK button, but `handleSetupClick()` never tests for
it. There is no way to go back to the title screen except pressing ESC.

### 1.8 [HIGH] `getMouse()` adds `+22` to Y as a macOS title-bar hack
`window.cpp:174-178` — this is fragile, breaks on other platforms/DPI settings, and
masks a real input bug. raylib's `GetMousePosition()` already returns client-area
coordinates. The git log shows this was added to work around an issue caused by
`SetWindowPosition`; the root cause should be fixed instead.

### 1.9 [HIGH] ESC in title screen calls `CloseWindow()` directly
`window.cpp:247` — `CloseWindow()` is called mid-loop, then the destructor calls it
again (double-close). Should set a flag and let `WindowShouldClose()` exit the loop
naturally.

### 1.10 [MEDIUM] Other correctness bugs
- `board.cpp:431-434` — `getSuperpositionProbability()` uses integer division
  (`100/3 = 33`), so multi-position superpositions show wrong probabilities.
- `window.cpp:386` — en passant captures don't play the capture sound (`wc` checks
  destination occupancy which is empty for en passant).
- `window.cpp:163` — overlay uses `\u2014` (em dash) which raylib's default font
  cannot render.
- `window.cpp:21` — `SetTargetFPS(30)` is too low for a game; should be 60.
- `window.cpp:286` — Quantum network case doesn't play audio or set `lastMove`.

---

## 2. Architecture & Code Quality

### 2.1 [HIGH] Unscoped enums cause name pollution
`types.h:6-26` — `SquareColor{White,Black,...}`, `PieceID{WKing,...}`, and
`States{selectPiece,...}` are unscoped. `White`/`Black`/`selectPiece` collide with
common identifiers and risk implicit conversion to int. **Fix**: convert to
`enum class`.

> **Status: deferred.** This is a ~150-usage mechanical refactor across every
> source file. `PieceID` is used as an array index (`sprites[p]`) and would need
> `static_cast<int>(p)` at each index site. The risk/reward of rushing this in a
> single pass is poor — it's best done as a dedicated, reviewed PR. A helper
> `inline int pieceIndex(PieceID p) { return static_cast<int>(p); }` should be
> introduced first to keep the call sites clean.

### 2.2 [HIGH] `Pos` has no default constructor
`types.h:30-37` — `Pos p;` leaves `row`/`column` uninitialized. Should default to
`{-1,-1}` (invalid) and add `isValid()`, `operator!=`, and a hash for `std::unordered_map`.

### 2.3 [MEDIUM] Missing bounds/null checks
- `board.cpp:200-220` `getPiecesMoves` dereferences `pieces[r][c]` without null check.
- `board.cpp:196-198` `getRawPieceMoves` doesn't bounds-check `piecePos`.
- `window.cpp:488-496` `loadSprites` doesn't check `LoadTexture` results.
- `audio.cpp:32-35` `loadSound` doesn't check `LoadSound` results.

### 2.4 [MEDIUM] Const-correctness gaps
Several `Board` accessors could be `const` but aren't (`isEmpty`, `isLegalMove`).
`getPieces()` returns by value instead of `const std::vector<...>`.

### 2.5 [MEDIUM] Static state and magic numbers in functions
- `board.cpp:439` — `static std::mt19937 rng` inside `collapseSuperposition`. Should
  be a member for testability.
- `network.cpp:156-157` — `static char partialBuf[4096]; static int partialLen` in
  `receiveMessage` — shared across all instances, not thread-safe. Should be members.
- `window.cpp:176` — `m.y += 22.0f` magic number.

### 2.6 [MEDIUM] Redundant input polling
`window.cpp:459` — `run()` calls `PollInputEvents()` then `pollEvents()`. But
`WindowShouldClose()` already calls `PollInputEvents()` internally, so events are
polled twice. Should just use `WindowShouldClose()`.

### 2.7 [LOW] Inconsistent by-value loops
`window.cpp:140` — `for (auto cur : game.getPieces())` copies pairs. Use `const auto&`.

### 2.8 [LOW] Inconsistent screen-size initialization
`window.h:28-29` defaults `screenWidth=1200`, `screenHeight=900`, but the
constructor overrides to `900x700`. The members are then kept in sync manually.

### 2.9 [LOW] `findKing` returns `{-1,-1}` to mean "not found"
`board.cpp:290-300` — ambiguous (could be a valid position). Should return
`std::optional<Pos>`.

### 2.10 [LOW] Large functions
`window.cpp::handleLeftMouseDown` (~115 lines) and `processNetworkMessages` (~30
lines) mix many concerns (selection, collapse, network sync, audio). Should be
decomposed into smaller methods.

---

## 3. Build System

### 3.1 [HIGH] raylib pinned to `master`
`CMakeLists.txt:16` — `GIT_TAG "master"` is unstable; a future raylib commit can
break the build silently. Pin to a stable tag (e.g. `5.5`).

### 3.2 [HIGH] Hardcoded `CMAKE_BUILD_TYPE Debug`
`CMakeLists.txt:3` — overrides any user-specified configuration. Should be a
default that users can override (`if(NOT CMAKE_BUILD_TYPE) set(CMAKE_BUILD_TYPE Release)`).

### 3.3 [HIGH] No compiler warnings enabled
No `-Wall -Wextra -Wpedantic`. Many bugs would surface with warnings.

### 3.4 [MEDIUM] Asset path fragility
Sprites/sounds are loaded with relative paths (`assets/...`). The `make run` target
sets `WORKING_DIRECTORY ${CMAKE_BINARY_DIR}` (i.e. `build/`), so `make run` fails to
find assets. Should either output the binary next to assets, or copy assets into
the build dir, or embed them.

### 3.5 [MEDIUM] No `compile_commands.json` export
Useful for clangd/IDE tooling. Add `set(CMAKE_EXPORT_COMPILE_COMMANDS ON)`.

### 3.6 [LOW] `BUILD_GAMES` set with no value
`CMakeLists.txt:11` — `set(BUILD_GAMES)` sets it to the empty string, not OFF. Looks
like a typo (should be `set(BUILD_GAMES OFF CACHE BOOL "" FORCE)`).

---

## 4. Missing Features

### 4.1 [HIGH] No pawn promotion choice
`board.cpp:148-152` — pawns always promote to Queen. Should show a UI to pick
Q/R/B/N.

### 4.2 [HIGH] No board flip
In hot-seat or as Black, the board should flip to the player's perspective.

### 4.3 [MEDIUM] No coordinate labels (a-h, 1-8)
Standard chess UX; helps readability.

### 4.4 [MEDIUM] No move history
No way to review the game. Should track moves in algebraic notation.

### 4.5 [MEDIUM] No captured pieces display
Visual feedback of material balance.

### 4.6 [MEDIUM] No settings (volume, theme)
`AudioManager::setVolume` exists but is never called from UI.

### 4.7 [MEDIUM] No pause/resign/restart menu mid-game
Only ESC (which quits to title) and R (restart).

### 4.8 [MEDIUM] No debug overlay
Helpful for development (FPS, state, mouse coords).

### 4.9 [LOW] Animations, particles, screen shake, AI, save/load
Documented in `docs/plan.md` Phase 5; out of scope for this pass.

---

## 5. Maintainability

### 5.1 [HIGH] No `AGENTS.md` with build/test commands
The repo has an `AGENTS.md` from the parent dir but no project-specific build/test
instructions. Should add one.

### 5.2 [HIGH] README is stale
`README.md` claims phases 2/3/4 are "planned/in-progress" but `docs/plan.md` marks
them complete. README needs to match reality.

### 5.3 [MEDIUM] No tests
No test framework or tests at all. Chess engines are highly testable; should add
Catch2 + tests for `Board` (movement, check, castling, en passant, superposition).

### 5.4 [MEDIUM] `docs/plan.md` is stale
Mixes ✅-complete and 🔨-in-progress items that are actually done.

### 5.5 [LOW] No `.clang-format` / `.clang-tidy`
Would enforce consistent style and catch bugs.

---

## 6. Implementation Plan (Execution Order)

1. **Phase A — Critical bug fixes** (Section 1): fix all critical/high correctness
   bugs first so the game actually plays correctly.
2. **Phase B — Type safety** (2.1, 2.2): convert enums to `enum class`, improve `Pos`.
3. **Phase C — Hardening** (2.3–2.10): bounds/null/const fixes, member RNG, remove
   static buffers, fix input polling.
4. **Phase D — Build system** (Section 3): warnings, pinned raylib, configurable
   build type, asset handling, compile_commands.
5. **Phase E — Features** (Section 4): promotion UI, move history, captured pieces,
   board flip, coordinates, settings, pause menu, debug overlay.
6. **Phase F — Docs** (Section 5): AGENTS.md, README, plan.md, .clang-format.
7. **Phase G — Verify**: full clean build, smoke-test the binary.

---

## 7. What Was Implemented (This Pass)

All Phase A–F items except the `enum class` refactor (deferred, see 2.1).

### Phase A — Critical bug fixes
- `isSquareAttacked` now uses a new `Piece::getAttackSquares()` virtual (overridden
  in `Pawn` to return only diagonal squares), so castling-through-check and
  king-safety detection are correct.
- `canCastleKingSide`/`canCastleQueenSide` now verify the king and rook are
  actually King/Rook of the right color (not just "some unmoved piece").
- `Window::endTurn()` now sets `waitingForOpponent = true` for network games,
  closing the "move the opponent's pieces" exploit.
- Network `Miss` handler now toggles `currentPlayer` and plays the miss sound.
- Network `Quantum` handler detects an already-active superposition and uses
  `addSuperpositionPositions` + the existing `pieceType` instead of clobbering
  with `InvalidPiece`.
- Title-screen buttons are now clickable (not just keyboard shortcuts), via a
  shared `drawButton`/`titleButtonRect` layout.
- Setup-screen BACK button now works.
- Removed the `+22` Y mouse hack; `GetMousePosition()` is used directly.
- ESC no longer calls `CloseWindow()` mid-loop; it sets `shouldClose` and the
  run loop exits cleanly. `SetExitKey(0)` lets ESC navigate screens.
- `getSuperpositionProbability()` now rounds properly (`std::lround`).
- En-passant captures now play the capture sound.
- Replaced the non-rendering `\u2014` em dash with ASCII `-`.
- `SetTargetFPS(60)` (was 30) and `FLAG_VSYNC_HINT` enabled.

### Phase B — Type safety
- `Pos` now has a default constructor (`{-1,-1}`), `operator!=`, and `isValid()`.
- `enum class` conversion documented as deferred (see 2.1).

### Phase C — Hardening
- `Board::getPieceID`/`isEmpty` are now `const`.
- `Board::getRawPieceMoves`/`getPiecesMoves` now bounds-check their input.
- `Board::collapseSuperposition` uses a member `std::mt19937` (was `static`).
- `NetworkManager`'s receive buffer is now a per-instance member (`std::array`)
  instead of a function-static buffer (was shared & not thread-safe).
- `Window::loadSprites` checks `LoadTexture` results and logs warnings.
- Removed redundant `PollInputEvents()` in the run loop (`WindowShouldClose`
  already polls).
- `const auto&` in the piece-render loop.

### Phase D — Build system
- Pinned raylib to stable tag `5.5` (was `master`).
- Configurable `CMAKE_BUILD_TYPE` (defaults to Release, was hardcoded Debug).
- Compiler warnings via a `qc_warnings` interface target
  (`-Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor -Woverloaded-virtual`).
- `CMAKE_EXPORT_COMPILE_COMMANDS ON` for clangd/IDE tooling.
- Fixed `BUILD_GAMES` typo (was set to empty string, now `OFF`).
- `make run` now runs from `${CMAKE_SOURCE_DIR}` so `assets/` resolve.
- `target_compile_features(... cxx_std_17)` instead of global `CMAKE_CXX_STANDARD`.
- Install target for binary + assets.

### Phase E — Features
- **Pawn promotion UI**: a dialog lets the player choose Q/R/B/N. The network
  protocol's `MOVE` message gained an optional 5th field for the chosen piece,
  so network games honor promotion choice too.
- **Move history panel** with algebraic notation (e.g. `Nf3`, `exd5`, `O-O`).
- **Captured-pieces tray** under the board.
- **Coordinate labels** (a–h, 1–8) with correct flip handling.
- **Board flip** (press `F`) for hot-seat / Black's perspective; all rendering
  and click detection are flip-aware via `squareToView`/`viewToSquare`.
- **Debug overlay** (press `F1`): FPS, state, mouse coords, current player,
  quantum/flip/net status, superposition probability.
- **Audio mute** (press `M`) via `AudioManager::setVolume`.

### Phase F — Docs
- This review document (`docs/REVIEW.md`).
- Project-specific `AGENTS.md` with build/test/run commands.
- Refreshed `README.md` and `docs/plan.md`.
- `.clang-format` for consistent style.

### Remaining / future work
- `enum class` conversion (see 2.1) — recommended dedicated PR.
- Unit tests with Catch2 (Section 5.3).
- Animations, particle effects, screen shake (plan.md Phase 5).
- AI opponent, save/load (PGN/FEN), reconnection.
- raygui-based settings menu (volume slider, theme picker).
