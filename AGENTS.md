# AGENTS.md

Terminal Tetris in C++20 (g++, POSIX only, no ncurses — raw `termios` + ANSI 256 / truecolor). No external dependencies, no package manager, no CI.

## Commands

- `make` — build `./tetris` (binary lands at repo root). `make run` to run.
- `make test` — unit tests, **no terminal needed**. Links only `Tablero.o Tetramino.o Bolsa.o Puntuacion.o Datos.o` + `tests/test_logica.cpp`; non-zero exit on failure. No test framework: `check(cond, "desc")` pattern, add cases in `main()`.
- `make debug` — rebuilds with ASan+UBSan (runs `clean` first; it builds only, then run `./tetris` yourself).
- `make clean` — removes `build/` and `./tetris`.
- `python3 tests/smpty_pty.py` — pty smoke test. Requires `make` done first. Feeds real keystrokes to `./tetris` in a pseudo-terminal and greps rendered text. Not wired into `make test`. Takes ~10 s (timed key sequences).

## Architecture constraints

- **Core logic must stay terminal-free.** `Tablero`, `Tetramino`, `Bolsa`, `Puntuacion`, `Datos` are shared by `make test`; never add `termios`, rendering, or terminal I/O there (`Datos`/`Puntuacion` may use `fstream`), or `make test` breaks.
- Flow: `src/main.cpp` → `Juego` (state machine MENU/AYUDA/AJUSTES/JUGANDO/FIN + game loop, gravity, lock delay). `Entrada` = raw termios + `poll`; `Renderizador` = ANSI output.
- **All color output must go through `Renderizador::pinta`** — it applies the active palette (CLASICA = per-element ANSI-256; MATRIX = single phosphor truecolor green `#00FF41`). Painting with raw `ansi()` outside the palette samples breaks MATRIX.
- `include/constantes.hpp` is the single tuning point (geometry, colors, NES speed table, scoring); everything depends on it.
- The game refuses to start without a real tty and a terminal ≥ 52x24 (diseño para 168x63) (`main.cpp` checks) — don't try to verify gameplay via pipes/redirects; use the pty smoke test instead.

## Repo-specific gotchas

- **All code, comments, and UI strings are in Spanish**; namespace is `juego`. Follow this convention in new code.
- Scores persist to `~/.tetris_puntos.json` (`{"record", "partidas": [últimas 5], "paleta"}`), created and managed by `src/Datos.cpp` (hand-rolled tolerant JSON parser/serializer, no deps). `Juego::rutaGuardado()`. One-time auto-migration from the legacy plain-text `~/.tetris_record`; the palette choice also persists. Reset from the AJUSTES screen (double-ENTER confirm).
- The pty smoke test isolates `HOME` in a temp dir so it never touches real save files; keep it that way if you extend it.
- Implemented rules to preserve when editing: SRS wall kicks (`rotarConKick`), 7-bag randomizer, NES speed curve, 500 ms lock delay with max 15 resets.
- Git repo has no commits yet and no `.gitignore`: never stage `build/` or the `tetris` binary if you commit.
