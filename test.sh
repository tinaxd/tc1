#!/bin/bash
assert() {
    expected="$1"
    input="$2"

    ./tc1 "$input" > tmp.s
    cc -o tmp tmp.s library.o
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

assert 0 "main() {0;}"
assert 42 "main() {42;}"
assert 21 'main() {5+20-4;}'
assert 41 "main() { 12 + 34 - 5 ;}"
assert 47 'main() {5 + 6*7;}'
assert 15 'main() {5 * (9-6);}'
assert 4 'main() {(3+5)/2;}'
assert 10 'main() {-10 +20;}'
assert 1 'main() {1<2;}'
assert 0 'main() {1>=2;}'
assert 42 "main() {42;}"
assert 2 "main() {a=b=2;}"
assert 3 "main() {a=1;b=3;b;}"
assert 5 "main() {a=1;b=3;c=5;c;}"
assert 3 "main() {a=1;b=3;c=5;b;}"
assert 1 "main() {a=1;b=3;c=5;a;}"
assert 1 "main() {a=1;b=3;a;}"
assert 5 "main() {a=2;b=3;a+b;}"
assert 5 "main() {a=1;b=3+1;a+b;}"
assert 5 "main() {foo=2; bar=3; foo+bar;}"
assert 5 "main() {foo=1;bar=3+1;foo+bar;}"
assert 10 "main() {return 10;}"
assert 5 "main() {a=1;b=5;return b;}"
assert 1 "main() {a=1;b=5;return a;}"
assert 2 "main() {if(1) return 2;}"
assert 2 "main() {if(1) return 2; return 3;}"
assert 3 "main() {if(0) return 2; return 3;}"
assert 2 "main() {if(1) 2; else 3;}"
assert 3 "main() {if(0) 2; else 3;}"
assert 2 "main() {if(1) return 2; else return 3;}"
assert 3 "main() {if(0) return 2; else return 3;}"
assert 3 "main() {while(1) return 3; return 2;}"
assert 2 "main() {while(0) return 3; return 2;}"
assert 4 "main() {for(i=0; i<6; i=i+1) i=i; 4;}"
assert 3 "main() {for(i=0; i<6; i=i+1) if(i == 3) return i; 4;}"
assert 100 "main() {for(i=0; i<6; i=i+1) if(i==5) return 100; i;}"
assert 7 "main() {for(i=0; i<6; i=i+1) if(i==6) return 100; 7;}"
assert 3 "main() {for(i=0; i<6; ) if((i=i+1) == 3) return i; 4;}"
assert 100 "main() {for(;;) return 100; return 50;}"
assert 3 "main() {while(1) {1; 2; return 3;}}"
assert 2 "main() {while(1) {1; return 2; 3;}}"
assert 3 "main() {for(i=0; i<10; i=i+1) { a = i + 3; return a; }}"
assert 15 "main() {sum = 0; for(i=1; i<=5; i=i+1) sum = sum + i; return sum;}"
assert 15 "main() {sum = 0; i = 1; while (i <= 5) { sum = sum + i; i = i+1; } return sum;}"
assert 2 "main() {for (i=1; i<=10; i=i+1) {a = i+1; return a;}}"
assert 100 "main() {for (i=1; i<=10; i=i+1) {a = 1; if (a == 2) return a;} return 100;}"
assert 2 "main() {for (i=1; i<=10; i=i+1) {a = i; if (a == 2) return a;} return 100;}"
assert 1 "main() {for (i=1; i<=10; i=i+1) {a = i+1; if (a == 2) return i;} return 100;}"
assert 0 "main() {foo(); return 0;}"
# assert 11 "return foo();"
assert 0 "main() {bar(3, 4); return 0;}"
assert 0 "main() {foobar(5); return 0;}"

echo OK
