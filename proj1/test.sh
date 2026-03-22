#!/bin/bash

SOLVER=cadical
TIMEOUT=${TIMEOUT:-30}

time_solve () {
    FILE=$1
    K=$2
    EXTRA_ARGS=${3:-}

    python3 k_colorability.py "$FILE" "$K" $EXTRA_ARGS > tmp.cnf
    /usr/bin/time -f "%e" timeout "$TIMEOUT" $SOLVER tmp.cnf > /dev/null 2> tmp.time
    if [[ $? -eq 124 ]]; then
        echo "TIMEOUT"
    else
        grep -E '^[0-9]' tmp.time
    fi
}

benchmark_instance () {
    FILE=$1
    K=$2

    T_NORMAL=$(time_solve "$FILE" "$K")
    T_NO_AMO=$(time_solve "$FILE" "$K" "--disable-amoc")

    if [[ "$T_NORMAL" == "TIMEOUT" || "$T_NO_AMO" == "TIMEOUT" ]]; then
        LC_ALL=C printf "  %-28s  k=%s  %9s  %9s  ratio: %s\n" \
            "$(basename "$FILE")" "$K" "$T_NORMAL" "$T_NO_AMO" "N/A"
    else
        # Compute speedup ratio as a float (no-amo / normal), >1 means no-amo is slower
        RATIO=$(awk -v n="$T_NORMAL" -v a="$T_NO_AMO" 'BEGIN { if (n+0 > 0) printf "%.2f", a/n; else print "N/A" }')
        LC_ALL=C printf "  %-28s  k=%s  %8.3fs  %8.3fs  ratio: %s\n" \
            "$(basename "$FILE")" "$K" "$T_NORMAL" "$T_NO_AMO" "$RATIO"
    fi
}

printf "\n=== Benchmark ===\n"
printf "  %-28s  %-3s  %9s  %9s  %s\n" "instance" "k" "normal" "no-amo" "no-amo/normal"
printf "  %s\n" "$(printf '%0.s-' {1..70})"
# REG
benchmark_instance instances/fpsol2.i.1.col 65
benchmark_instance instances/fpsol2.i.1.col 64
benchmark_instance instances/fpsol2.i.2.col 30
benchmark_instance instances/fpsol2.i.2.col 29
benchmark_instance instances/fpsol2.i.3.col 30
benchmark_instance instances/fpsol2.i.3.col 29
echo ""
benchmark_instance instances/inithx.i.1.col 54
benchmark_instance instances/inithx.i.1.col 53
benchmark_instance instances/inithx.i.2.col 31
benchmark_instance instances/inithx.i.2.col 30
benchmark_instance instances/inithx.i.3.col 31
benchmark_instance instances/inithx.i.3.col 30
echo ""
benchmark_instance instances/mulsol.i.1.col 49
benchmark_instance instances/mulsol.i.1.col 48
benchmark_instance instances/mulsol.i.2.col 31
benchmark_instance instances/mulsol.i.2.col 30
benchmark_instance instances/mulsol.i.3.col 31
benchmark_instance instances/mulsol.i.3.col 30
benchmark_instance instances/mulsol.i.4.col 31
benchmark_instance instances/mulsol.i.4.col 30
benchmark_instance instances/mulsol.i.5.col 31
benchmark_instance instances/mulsol.i.5.col 30
echo ""
benchmark_instance instances/zeroin.i.1.col 49
benchmark_instance instances/zeroin.i.1.col 48
benchmark_instance instances/zeroin.i.2.col 30
benchmark_instance instances/zeroin.i.2.col 29
benchmark_instance instances/zeroin.i.3.col 30
benchmark_instance instances/zeroin.i.3.col 29
echo ""
# LEI
benchmark_instance instances/le450_5a.col 5
benchmark_instance instances/le450_5a.col 4
benchmark_instance instances/le450_5b.col 5
benchmark_instance instances/le450_5b.col 4
benchmark_instance instances/le450_5c.col 5
benchmark_instance instances/le450_5c.col 4
benchmark_instance instances/le450_5d.col 5
benchmark_instance instances/le450_5d.col 4
echo ""
benchmark_instance instances/le450_15a.col 15
benchmark_instance instances/le450_15a.col 14
benchmark_instance instances/le450_15b.col 15
benchmark_instance instances/le450_15b.col 14
benchmark_instance instances/le450_15c.col 15
benchmark_instance instances/le450_15c.col 14
benchmark_instance instances/le450_15d.col 15
benchmark_instance instances/le450_15d.col 14
echo ""
benchmark_instance instances/le450_25a.col 25
benchmark_instance instances/le450_25a.col 24
benchmark_instance instances/le450_25b.col 25
benchmark_instance instances/le450_25b.col 24
benchmark_instance instances/le450_25c.col 25
benchmark_instance instances/le450_25c.col 24
benchmark_instance instances/le450_25d.col 25
benchmark_instance instances/le450_25d.col 24
echo ""
# SGB
benchmark_instance instances/miles250.col 8
benchmark_instance instances/miles250.col 7
benchmark_instance instances/miles500.col 20
benchmark_instance instances/miles500.col 19
benchmark_instance instances/miles750.col 31
benchmark_instance instances/miles750.col 30
benchmark_instance instances/miles1000.col 42
benchmark_instance instances/miles1000.col 41
benchmark_instance instances/miles1500.col 73
benchmark_instance instances/miles1500.col 72
echo ""
benchmark_instance instances/games120.col 9
benchmark_instance instances/games120.col 8
echo ""
benchmark_instance instances/anna.col 11
benchmark_instance instances/anna.col 10
benchmark_instance instances/david.col 11
benchmark_instance instances/david.col 10
benchmark_instance instances/homer.col 13
benchmark_instance instances/homer.col 12
benchmark_instance instances/huck.col 11
benchmark_instance instances/huck.col 10
benchmark_instance instances/jean.col 10
benchmark_instance instances/jean.col 9
echo ""
benchmark_instance instances/queen5_5.col 5
benchmark_instance instances/queen5_5.col 4
benchmark_instance instances/queen6_6.col 7
benchmark_instance instances/queen6_6.col 6
benchmark_instance instances/queen7_7.col 7
benchmark_instance instances/queen7_7.col 6
benchmark_instance instances/queen8_8.col 9
benchmark_instance instances/queen8_8.col 8
benchmark_instance instances/queen8_12.col 12
benchmark_instance instances/queen8_12.col 11
benchmark_instance instances/queen9_9.col 10
benchmark_instance instances/queen9_9.col 9
benchmark_instance instances/queen11_11.col 11
benchmark_instance instances/queen11_11.col 10
benchmark_instance instances/queen13_13.col 13
benchmark_instance instances/queen13_13.col 12
echo ""
# MYC
benchmark_instance instances/myciel3.col 4
benchmark_instance instances/myciel3.col 3
benchmark_instance instances/myciel4.col 5
benchmark_instance instances/myciel4.col 4
benchmark_instance instances/myciel5.col 6
benchmark_instance instances/myciel5.col 5
benchmark_instance instances/myciel6.col 7
benchmark_instance instances/myciel6.col 6
benchmark_instance instances/myciel7.col 8
benchmark_instance instances/myciel7.col 7
printf "  %s\n\n" "$(printf '%0.s-' {1..70})"

