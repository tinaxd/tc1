#include <stdio.h>

int foo() {
    printf("foo OK!\n");
    return 11;
}

int bar(int a, int b) {
    printf("bar %d+%d=%d OK!\n", a, b, a+b);
    return 12;
}

int foobar(int a) {
    printf("foobar a=%d OK!\n", a);
    return 13;
}
