#!/bin/bash

SOLVER=cadical

test_instance () {
    FILE=$1
    OPT=$2

    echo "Testing $FILE (χ=$OPT)"

    # SAT test
    python3 k_colorability.py "$FILE" "$OPT" > tmp.cnf
    RES=$($SOLVER tmp.cnf | grep -E "s SATISFIABLE|s UNSATISFIABLE")

    echo $RES

    if [[ "$RES" != "s SATISFIABLE" ]]; then
        echo "FAIL: expected SATISFIABLE for k=$OPT"
        exit 1
    fi

    # UNSATISFIABLE test
    python3 k_colorability.py "$FILE" $(($OPT - 1)) > tmp.cnf
    RES=$($SOLVER tmp.cnf | grep -E "s SATISFIABLE|s UNSATISFIABLE")

    if [[ "$RES" != "s UNSATISFIABLE" ]]; then
        echo "FAIL: expected UNSATISFIABLE for k=$(($OPT-1))"
        exit 1
    fi

    echo "OK"
}

test_instance instances/queen5_5.col 5
test_instance instances/myciel3.col 4
test_instance instances/myciel4.col 5
test_instance instances/myciel5.col 6
test_instance instances/myciel6.col 7