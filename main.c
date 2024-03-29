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
    printf(".section .note.GNU-stack\n");
    printf(".section .text\n");
    printf(".globl main\n");

    for (int i=0; code[i]; i++) {
        gen(code[i]);

        if (code[i]->kind != ND_GVAR_DECL) {
            printf("    mov rsp, rbp\n");
            printf("    pop rbp\n");
            printf("    ret\n");
        }
    }
    return 0;
}
