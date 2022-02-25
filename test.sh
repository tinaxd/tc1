#!/bin/bash
assert() {
    expected="$1"
    input="$2"

    ./tc1 "$input" > tmp.s
    cc -o tmp tmp.s
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]
    then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

assert 0 "0;"
assert 42 "42;"
assert 21 '5+20-4;'
assert 41 " 12 + 34 - 5 ;"
assert 47 '5 + 6*7;'
assert 15 '5 * (9-6);'
assert 4 '(3+5)/2;'
assert 10 '-10 +20;'
assert 1 '1<2;'
assert 0 '1>=2;'
assert 42 "42;"
assert 2 "a=b=2;"
assert 5 "a=2;b=3;a+b;"

echo OK
