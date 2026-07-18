/*
 * Puzzle Solver - command line front end.
 *
 * Reads one command per line and dispatches it. All the game logic lives in
 * puzzle.c; this file only deals with parsing what the player typed.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "puzzle.h"

static void print_help(void)
{
    printf("Comandos:\n"
           "  l <ficheiro>  carregar um jogo\n"
           "  g <ficheiro>  gravar o jogo\n"
           "  b <coord>     pintar uma casa de branco  (ex: b b3)\n"
           "  r <coord>     riscar uma casa            (ex: r b3)\n"
           "  <coord>       ver o estado de uma casa   (ex: b3)\n"
           "  v             verificar erros\n"
           "  a             pedir uma dica\n"
           "  A             aplicar dicas ate esgotar\n"
           "  R             resolver o jogo\n"
           "  d             desfazer\n"
           "  ?             mostrar esta ajuda\n"
           "  s             sair\n");
}

/*
 * Several commands share a letter with a column: "b" is both "paint white"
 * and the second column. We tell them apart by the space - "b b3" is a
 * command, "b3" is a coordinate. Nothing else would disambiguate them.
 */
static bool is_command_with_arg(const char *cmd)
{
    return cmd[1] == ' ';
}

/* Handles "b3" - a bare coordinate, meaning "tell me about this cell". */
static void inspect_cell(const char *cmd)
{
    int r, c;
    if (!get_coords(cmd, &r, &c)) {
        printf("Comando desconhecido. Escreve '?' para ver a ajuda.\n");
        return;
    }

    char cell = game.board[r][c];
    const char *state = is_white(cell)   ? "branca"
                      : is_crossed(cell) ? "riscada"
                                         : "por decidir";

    printf("%s: %c (letra original '%c', %s)\n", cmd, cell, game.orig[r][c], state);
}

int main(void)
{
    char cmd[100];

    printf("Bem-vindo ao Puzzle Solver!\n");
    print_help();

    for (;;) {
        printf("> ");
        if (!fgets(cmd, sizeof(cmd), stdin))
            break; /* EOF - the player piped a file in, or hit Ctrl+D */

        cmd[strcspn(cmd, "\n")] = '\0';
        if (cmd[0] == '\0')
            continue;

        char arg[100];
        int r, c;

        switch (cmd[0]) {
        case 'l':
            if (!is_command_with_arg(cmd))
                printf("Uso: l <ficheiro>\n");
            else if (sscanf(cmd, "l %99s", arg) != 1)
                printf("Uso: l <ficheiro>\n");
            else if (load_game(arg))
                show_board();
            break;

        case 'g':
            if (!is_command_with_arg(cmd) || sscanf(cmd, "g %99s", arg) != 1)
                printf("Uso: g <ficheiro>\n");
            else
                save_game(arg);
            break;

        case 'b':
            if (!is_command_with_arg(cmd)) {
                inspect_cell(cmd);
            } else if (sscanf(cmd, "b %99s", arg) != 1) {
                printf("Uso: b <coord>\n");
            } else if (get_coords(arg, &r, &c)) {
                make_white(r, c);
                show_board();
            } else {
                printf("Coordenada invalida: '%s'\n", arg);
            }
            break;

        case 'r':
            if (!is_command_with_arg(cmd)) {
                inspect_cell(cmd);
            } else if (sscanf(cmd, "r %99s", arg) != 1) {
                printf("Uso: r <coord>\n");
            } else if (get_coords(arg, &r, &c)) {
                cross_out(r, c);
                show_board();
            } else {
                printf("Coordenada invalida: '%s'\n", arg);
            }
            break;

        case 'v':
            if (cmd[1] == '\0')
                check_errors(&r, &c);
            else
                inspect_cell(cmd);
            break;

        case 'a':
            if (cmd[1] == '\0') {
                give_hint();
                show_board();
            } else {
                inspect_cell(cmd);
            }
            break;

        case 'A':
            apply_all_hints();
            show_board();
            break;

        case 'R':
            solve();
            show_board();
            break;

        case 'd':
            if (cmd[1] == '\0') {
                undo();
                show_board();
            } else {
                inspect_cell(cmd);
            }
            break;

        case '?':
            print_help();
            break;

        case 's':
            if (cmd[1] == '\0') {
                printf("A sair. Ate a proxima!\n");
                return 0;
            }
            inspect_cell(cmd);
            break;

        default:
            inspect_cell(cmd);
            break;
        }
    }

    return 0;
}
