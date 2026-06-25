// The Game of Life - "automaton" variation.
// Data-oriented design: the 80x25 board is a single flat array, and every cell's
// eight wrap-around (toroidal) neighbours are worked out once at start-up into a
// lookup table, so the per-tick loop is just table reads. Cells that just died are
// shown as a faint trail for one tick, and the run can be paused.

// Ask the standard library to expose POSIX helpers (isatty and /dev/tty access).
#define _POSIX_C_SOURCE 200809L

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Board is 80 columns wide and 25 rows tall, as the task requires.
#define W 80
#define H 25
#define CELLS (H * W)

// Cell values: 0 empty, 1 alive, 2 a one-tick fading trail of a cell that died.
#define DEAD 0
#define ALIVE 1
#define TRAIL 2

// Speed is a delay between frames in milliseconds, adjustable at run time.
#define START_DELAY 120
#define MIN_DELAY 20
#define MAX_DELAY 1000
#define SPEED_STEP 20

// Decide whether one input character means a living cell.
static int is_alive_char(int ch) {
    return ch == '*' || ch == 'O' || ch == 'o' || ch == 'X' || ch == 'x' || ch == '#' || ch == '@' ||
           ch == '+' || ch == '1';
}

// Fill the lookup table: for every cell, store the indices of its 8 neighbours.
static void build_neighbor_table(int* neigh) {
    for (int r = 0; r < H; r++) {
        for (int c = 0; c < W; c++) {
            int i = r * W + c;
            int slot = 0;
            // Wrapping the offsets here means the hot loop never has to.
            for (int dr = -1; dr <= 1; dr++) {
                for (int dc = -1; dc <= 1; dc++) {
                    if (dr == 0 && dc == 0) {
                        continue;
                    }
                    int nr = (r + dr + H) % H;
                    int nc = (c + dc + W) % W;
                    neigh[i * 8 + slot] = nr * W + nc;
                    slot++;
                }
            }
        }
    }
}

// Count living neighbours of cell i using its precomputed neighbour indices.
static int alive_neighbors(const char* board, const int* neigh, int i) {
    int live = 0;
    for (int k = 0; k < 8; k++) {
        if (board[neigh[i * 8 + k]] == ALIVE) {
            live++;
        }
    }
    return live;
}

// Compute the next generation (B3/S23); dying cells become a trail, not empty.
static void step(const char* board, char* next, const int* neigh) {
    for (int i = 0; i < CELLS; i++) {
        int n = alive_neighbors(board, neigh, i);
        int alive = board[i] == ALIVE;
        if (alive && (n == 2 || n == 3)) {
            next[i] = ALIVE;
        } else if (!alive && n == 3) {
            next[i] = ALIVE;
        } else if (alive) {
            next[i] = TRAIL;  // just died: show a faint mark for one tick
        } else {
            next[i] = DEAD;  // stays empty, or a trail fades away
        }
    }
}

// Read the starting board from standard input, one character per cell.
static void read_seed(char* board) {
    for (int r = 0; r < H; r++) {
        char line[256];
        if (fgets(line, (int)sizeof(line), stdin) == NULL) {
            break;
        }
        for (int c = 0; c < W && line[c] != '\0' && line[c] != '\n'; c++) {
            if (is_alive_char((unsigned char)line[c])) {
                board[r * W + c] = ALIVE;
            }
        }
        // If the row was longer than the buffer, skip the rest so rows stay aligned.
        if (strchr(line, '\n') == NULL) {
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {
            }
        }
    }
}

// With no input redirected, place a single glider so the screen is not empty.
static void load_default(char* board) {
    int r = H / 2;
    int c = W / 2;
    board[(r - 1) * W + c] = ALIVE;
    board[r * W + (c + 1)] = ALIVE;
    board[(r + 1) * W + (c - 1)] = ALIVE;
    board[(r + 1) * W + c] = ALIVE;
    board[(r + 1) * W + (c + 1)] = ALIVE;
}

// Count how many cells are currently alive (trails do not count).
static int population(const char* board) {
    int count = 0;
    for (int i = 0; i < CELLS; i++) {
        if (board[i] == ALIVE) {
            count++;
        }
    }
    return count;
}

// Draw the board: a solid mark for life, a faint dot for a fading trail.
static void draw(const char* board, int gen, int pop, int delay, int paused) {
    for (int r = 0; r < H; r++) {
        for (int c = 0; c < W; c++) {
            char v = board[r * W + c];
            if (v == ALIVE) {
                mvaddch(r, c, 'O');
            } else if (v == TRAIL) {
                attron(A_DIM);
                mvaddch(r, c, '.');
                attroff(A_DIM);
            } else {
                mvaddch(r, c, ' ');
            }
        }
    }
    if (LINES > H) {
        mvprintw(H, 0, "gen:%-5d pop:%-4d %dms [%s] A:+ Z:- P:pause Space:quit", gen, pop, delay,
                 paused ? "PAUSED" : "RUNNING");
    }
}

// React to a key press: adjust speed, toggle pause, or report a request to quit.
static int apply_key(int ch, int* delay, int* paused) {
    if (ch == 'a' || ch == 'A') {
        *delay -= SPEED_STEP;
        if (*delay < MIN_DELAY) {
            *delay = MIN_DELAY;
        }
    } else if (ch == 'z' || ch == 'Z') {
        *delay += SPEED_STEP;
        if (*delay > MAX_DELAY) {
            *delay = MAX_DELAY;
        }
    } else if (ch == 'p' || ch == 'P') {
        *paused = !*paused;
    } else if (ch == ' ') {
        return 1;
    }
    return 0;
}

// Interactive loop with a small running/paused state machine.
static void run_ncurses(char* board, char* next, const int* neigh) {
    int delay = START_DELAY;
    int gen = 0;
    int paused = 0;
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    while (1) {
        timeout(delay);
        if (apply_key(getch(), &delay, &paused)) {
            break;
        }
        erase();
        draw(board, gen, population(board), delay, paused);
        refresh();
        if (!paused) {
            step(board, next, neigh);
            memcpy(board, next, (size_t)CELLS);
            gen++;
        }
    }
    endwin();
}

// Advance the simulation with no screen output (the headless --frames mode).
static void run_headless(char* board, char* next, const int* neigh, int frames) {
    for (int i = 0; i < frames; i++) {
        step(board, next, neigh);
        memcpy(board, next, (size_t)CELLS);
    }
    printf("frames=%d population=%d\n", frames, population(board));
}

int main(int argc, char* const argv[]) {
    static char board[CELLS];
    static char next[CELLS];
    // The neighbour table is the only heap allocation; it is freed before exit.
    int* neigh = malloc((size_t)CELLS * 8 * sizeof(int));
    if (neigh == NULL) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }
    build_neighbor_table(neigh);
    // Look for "--frames N": N runs headless (no screen) for testing; -1 means play.
    int frames = -1;
    for (int i = 1; i + 1 < argc; i++) {
        if (strcmp(argv[i], "--frames") == 0) {
            frames = atoi(argv[i + 1]);
        }
    }
    // Take the seed from a redirected file, or fall back to a built-in glider.
    if (isatty(STDIN_FILENO)) {
        load_default(board);
    } else {
        read_seed(board);
    }
    if (frames >= 0) {
        run_headless(board, next, neigh, frames);
        free(neigh);
        return 0;
    }
    // The seed arrived on stdin; reconnect the keyboard so keys can be read.
    if (freopen("/dev/tty", "r", stdin) == NULL) {
        fprintf(stderr, "cannot open terminal for input\n");
        free(neigh);
        return 1;
    }
    run_ncurses(board, next, neigh);
    free(neigh);
    return 0;
}
