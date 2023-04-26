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

assert 0 "int main() {0;}"
assert 42 "int main() {42;}"
assert 21 'int main() {5+20-4;}'
assert 41 "int main() { 12 + 34 - 5 ;}"
assert 47 'int main() {5 + 6*7;}'
assert 15 'int main() {5 * (9-6);}'
assert 4 'int main() {(3+5)/2;}'
assert 10 'int main() {-10 +20;}'
assert 1 'int main() {1<2;}'
assert 0 'int main() {1>=2;}'
assert 42 "int main() {42;}"
assert 2 "int main() {int a; int b;a=b=2;}"
assert 3 "int main() {int a;int b;a=1;b=3;b;}"
assert 5 "int main() {int a;int b;int c;a=1;b=3;c=5;c;}"
assert 3 "int main() {int a;int b;int c;a=1;b=3;c=5;b;}"
assert 1 "int main() {int a;int b;int c;a=1;b=3;c=5;a;}"
assert 1 "int main() {int a; int b;a=1;b=3;a;}"
assert 5 "int main() {int a; int b;a=2;b=3;a+b;}"
assert 5 "int main() {int a; int b;a=1;b=3+1;a+b;}"
assert 5 "int main() {int foo; int bar;foo=2; bar=3; foo+bar;}"
assert 5 "int main() {int foo; int bar;foo=1;bar=3+1;foo+bar;}"
assert 10 "int main() {return 10;}"
assert 5 "int main() {int a; int b; a=1;b=5;return b;}"
assert 1 "int main() {int a; int b; a=1;b=5;return a;}"
assert 2 "int main() {if(1) return 2;}"
assert 2 "int main() {if(1) return 2; return 3;}"
assert 3 "int main() {if(0) return 2; return 3;}"
assert 2 "int main() {if(1) 2; else 3;}"
assert 3 "int main() {if(0) 2; else 3;}"
assert 2 "int main() {if(1) return 2; else return 3;}"
assert 3 "int main() {if(0) return 2; else return 3;}"
assert 3 "int main() {while(1) return 3; return 2;}"
assert 2 "int main() {while(0) return 3; return 2;}"
assert 4 "int main() {int i; for(i=0; i<6; i=i+1) i=i; 4;}"
assert 3 "int main() {int i; for(i=0; i<6; i=i+1) if(i == 3) return i; 4;}"
assert 100 "int main() {int i; for(i=0; i<6; i=i+1) if(i==5) return 100; i;}"
assert 7 "int main() {int i; for(i=0; i<6; i=i+1) if(i==6) return 100; 7;}"
assert 3 "int main() {int i; for(i=0; i<6; ) if((i=i+1) == 3) return i; 4;}"
assert 100 "int main() {for(;;) return 100; return 50;}"
assert 3 "int main() {while(1) {1; 2; return 3;}}"
assert 2 "int main() {while(1) {1; return 2; 3;}}"
assert 3 "int main() {int i; for(i=0; i<10; i=i+1) { int a; a = i + 3; return a; }}"
assert 15 "int main() {int sum; int i; sum = 0; for(i=1; i<=5; i=i+1) sum = sum + i; return sum;}"
assert 15 "int main() {int sum; int i; sum = 0; i = 1; while (i <= 5) { sum = sum + i; i = i+1; } return sum;}"
assert 2 "int main() {int i; int a; for (i=1; i<=10; i=i+1) {a = i+1; return a;}}"
assert 100 "int main() {int i; int a; for (i=1; i<=3; i=i+1) {a = 1; if (a == 2) return a;} return 100;}"
assert 2 "int main() {int i; int a; for (i=1; i<=10; i=i+1) {a = i; if (a == 2) return a;} return 100;}"
assert 1 "int main() {int i; int a; for (i=1; i<=10; i=i+1) {a = i+1; if (a == 2) return i;} return 100;}"
assert 0 "int main() {foo(); return 0;}"
# assert 11 "return foo();"
assert 0 "int main() {bar(3, 4); return 0;}"
assert 0 "int main() {foobar(5); return 0;}"
assert 3 "int three() {return 3;} int main() {int a; a= three(); return a;}"
assert 3 "int three() {return 3;} int main() {return three();}"
assert 5 "int add(int a, int b) {return a+b;} int main() {return add(2, 3);}"
assert 5 "int three() {return 3;} int two() {return 2;} int main() {return two()+three();}"
assert 5 "int add(int a, int b) {return a+b;} int main() {return add(2, add(2,1));}"
assert 5 "int add(int a, int b) {return a+b;} int sub(int c, int d) {return c-d;} int main() {return add(2, sub(5, 2));}"
assert 5 "int add(int a, int b) {return a+b;} int sub(int a, int b) {return a-b;} int main() {return add(2, sub(5, 2));}"
assert 55 "int fibo(int x) { if(x==0) return 0; if(x==1) return 1; return fibo(x-1)+fibo(x-2);} int main(){return fibo(10);}"
assert 3 "int main() {int x; int *y; x=3; y=&x; return *y;}"
assert 3 "int main() {int x; int *y; y = &x; *y = 3; return x;}"
assert 4 "int main() {int *p; alloc4(&p, 1, 2, 4, 8); int *q; q = p+2; return *q;}"
assert 8 "int main() {int *p; alloc4(&p, 1, 2, 4, 8); int *q; q = p+2; *q; q=q+1; return *q;}"
assert 4 "int main() {int x; return sizeof(x);}"
assert 8 "int main() {int *y; return sizeof(y);}"
assert 4 "int main() {int x; return sizeof(x+3);}"
assert 8 "int main() {int *y; return sizeof(y+3);}"
assert 4 "int main() {return sizeof(1);}"
assert 8 "int main() {int *y; return sizeof(y);}"
assert 4 "int main() {int *y; return sizeof(*y);}"
assert 8 "int main() {int x; return sizeof(&x);}"
assert 4 "int main() {return sizeof(sizeof(1));}"
assert 0 "int main() {int a[10]; return 0;}"
assert 3 "int main() {int a[1]; *a=2; return *a+1; }"
assert 4 "int main() {int a[2]; *a=3; return *a+1; }"
assert 5 "int main() {int a[2]; *(a+1)=4; return *(a+1)+1; }"
assert 5 "int main() {int a[2]; *(a+1)=4; *a=3; return *(a+1)+1; }"
assert 5 "int main() {int a[2]; *a=4; *(a+1)=3; return (*a)+1; }"
assert 3 "int main() {int a[2]; *a=1; *(a+1)=2; return *a + *(a+1); }"
assert 0 "int main() {int a[10]; int i; for(i=0; i<10; i=i+1) {*(a+i)=i;} return 0;}"
assert 45 "int main() {int a[10]; int i; for(i=0; i<10; i=i+1) {*(a+i)=i;} int sum; sum=0;for(i=0; i<10; i=i+1) sum = sum+ (*(a+i)); return sum;}"
assert 5 "int main() {int a[2]; a[1]=4; return a[1]+1; }"
assert 5 "int main() {int a[2]; a[1]=4; *a=3; return a[1]+1; }"
assert 5 "int main() {int a[2]; a[0]=4; a[1]=3; return a[0]+1; }"
assert 3 "int main() {int a[2]; a[0]=1; a[1]=2; return a[0] + a[1]; }"
assert 5 "int main() {int a[2]; *(1+a)=4; return *(1+a)+1; }"
assert 5 "int main() {int a[2]; *(1+a)=4; *a=3; return *(1+a)+1; }"
assert 5 "int main() {int a[2]; *a=4; *(1+a)=3; return (*a)+1; }"
assert 3 "int main() {int a[2]; *a=1; *(1+a)=2; return *a + *(1+a); }"
assert 5 "int main() {int a[2]; 1[a]=4; return 1[a]+1; }"
assert 5 "int main() {int a[2]; 1[a]=4; 0[a]=3; return 1[a]+1; }"
assert 5 "int main() {int a[2]; 0[a]=4; 1[a]=3; return 0[a]+1; }"
assert 3 "int main() {int a[2]; 0[a]=1; 1[a]=2; return 0[a] + 1[a]; }"
assert 8 "int main() {int a[2]; return sizeof(a);}"

echo OK
