#!/bin/bash
PASS=0
FAIL=0
COMPILER=./mycc

run_test() {
    local name="$1"
    local src="$2"
    local expected_exit="$3"
    local expected_out="$4"

    $COMPILER "$src" -o /tmp/test_bin 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "FAIL $name: compilation failed"
        FAIL=$((FAIL+1))
        return
    fi

    actual_out=$(/tmp/test_bin 2>/dev/null)
    actual_exit=$?

    if [ "$actual_exit" -eq "$expected_exit" ]; then
        if [ -z "$expected_out" ] || [ "$actual_out" = "$expected_out" ]; then
            echo "PASS $name"
            PASS=$((PASS+1))
        else
            echo "FAIL $name: expected stdout '$expected_out' got '$actual_out'"
            FAIL=$((FAIL+1))
        fi
    else
        echo "FAIL $name: expected exit $expected_exit got $actual_exit"
        FAIL=$((FAIL+1))
    fi
}

run_test "t1_arithmetic" tests/t1_arithmetic.c 26 ""
run_test "t2_if"         tests/t2_if.c         1  ""
run_test "t3_while"      tests/t3_while.c      55 ""
run_test "t4_factorial"  tests/t4_factorial.c  120 ""
run_test "t5_fibonacci"  tests/t5_fibonacci.c  55 "55"

echo ""
echo "Results: $PASS passed, $FAIL failed"
[ $FAIL -eq 0 ] && exit 0 || exit 1
