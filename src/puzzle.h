#ifndef PUZZLE_H
#define PUZZLE_H

#include <stdbool.h>

/* Largest board we accept. Columns are labelled a..z, so this can never
 * grow past 26 without changing the coordinate scheme. */
#define MAX_SIZE 20

/* How many board states the undo stack keeps. */
#define MAX_HIST 100

/*
 * A cell can be in one of three states, encoded directly in the character:
 *
 *   'a'..'z'  unmarked  - the player hasn't decided yet
 *   'A'..'Z'  white     - the player kept this letter
 *   '#'       crossed   - the player struck this letter out
 *
 * `orig` always holds the lowercase letter the puzzle started with, so we
 * can recover a letter after it has been crossed out.
 */
typedef struct {
    char board[MAX_SIZE][MAX_SIZE];
    char orig[MAX_SIZE][MAX_SIZE];
    int rows, cols;
} Game;

extern Game game;

/* --- state ------------------------------------------------------------- */

void save_state(void);
void undo(void);

/* --- cell state helpers ------------------------------------------------ */

bool is_white(char c);
bool is_crossed(char c);
bool is_unmarked(char c);

/* --- files ------------------------------------------------------------- */

bool load_game(const char *file);
bool save_game(const char *file);

/* --- player moves ------------------------------------------------------ */

/* Parses "b3" into row/col. Returns false if the coordinate is malformed or
 * falls outside the current board. */
bool get_coords(const char *coord, int *row, int *col);

void make_white(int row, int col);
void cross_out(int row, int col);

/* --- board ------------------------------------------------------------- */

void show_board(void);

/* Reports the first rule violation found and writes its position to
 * row/col. Returns true when an error exists. */
bool check_errors(int *row, int *col);

/* --- solving ----------------------------------------------------------- */

void give_hint(void);      /* one round of deductions, narrated */
void apply_all_hints(void); /* deduce until nothing else follows */
void solve(void);           /* reset to the original board, then solve it */

#endif /* PUZZLE_H */
