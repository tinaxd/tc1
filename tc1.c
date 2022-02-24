#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

typedef enum {
    TK_RESERVED,
    TK_NUM,
    TK_EOF,
} TokenKind;

typedef struct Token Token;

struct Token {
    TokenKind kind;
    Token *next;
    int val; // when TK_NUM
    char *str;
};

// token currently waching
Token *token;

void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// Advances a token and returns true when the next token is one of expected symbols.
// Returns false otherwise
bool consume(char op) {
    if (token->kind != TK_RESERVED || token->str[0] != op)
        return false;
    token = token->next;
    return true;
}

// Advances a token when the next token is a number.
// Reports an error otherwise.
void expect(char op) {
    if (token->kind != TK_RESERVED || token->str[0] != op)
        error("not '%c'", op);
    token = token->next;
}

// Advances a token and returns the number when the next token is a number.
// Reports an error otherwise.
int expect_number() {
    if (token->kind != TK_NUM)
        error("not a number");
    int val = token->val;
    token = token->next;
    return val;
}

bool at_eof() {
    return token->kind == TK_EOF;
}

// Creates a new token and links it next to cur.
Token *new_token(TokenKind kind, Token *cur, char *str) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    cur->next = tok;
    return tok;
}

Token *tokenize(char *p) {
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
        // Skip spaces
        if (isspace(*p)) {
            p++;
            continue;
        }

        if (*p == '+' || *p == '-') {
            cur = new_token(TK_RESERVED, cur, p++);
            continue;
        }

        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p);
            cur->val = strtol(p, &p, 10);
            continue;
        }

        error("cannot tokenize");
    }

    new_token(TK_EOF, cur, p);
    return head.next;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "the number of arguments does not match\n");
        return 1;
    }

    token = tokenize(argv[1]);


    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");
    printf("main:\n");

    // The first term must be a number.
    // Outputs the first mov.
    printf("    mov rax, %d\n", expect_number());
    
    // Consumes a sequence of `+ <number>` and `- <number>`
    // and outputs assembly.
    while (!at_eof()) {
        if (consume('+')) {
            printf("    add rax, %d\n", expect_number());
            continue;
        }

        expect('-');
        printf("    sub rax, %d\n", expect_number());
    }

    printf("    ret\n");
    return 0;
}
