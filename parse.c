#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include "tc1.h"

void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, " ");
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// Advances a token and returns true when the next token is one of expected symbols.
// Returns false otherwise
bool consume(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(token->str, op, token->len))
        return false;
    token = token->next;
    return true;
}

// Advances a token and returns true when the next token is a number.
// Returns false otherwise
bool consume_number(int *val) {
    if (token->kind != TK_NUM)
        return false;
    *val = token->val;
    token = token->next;
    return true;
}

// Advances a token when the next token is a number.
// Reports an error otherwise.
void expect(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(token->str, op, token->len))
        error_at(token->str, "not '%s'", op);
    token = token->next;
}

// Advances a token and returns the number when the next token is a number.
// Reports an error otherwise.
int expect_number() {
    if (token->kind != TK_NUM)
        error_at(token->str, "not a number: '%s'", token->str);
    int val = token->val;
    token = token->next;
    return val;
}

// Advances a token and returns the ident when the next token is an ident.
// Reports an error otherwise.
Token *expect_ident() {
    if (token->kind != TK_IDENT)
        error_at(token->str, "not an ident: '%s'", token->str);
    char *val = token;
    token = token->next;
    return val;
}

bool at_eof() {
    return token->kind == TK_EOF;
}

// Creates a new token and links it next to cur.
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    cur->next = tok;
    tok->len = len;
    return tok;
}

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_node_num(int val) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

Node *new_node_lvar(Token *tok) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_LVAR;

    LVar *var = find_lvar(tok);
    if (var) {
        node->offset = var->offset;
    } else {
        var = calloc(1, sizeof(LVar));
        var->next = locals;
        var->name = tok->str;
        var->len = tok->len;
        if (locals == NULL) {
            // first LVar
            var->offset = 8;
        } else {
            var->offset = locals->offset + 8;
        }
        node->offset = var->offset;
        locals = var;
    }
    // fprintf(stderr, "tok %c offset %d\n", tok->str[0], var->offset);

    return node;
}

/*
program    = stmt*
stmt       = expr ";"
expr       = assign
assign     = equality ("=" assign)?
equality   = relational ("==" relational | "!=" relational)*
relational = add ("<" add | "<=" add | ">" add | ">=" add)*
add        = mul ("+" mul | "-" mul)*
mul        = unary ("*" unary | "/" unary)*
unary      = ("+" | "-")? primary
primary    = num | ident | "(" expr ")"
*/

Node *code[100];

void program() {
    int i = 0;
    while (!at_eof()) {
        code[i++] = stmt();
    }
    code[i] = NULL;
}

Node *stmt() {
    Node *node = expr();
    expect(";");
    return node;
}

Node *expr() {
    return assign();
}

Node *assign() {
    Node *node = equality();

    if (consume("=")) {
        node = new_node(ND_ASSIGN, node, assign());
    }
    return node;
}

Node *equality() {
    Node *node = relational();
    
    while (true) {
        if (consume("=="))
            node = new_node(ND_EQ, node, relational());
        else if (consume("!="))
            node = new_node(ND_NEQ, node, relational());
        else
            return node;
    }
}

Node *relational() {
    Node *node = add();
    
    while (true) {
        if (consume("<"))
            node = new_node(ND_LT, node, add());
        else if (consume("<="))
            node = new_node(ND_LE, node, add());
        else if (consume(">"))
            node = new_node(ND_LT, add(), node);
        else if (consume(">="))
            node = new_node(ND_LE, add(), node);
        else if (consume("=="))
            node = new_node(ND_EQ, node, add());
        else if (consume("!="))
            node = new_node(ND_NEQ, node, add());
        else
            return node;
    }
}

Node *add() {
    Node *node = mul();
    
    while (true) {
        if (consume("+"))
            node = new_node(ND_ADD, node, mul());
        else if (consume("-"))
            node = new_node(ND_SUB, node, mul());
        else
            return node;
    }
}

Node *mul() {
    Node *node = unary();

    while (true) {
        if (consume("*"))
            node = new_node(ND_MUL, node, unary());
        else if (consume("/"))
            node = new_node(ND_DIV, node, unary());
        else
            return node;
    }
}

Node *unary() {
    if (consume("+"))
        return primary();
    if (consume("-"))
        return new_node(ND_SUB, new_node_num(0), primary());
    return primary();
}

Node *primary() {
    if (consume("(")) {
        Node *node = expr();
        expect(")");
        return node;
    }

    int num;
    if (consume_number(&num)) {
        return new_node_num(num);
    }

    return new_node_lvar(expect_ident());
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

        if (!memcmp(p, "==", 2) || !memcmp(p, "<=", 2) || !memcmp(p, ">=", 2) || !memcmp(p, "!=", 2)) {
            cur = new_token(TK_RESERVED, cur, p, 2);
            p += 2;
            continue;
        }

        if (*p == '<' || *p == '>') {
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        }

        if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')') {
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        }

        if (*p == '=' | *p == ';') {
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        }

        if (isdigit(*p)) {
            char *oldP = p;
            int val = strtol(oldP, &p, 10);
            int len = p - oldP;

            cur = new_token(TK_NUM, cur, p, len);
            cur->val = val;
            continue;
        }

        // LVar
        char name[100];
        int i = 0;
        char *firstP = p;
        while ('a' <= *p && *p <= 'z') {
            name[i++] = *p;
            p++;
        }
        name[i] = '\0';
        if (i != 0) {
            cur = new_token(TK_IDENT, cur, firstP, i);
            continue;
        }

        error_at(p, "cannot tokenize");
    }

    new_token(TK_EOF, cur, p, 0);
    return head.next;
}

LVar *locals = NULL;

LVar *find_lvar(Token *tok) {
    for (LVar *var = locals; var; var = var->next) {
        if (var->len == tok->len && !memcmp(var->name, tok->str, var->len)) {
            return var;
        }
    }
    return NULL;
}
