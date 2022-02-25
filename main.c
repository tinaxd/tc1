#include "tc1.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

char *user_input;
Token *token;

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "the number of arguments does not match\n");
        return 1;
    }

    user_input = argv[1];
    token = tokenize(argv[1]);
    program();

    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");
    printf("main:\n");

    printf("    push rbp\n");
    printf("    mov rbp, rsp\n");
    printf("    sub rsp, %d\n", 26*8);

    for (int i=0; code[i]; i++) {
        gen(code[i]);

        printf("    pop rax\n");
    }

    printf("    mov rsp, rbp\n");
    printf("    pop rbp\n");
    printf("    ret\n");
    return 0;
}
