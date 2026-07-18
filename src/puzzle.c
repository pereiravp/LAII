#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "puzzle.h"

Game game;

/* The undo stack lives here rather than in the header: nothing outside this
 * file has any business touching it. */
static Game history[MAX_HIST];
static int hist_pos = -1;

/* The four orthogonal neighbours, used by most of the rules below. */
static const int DR[4] = {-1, 1, 0, 0};
static const int DC[4] = {0, 0, -1, 1};

static bool in_bounds(int row, int col)
{
    return row >= 0 && row < game.rows && col >= 0 && col < game.cols;
}

/* ---------------------------------------------------------------------- */
/* State                                                                   */
/* ---------------------------------------------------------------------- */

void save_state(void)
{
    if (hist_pos < MAX_HIST - 1)
        history[++hist_pos] = game;
    else
        printf("Aviso: historico cheio, esta jogada nao podera ser desfeita.\n");
}

void undo(void)
{
    if (hist_pos < 0) {
        printf("Nada para desfazer.\n");
        return;
    }
    game = history[hist_pos--];
    printf("Desfeito.\n");
}

/* ---------------------------------------------------------------------- */
/* Cell state                                                              */
/* ---------------------------------------------------------------------- */

bool is_white(char c)    { return c >= 'A' && c <= 'Z'; }
bool is_crossed(char c)  { return c == '#'; }
bool is_unmarked(char c) { return c >= 'a' && c <= 'z'; }

/* ---------------------------------------------------------------------- */
/* Files                                                                   */
/* ---------------------------------------------------------------------- */

/*
 * File format
 * -----------
 *   <rows> <cols>
 *   <rows lines of letters>          the puzzle itself, always lowercase
 *   <rows lines of marks>            optional: '.' undecided, 'o' white, '#' crossed
 *
 * A fresh puzzle has no mark section, so the files in puzzles/ load exactly
 * as they always did. A saved game carries one.
 *
 * The marks are kept apart from the letters on purpose. Writing '#' straight
 * over a letter - which is what this used to do - throws the letter away, so
 * reloading a saved game left the solver with nothing to work from.
 */

static bool read_grid_line(FILE *f, char *line, int cols, int row, const char *what)
{
    /* The width in the format string has to match the buffer, otherwise a
     * long line in the file walks straight off the end of it. */
    if (fscanf(f, "%20s", line) != 1) {
        printf("Erro: faltam linhas de %s (parei na linha %d).\n", what, row + 1);
        return false;
    }
    if ((int) strlen(line) != cols) {
        printf("Erro: a linha %d de %s tem %d caracteres, esperava %d.\n",
               row + 1, what, (int) strlen(line), cols);
        return false;
    }
    return true;
}

bool load_game(const char *file)
{
    FILE *f = fopen(file, "r");
    if (!f) {
        printf("Erro: nao consegui abrir '%s'.\n", file);
        return false;
    }

    Game loaded;
    if (fscanf(f, "%d %d", &loaded.rows, &loaded.cols) != 2) {
        printf("Erro: a primeira linha devia ter as dimensoes do tabuleiro.\n");
        fclose(f);
        return false;
    }

    if (loaded.rows < 1 || loaded.rows > MAX_SIZE ||
        loaded.cols < 1 || loaded.cols > MAX_SIZE) {
        printf("Erro: dimensoes %dx%d fora do intervalo permitido (1..%d).\n",
               loaded.rows, loaded.cols, MAX_SIZE);
        fclose(f);
        return false;
    }

    for (int i = 0; i < loaded.rows; i++) {
        char line[MAX_SIZE + 1];
        if (!read_grid_line(f, line, loaded.cols, i, "letras")) {
            fclose(f);
            return false;
        }

        for (int j = 0; j < loaded.cols; j++) {
            if (!isalpha((unsigned char) line[j])) {
                printf("Erro: caracter invalido '%c' na linha %d.\n", line[j], i + 1);
                fclose(f);
                return false;
            }
            loaded.orig[i][j] = (char) tolower((unsigned char) line[j]);
            loaded.board[i][j] = loaded.orig[i][j];
        }
    }

    /* A mark section is optional. If the next line is there, all of it has
     * to be there. */
    char first[MAX_SIZE + 1];
    if (fscanf(f, "%20s", first) == 1) {
        for (int i = 0; i < loaded.rows; i++) {
            char line[MAX_SIZE + 1];

            if (i == 0) {
                if ((int) strlen(first) != loaded.cols) {
                    printf("Erro: a linha 1 de marcas tem %d caracteres, esperava %d.\n",
                           (int) strlen(first), loaded.cols);
                    fclose(f);
                    return false;
                }
                strcpy(line, first);
            } else if (!read_grid_line(f, line, loaded.cols, i, "marcas")) {
                fclose(f);
                return false;
            }

            for (int j = 0; j < loaded.cols; j++) {
                switch (line[j]) {
                case '.': loaded.board[i][j] = loaded.orig[i][j]; break;
                case 'o': loaded.board[i][j] = (char) toupper((unsigned char) loaded.orig[i][j]); break;
                case '#': loaded.board[i][j] = '#'; break;
                default:
                    printf("Erro: marca invalida '%c' na linha %d "
                           "(esperava '.', 'o' ou '#').\n", line[j], i + 1);
                    fclose(f);
                    return false;
                }
            }
        }
    }

    fclose(f);

    /* Only commit once the whole file parsed, so a bad file leaves whatever
     * the player already had on screen untouched. */
    game = loaded;
    hist_pos = -1;

    printf("Jogo carregado de '%s'.\n", file);
    return true;
}

bool save_game(const char *file)
{
    FILE *f = fopen(file, "w");
    if (!f) {
        printf("Erro: nao consegui escrever em '%s'.\n", file);
        return false;
    }

    fprintf(f, "%d %d\n", game.rows, game.cols);

    for (int i = 0; i < game.rows; i++) {
        for (int j = 0; j < game.cols; j++)
            fputc(game.orig[i][j], f);
        fputc('\n', f);
    }

    for (int i = 0; i < game.rows; i++) {
        for (int j = 0; j < game.cols; j++) {
            char cell = game.board[i][j];
            fputc(is_white(cell) ? 'o' : is_crossed(cell) ? '#' : '.', f);
        }
        fputc('\n', f);
    }

    if (fclose(f) != 0) {
        printf("Erro: falha ao fechar '%s', o jogo pode nao ter sido gravado.\n", file);
        return false;
    }

    printf("Jogo gravado em '%s'.\n", file);
    return true;
}

/* ---------------------------------------------------------------------- */
/* Player moves                                                            */
/* ---------------------------------------------------------------------- */

bool get_coords(const char *coord, int *row, int *col)
{
    if (!isalpha((unsigned char) coord[0]) || !isdigit((unsigned char) coord[1]))
        return false;

    *col = tolower((unsigned char) coord[0]) - 'a';
    *row = atoi(coord + 1) - 1;

    return in_bounds(*row, *col);
}

void make_white(int row, int col)
{
    if (!in_bounds(row, col)) {
        printf("Coordenada invalida.\n");
        return;
    }
    save_state();
    game.board[row][col] = (char) toupper((unsigned char) game.orig[row][col]);
    printf("Casa %c%d pintada de branco.\n", 'a' + col, row + 1);
}

void cross_out(int row, int col)
{
    if (!in_bounds(row, col)) {
        printf("Coordenada invalida.\n");
        return;
    }
    save_state();
    game.board[row][col] = '#';
    printf("Casa %c%d riscada.\n", 'a' + col, row + 1);
}

/* ---------------------------------------------------------------------- */
/* Board                                                                   */
/* ---------------------------------------------------------------------- */

void show_board(void)
{
    if (game.rows == 0) {
        printf("Nenhum jogo carregado. Usa 'l <ficheiro>'.\n");
        return;
    }

    printf("\n   ");
    for (int j = 0; j < game.cols; j++)
        printf("%c ", 'a' + j);
    printf("\n");

    for (int i = 0; i < game.rows; i++) {
        printf("%2d ", i + 1);
        for (int j = 0; j < game.cols; j++)
            printf("%c ", game.board[i][j]);
        printf("\n");
    }
    printf("\n");
}

bool check_errors(int *row, int *col)
{
    /* Rule 1: no repeated white letter in a row. */
    for (int i = 0; i < game.rows; i++) {
        for (int j = 0; j < game.cols; j++) {
            if (!is_white(game.board[i][j]))
                continue;
            for (int k = j + 1; k < game.cols; k++) {
                if (game.board[i][k] == game.board[i][j]) {
                    printf("Erro: '%c' repetida na linha %d.\n", game.board[i][j], i + 1);
                    *row = i;
                    *col = k;
                    return true;
                }
            }
        }
    }

    /* Rule 1, again, down the columns. */
    for (int j = 0; j < game.cols; j++) {
        for (int i = 0; i < game.rows; i++) {
            if (!is_white(game.board[i][j]))
                continue;
            for (int k = i + 1; k < game.rows; k++) {
                if (game.board[k][j] == game.board[i][j]) {
                    printf("Erro: '%c' repetida na coluna %c.\n", game.board[i][j], 'a' + j);
                    *row = k;
                    *col = j;
                    return true;
                }
            }
        }
    }

    /* Rule 2: two crossed cells may never touch. An unmarked neighbour is
     * fine - the player simply hasn't got to it yet. */
    for (int i = 0; i < game.rows; i++) {
        for (int j = 0; j < game.cols; j++) {
            if (!is_crossed(game.board[i][j]))
                continue;
            for (int k = 0; k < 4; k++) {
                int ni = i + DR[k], nj = j + DC[k];
                if (in_bounds(ni, nj) && is_crossed(game.board[ni][nj])) {
                    printf("Erro: casas riscadas adjacentes em %c%d e %c%d.\n",
                           'a' + j, i + 1, 'a' + nj, ni + 1);
                    *row = i;
                    *col = j;
                    return true;
                }
            }
        }
    }

    printf("Sem erros.\n");
    return false;
}

/* ---------------------------------------------------------------------- */
/* Deduction rules                                                         */
/* ---------------------------------------------------------------------- */
/*
 * Every rule below is sound: it only marks a cell when no valid solution
 * could have marked it the other way.
 *
 * That is worth stating because an earlier version of this file also had a
 * rule saying "a letter that appears once in its row must be white". It
 * sounds reasonable and it is false - a letter can be unique in its row and
 * still have to be crossed out because of its column. That one rule was
 * enough to produce boards that failed the program's own error check.
 *
 * The two setters below return false when a rule tries to mark a cell in a
 * way that contradicts what is already there. During a hint that means the
 * player's board is broken; during the search it means the current guess was
 * wrong and we should back out.
 */

static bool set_white(int i, int j, bool verbose, bool *changed)
{
    if (is_white(game.board[i][j]))
        return true;
    if (is_crossed(game.board[i][j]))
        return false;

    game.board[i][j] = (char) toupper((unsigned char) game.orig[i][j]);
    *changed = true;
    if (verbose)
        printf("  Pintei %c%d de branco.\n", 'a' + j, i + 1);
    return true;
}

static bool set_crossed(int i, int j, bool verbose, bool *changed)
{
    if (is_crossed(game.board[i][j]))
        return true;
    if (is_white(game.board[i][j]))
        return false;

    game.board[i][j] = '#';
    *changed = true;
    if (verbose)
        printf("  Risquei %c%d.\n", 'a' + j, i + 1);
    return true;
}

/* Counts the cells next to (row, col) that could still end up white, while
 * pretending (skip_r, skip_c) is crossed. Used to spot white cells that
 * would be cut off from the rest of the board. */
static int reachable_neighbours(int row, int col, int skip_r, int skip_c)
{
    int count = 0;
    for (int k = 0; k < 4; k++) {
        int nr = row + DR[k], nc = col + DC[k];
        if (!in_bounds(nr, nc) || (nr == skip_r && nc == skip_c))
            continue;
        if (!is_crossed(game.board[nr][nc]))
            count++;
    }
    return count;
}

/* The pattern rules work the same way along a row and down a column, so they
 * are written once against these three helpers. `vertical` picks which. */

static char line_letter(int line, int a, bool vertical)
{
    return vertical ? game.orig[a][line] : game.orig[line][a];
}

static bool line_set_white(int line, int a, bool vertical, bool verbose, bool *changed)
{
    return vertical ? set_white(a, line, verbose, changed)
                    : set_white(line, a, verbose, changed);
}

static bool line_set_crossed(int line, int a, bool vertical, bool verbose, bool *changed)
{
    return vertical ? set_crossed(a, line, verbose, changed)
                    : set_crossed(line, a, verbose, changed);
}

/*
 * The three classic pattern rules, applied to one row or one column.
 *
 *   triple    x x x  -> middle white, outer two crossed. Crossing the middle
 *                       would leave two crossed cells touching, and once the
 *                       middle is white the outer two duplicate it.
 *   sandwich  x y x  -> y is white. One of the two x's must be crossed, and
 *                       whichever one it is, y sits right next to it.
 *   pair      x x    -> exactly one of the pair survives, so every *other* x
 *                       in the same line has to go.
 */
static bool apply_line_patterns(int line, int len, bool vertical,
                                bool verbose, bool *changed)
{
    for (int a = 0; a + 2 < len; a++) {
        char first = line_letter(line, a, vertical);
        char mid   = line_letter(line, a + 1, vertical);
        char last  = line_letter(line, a + 2, vertical);

        if (first == mid && mid == last) {
            if (!line_set_white(line, a + 1, vertical, verbose, changed) ||
                !line_set_crossed(line, a, vertical, verbose, changed) ||
                !line_set_crossed(line, a + 2, vertical, verbose, changed))
                return false;
        } else if (first == last) {
            if (!line_set_white(line, a + 1, vertical, verbose, changed))
                return false;
        }
    }

    for (int a = 0; a + 1 < len; a++) {
        char letter = line_letter(line, a, vertical);
        if (letter != line_letter(line, a + 1, vertical))
            continue;

        for (int b = 0; b < len; b++) {
            if (b == a || b == a + 1)
                continue;
            if (line_letter(line, b, vertical) == letter &&
                !line_set_crossed(line, b, vertical, verbose, changed))
                return false;
        }
    }

    return true;
}

/*
 * One pass of every rule we know. Sets *changed if the board moved. Returns
 * false if the board contradicts itself.
 *
 * This logic used to be copy-pasted between give_hint() and apply_rules(),
 * and the two copies had already drifted apart - one was missing the
 * isolation rule entirely.
 */
static bool apply_rules_once(bool verbose, bool *changed)
{
    /* Rule 1: a white letter forces every twin in its row/column out. */
    for (int i = 0; i < game.rows; i++) {
        for (int j = 0; j < game.cols; j++) {
            if (!is_white(game.board[i][j]))
                continue;
            char letter = game.orig[i][j];

            for (int k = 0; k < game.cols; k++)
                if (k != j && game.orig[i][k] == letter &&
                    !set_crossed(i, k, verbose, changed))
                    return false;

            for (int k = 0; k < game.rows; k++)
                if (k != i && game.orig[k][j] == letter &&
                    !set_crossed(k, j, verbose, changed))
                    return false;
        }
    }

    /* Rule 2: crossed cells may not touch, so their neighbours are white. */
    for (int i = 0; i < game.rows; i++) {
        for (int j = 0; j < game.cols; j++) {
            if (!is_crossed(game.board[i][j]))
                continue;
            for (int k = 0; k < 4; k++) {
                int ni = i + DR[k], nj = j + DC[k];
                if (in_bounds(ni, nj) && !set_white(ni, nj, verbose, changed))
                    return false;
            }
        }
    }

    /* Rule 3: if crossing (i,j) would strand a white neighbour with no way
     * out, (i,j) has to stay white. */
    for (int i = 0; i < game.rows; i++) {
        for (int j = 0; j < game.cols; j++) {
            if (!is_unmarked(game.board[i][j]))
                continue;
            for (int k = 0; k < 4; k++) {
                int ni = i + DR[k], nj = j + DC[k];
                if (!in_bounds(ni, nj) || !is_white(game.board[ni][nj]))
                    continue;
                if (reachable_neighbours(ni, nj, i, j) == 0) {
                    if (!set_white(i, j, verbose, changed))
                        return false;
                    break;
                }
            }
        }
    }

    /* Rules 4-6: the pattern rules, on every row and every column. */
    for (int i = 0; i < game.rows; i++)
        if (!apply_line_patterns(i, game.cols, false, verbose, changed))
            return false;

    for (int j = 0; j < game.cols; j++)
        if (!apply_line_patterns(j, game.rows, true, verbose, changed))
            return false;

    return true;
}

/* Deduce as far as the rules go. False means the board is contradictory. */
static bool propagate(bool verbose, int *rounds)
{
    bool changed = true;
    int count = 0;

    while (changed) {
        changed = false;
        if (!apply_rules_once(verbose, &changed))
            return false;
        if (changed)
            count++;
    }

    if (rounds)
        *rounds = count;
    return true;
}

/* ---------------------------------------------------------------------- */
/* Search                                                                  */
/* ---------------------------------------------------------------------- */

/* Are all the white cells joined into a single blob? Plain flood fill from
 * the first white cell we find. */
static bool whites_connected(void)
{
    bool seen[MAX_SIZE][MAX_SIZE] = {{false}};
    int stack[MAX_SIZE * MAX_SIZE][2];
    int top = 0, total = 0, found = 0;

    for (int i = 0; i < game.rows; i++)
        for (int j = 0; j < game.cols; j++)
            if (is_white(game.board[i][j]))
                total++;

    if (total == 0)
        return true;

    for (int i = 0; i < game.rows && top == 0; i++)
        for (int j = 0; j < game.cols && top == 0; j++)
            if (is_white(game.board[i][j])) {
                stack[top][0] = i;
                stack[top][1] = j;
                top++;
                seen[i][j] = true;
            }

    while (top > 0) {
        top--;
        int i = stack[top][0], j = stack[top][1];
        found++;
        for (int k = 0; k < 4; k++) {
            int ni = i + DR[k], nj = j + DC[k];
            if (in_bounds(ni, nj) && !seen[ni][nj] && is_white(game.board[ni][nj])) {
                seen[ni][nj] = true;
                stack[top][0] = ni;
                stack[top][1] = nj;
                top++;
            }
        }
    }

    return found == total;
}

static bool find_unmarked(int *row, int *col)
{
    for (int i = 0; i < game.rows; i++)
        for (int j = 0; j < game.cols; j++)
            if (is_unmarked(game.board[i][j])) {
                *row = i;
                *col = j;
                return true;
            }
    return false;
}

/*
 * Deduce what we can, then guess when the rules run dry.
 *
 * Guessing is what makes this correct on every puzzle instead of only the
 * easy ones. The rules on their own get stuck part way through, and the old
 * code papered over that with an unsound rule rather than searching.
 */
static bool search(void)
{
    char before[MAX_SIZE][MAX_SIZE];
    memcpy(before, game.board, sizeof(before));

    if (!propagate(false, NULL)) {
        memcpy(game.board, before, sizeof(before));
        return false;
    }

    int i, j;
    if (!find_unmarked(&i, &j)) {
        if (whites_connected())
            return true;
        memcpy(game.board, before, sizeof(before));
        return false;
    }

    char snapshot[MAX_SIZE][MAX_SIZE];
    memcpy(snapshot, game.board, sizeof(snapshot));

    game.board[i][j] = (char) toupper((unsigned char) game.orig[i][j]);
    if (search())
        return true;

    memcpy(game.board, snapshot, sizeof(snapshot));
    game.board[i][j] = '#';
    if (search())
        return true;

    memcpy(game.board, before, sizeof(before));
    return false;
}

/* ---------------------------------------------------------------------- */
/* Public solving commands                                                 */
/* ---------------------------------------------------------------------- */

void give_hint(void)
{
    int r, c;
    if (check_errors(&r, &c)) {
        printf("Corrige o erro em %c%d antes de pedir dicas.\n", 'a' + c, r + 1);
        return;
    }

    save_state();

    bool changed = false;
    if (!apply_rules_once(true, &changed)) {
        printf("As regras entram em contradicao a partir deste tabuleiro. "
               "Alguma marca tua deve estar errada.\n");
        undo();
        return;
    }

    if (!changed) {
        printf("Nao ha nenhuma dica obvia a partir daqui. Usa 'R' para resolver.\n");
        hist_pos--; /* nothing changed, so don't leave a dead undo step */
    }
}

void apply_all_hints(void)
{
    int r, c;
    if (check_errors(&r, &c)) {
        printf("Corrige o erro em %c%d antes de pedir dicas.\n", 'a' + c, r + 1);
        return;
    }

    save_state();

    int rounds = 0;
    if (!propagate(true, &rounds)) {
        printf("As regras entram em contradicao a partir deste tabuleiro. "
               "Alguma marca tua deve estar errada.\n");
        undo();
        return;
    }

    printf("Apliquei dicas durante %d ronda(s).\n", rounds);
    if (rounds == 0)
        hist_pos--;
}

void solve(void)
{
    save_state();
    printf("A resolver...\n");

    /* Start from a clean slate - the player's marks might be wrong, and the
     * search only ever adds information on top of what it is given. */
    for (int i = 0; i < game.rows; i++)
        for (int j = 0; j < game.cols; j++)
            game.board[i][j] = game.orig[i][j];

    if (search())
        printf("Resolvido.\n");
    else
        printf("Este tabuleiro nao tem solucao.\n");
}
