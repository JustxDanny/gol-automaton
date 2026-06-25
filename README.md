# gol-automaton — Conway's Game of Life (data-oriented edition)

![ci](https://github.com/JustxDanny/gol-automaton/actions/workflows/ci.yml/badge.svg)

A terminal Game of Life in C with **ncurses**, built data-oriented. The 80×25 board
is a single **flat array**, and every cell's eight wrap-around neighbours are
precomputed once into a **lookup table** — so the per-tick loop is just table reads,
with no edge maths at all. Cells that just died leave a **faint trail** for one tick,
and the simulation can be **paused**.

```
        O O                                  .  .
         OO       gen:88  pop:31  120ms [RUNNING]
         O        A:+ Z:- P:pause Space:quit
       .   .          ( . = a cell that died last tick )
```

## Rules

Conway's standard **B3/S23** on an 80×25 **toroidal** board. Trails are purely
visual — they are not alive and never affect the simulation.

## Build & run

```sh
cd src
make
./game_of_life < ../seeds/pulsar.txt
```

The starting board is read from **standard input**; any of `* O o X x # @ + 1`
marks a living cell. Ready-made patterns are in [`seeds/`](seeds).

## Controls

| Key | Action |
|-----|--------|
| `A` | speed up |
| `Z` | slow down |
| `P` | pause / resume |
| `Space` | quit |

A bare run (no input) starts with a single glider.

## Patterns (`seeds/`)

`block` (still life) · `blinker_toad` / `pulsar` (oscillators) · `glider`
(spaceship) · `gosper_gun` (glider gun) · `r_pentomino` / `acorn` (methuselahs).

## How it works (architecture)

- **State:** two flat `char[2000]` boards; a cell is empty, alive, or a one-tick
  trail.
- **Neighbour table:** `build_neighbor_table()` stores, for each of the 2000 cells,
  the indices of its 8 toroidal neighbours (heap-allocated, freed at exit). The hot
  loop never computes a wrap.
- **State machine:** a tiny running/paused loop; pausing freezes stepping but keeps
  the screen and keys live.
- **Input handover:** the seed is read from stdin, then the controlling terminal is
  reopened so ncurses can read live keys.
- **Tested:** a hidden `--frames N` mode runs headless so CI can valgrind the table
  allocation for leaks and assert known populations.

## Tests

GitHub Actions runs **clang-format-18** (Google style), **cppcheck 2.13**, a
`-Wall -Wextra -Werror` build, a **valgrind** leak check, and population invariants
on every push. See [`.github/workflows/ci.yml`](.github/workflows/ci.yml).

## Family

One of three independent takes on the same task:
[gol-classic](https://github.com/JustxDanny/gol-classic) ·
[gol-world](https://github.com/JustxDanny/gol-world) ·
[gol-automaton](https://github.com/JustxDanny/gol-automaton).
