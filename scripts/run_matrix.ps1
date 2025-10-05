$OUT = "results.csv"
$TRACE = "traces\stride.hex"

.\gen-stride 65536 64 4 200000 | Out-File -Encoding ascii $TRACE

"size_kb,line,assoc,policy,miss_rate,compulsory,capacity,conflict,total,hits,misses" | Out-File -Encoding ascii $OUT

foreach ($SZ in 8,16,32,64,128) {
  .\cache-sim --size $SZ --line 64 --assoc 1 --trace $TRACE | Select-Object -Last 1 | Out-File -Append -Encoding ascii $OUT
  .\cache-sim --size $SZ --line 64 --assoc 4 --trace $TRACE | Select-Object -Last 1 | Out-File -Append -Encoding ascii $OUT
}

Write-Host "Wrote $OUT"
