#!/usr/bin/env bash
#
# Feeds scripted commands to the built binary and checks what comes back.
# Run it from the project root with `make test`.

set -uo pipefail

BIN=./puzzle
pass=0
fail=0

# run <name> <input> <expected substring>
run() {
    local name=$1 input=$2 expect=$3 out
    out=$(printf '%b' "$input" | $BIN 2>&1)
    if grep -qF -- "$expect" <<< "$out"; then
        printf '  ok   %s\n' "$name"
        pass=$((pass + 1))
    else
        printf '  FAIL %s\n' "$name"
        printf '       expected to find: %s\n' "$expect"
        fail=$((fail + 1))
    fi
}

if [ ! -x "$BIN" ]; then
    echo "error: $BIN not built. Run 'make' first." >&2
    exit 1
fi

echo "Solving"
for p in j1 j2 j3 j4; do
    run "solves $p"          "l puzzles/$p.txt\nR\ns\n"    "Resolvido."
    run "$p solution valid"  "l puzzles/$p.txt\nR\nv\ns\n" "Sem erros."
done

echo "Files"
run "load reports missing file" "l nope.txt\ns\n" "nao consegui abrir"
run "save then reload"          "l puzzles/j4.txt\nR\ng /tmp/pz_rt.txt\nl /tmp/pz_rt.txt\nv\ns\n" "Sem erros."

printf '3 3\nabc\nab\ncab\n' > /tmp/pz_short.txt
run "rejects short row"    "l /tmp/pz_short.txt\ns\n" "esperava 3"

printf '99 99\nabc\n' > /tmp/pz_big.txt
run "rejects huge board"   "l /tmp/pz_big.txt\ns\n" "fora do intervalo"

printf '2 2\nab\nb1\n' > /tmp/pz_bad.txt
run "rejects non-letter"   "l /tmp/pz_bad.txt\ns\n" "caracter invalido"

echo "Moves"
run "rejects bad coord"    "l puzzles/j4.txt\nb z9\ns\n"  "Coordenada invalida"
run "undo restores"        "l puzzles/j4.txt\nr a1\nd\ns\n" "Desfeito."
run "undo with no history" "l puzzles/j4.txt\nd\ns\n"     "Nada para desfazer"
run "unknown command"      "l puzzles/j4.txt\nzzz\ns\n"   "Comando desconhecido"
run "inspect a cell"       "l puzzles/j4.txt\nb1\ns\n"    "b1:"
run "b with arg is command" "l puzzles/j4.txt\nb a1\ns\n" "pintada de branco"

echo "Hints"
run "hints stay valid"     "l puzzles/j1.txt\nA\nv\ns\n"  "Sem erros."
run "hint spots bad board" "l puzzles/j4.txt\nb a1\nb a3\na\ns\n" "Corrige o erro"

echo
echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ]
