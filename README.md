# Puzzle Solver

A command line implementation of a Hitori-style logic puzzle, written in C.
You get a grid of letters and have to decide, for every cell, whether to keep
it or cross it out. It plays the game, checks your work, gives hints, and can
solve any board outright.

Built as a university project at Universidade do Minho, then cleaned up and
extended. The interface is in Portuguese; the code and this README are in
English.

```
   a b c d e f g h i          a b c d e f g h i
 1 i d a e e g f g b        1 I D A # E # F G #
 2 h i c i d e b g f        2 H # C I D E B # F
 3 g b a d h b a f e        3 G B # D H # A F E
 4 i a f i b h g e d   ->   4 # A F # B H G E #
 5 d g g f a b b h c        5 D G # F # B # H C
 6 e c b g c f i f d        6 E # B G C F I # D
 7 d f e f g i b a b        7 # F E # G I # A B
 8 f e g h a g c g i        8 F E # H A G C # I
 9 b g h e d a d i b        9 B # H E # A D I #
```

## The rules

Every cell is either **white** (kept, shown uppercase) or **crossed**
(removed, shown as `#`). A board is solved when all three hold:

1. **No repeats.** No letter appears twice among the white cells of any row or
   column.
2. **No touching.** Two crossed cells are never orthogonally adjacent.
3. **All connected.** The white cells form a single group, reachable from one
   another horizontally and vertically.

Cells you haven't decided on yet are shown in lowercase.

## Build and run

Needs a C11 compiler and `make`.

```sh
make          # build ./puzzle
make run      # build and start it
make test     # build and run the test suite
make clean    # remove build artefacts
```

## Commands

| Command | Description |
| --- | --- |
| `l <file>` | Load a puzzle, e.g. `l puzzles/j1.txt` |
| `g <file>` | Save the current game |
| `b <coord>` | Paint a cell white, e.g. `b b3` |
| `r <coord>` | Cross a cell out, e.g. `r b3` |
| `<coord>` | Show a cell's state, e.g. `b3` |
| `v` | Check the board for rule violations |
| `a` | Apply one round of hints |
| `A` | Apply hints until nothing more follows |
| `R` | Solve the board |
| `d` | Undo |
| `?` | Help |
| `s` | Quit |

Columns are letters, rows are numbers, so `c4` is column `c`, row 4.

Note the space: `b b3` paints cell b3 white, while `b3` on its own just tells
you about cell b3. The same goes for `r`, `l` and `g`.

### A short session

```
> l puzzles/j4.txt
> A                 # let the rules do what they can
> v                 # any mistakes?
> R                 # solve it
> g solved.txt      # keep it
```

## File format

```
<rows> <cols>
<rows lines of letters>     the puzzle, lowercase
<rows lines of marks>       optional: '.' undecided, 'o' white, '#' crossed
```

A fresh puzzle is just the dimensions and the letters, see `puzzles/`. Saved
games carry the extra mark section, which is how the letters survive being
crossed out and a saved game can be picked up where you left it.

## How the solver works

Two layers: deduction first, then search for the rest.

<details>
<summary>Deduction rules and the backtracking search</summary>

Deduction is six rules that only ever mark a cell when there's no other
option. A white letter forces its twins in the same row and column out, a
crossed cell forces its neighbours white, a cell has to stay white if
crossing it would strand a white neighbour, plus three pattern rules for
triples (`x x x`), sandwiches (`x y x`) and pairs (`x x`). The `a` and `A`
commands are just these rules applied directly, so every hint they give can
be explained.

Deduction alone isn't enough for harder boards, so `R` adds a search step on
top: it runs the rules until they stop making progress, guesses a cell,
recurses, and backtracks if a guess leads to a contradiction. That's what
makes it able to solve any board, not just the easy ones.

</details>

## Tests

`make test` runs 21 checks: all four puzzles solve and pass their own
validation, malformed files are rejected, save/reload round-trips, undo works,
and the command parsing handles the `b b3` vs `b3` ambiguity.

## Layout

```
src/        main.c (CLI), puzzle.c (game and solver), puzzle.h
puzzles/    sample boards, j1 (9x9) through j4 (5x5)
tests/      test suite
```

## Limitations

- Boards up to 20x20, since columns are labelled `a`..`z`.
- Undo history is capped at 100 moves.
- The search is plain backtracking. It is instant on these boards, but a
  deliberately adversarial 20x20 could make it work.
