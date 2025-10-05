#!/usr/bin/env bash
set -euo pipefail

OUT=results.csv
TRACE=traces/stride.hex

# generate a repeatable trace (64 KiB array, 64B lines, stride=4 lines, 200k accesses)
./gen-stride 65536 64 4 200000 > "$TRACE"

echo "size_kb,line,assoc,policy,miss_rate,compulsory,capacity,conflict,total,hits,misses" > "$OUT"

for SZ in 8 16 32 64 128; do
  ./cache-sim --size $SZ --line 64 --assoc 1 --trace "$TRACE" | tail -n1 >> "$OUT"
  ./cache-sim --size $SZ --line 64 --assoc 4 --trace "$TRACE" | tail -n1 >> "$OUT"
done

echo "Wrote $OUT"
