#!/usr/bin/env bash
# Compile-time checks for OneMenu's rules() inside wrapper components (Hidden/Decor/NumField/EnDis, MenuPrinter/ItemPrinter/AsFmt).
#   ok_*.cpp     must compile (positive controls: the failing cases differ from these only by the violation)
#   other *.cpp  must FAIL to compile AND print the text on their "// EXPECT-ERROR:" line, so a case cannot pass for the wrong reason
# Needs the sibling HAPI/OneData/OneItem/... checkouts next to OneMenu (IOP=<dir holding them>; default: OneMenu's parent).
# Usage: test/negative/run.sh   (CXX=clang++ to use another compiler; exits non-zero on any mismatch)
set -u
cd "$(dirname "$0")"
IOP=${IOP:-$(cd ../../.. && pwd)}
CXX=${CXX:-g++}
FLAGS="-std=c++17 -fsyntax-only"
for p in HAPI OneBit OnePin OneBus OneChip OneData OneOutput OneItem OneParse OneInput OneIO OneMenu; do FLAGS="$FLAGS -I$IOP/$p/include"; done
bad=0; n=0
for f in *.cpp; do
  n=$((n+1))
  out=$($CXX $FLAGS "$f" 2>&1); rc=$?
  case "$f" in
    ok_*) if [ $rc -eq 0 ]; then echo "ok   $f"; else echo "FAIL $f: should compile"; printf '%s\n' "$out" | grep -m1 error; bad=$((bad+1)); fi;;
    *)    expect=$(sed -n 's|^// EXPECT-ERROR: *||p' "$f" | head -1)
          if [ -z "$expect" ]; then echo "BAD  $f: no '// EXPECT-ERROR:' line"; bad=$((bad+1))
          elif [ $rc -eq 0 ]; then echo "FAIL $f: compiled, but an error was expected"; bad=$((bad+1))
          elif printf '%s' "$out" | grep -qF -- "$expect"; then echo "ok   $f"
          else echo "FAIL $f: failed to compile, but without the expected text: $expect"; bad=$((bad+1)); fi;;
  esac
done
echo "$((n-bad))/$n OneMenu rule cases behave as expected ($CXX)"
[ "$bad" -eq 0 ]
