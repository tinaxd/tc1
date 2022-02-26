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

// Advances a token and returns true when the next token is specified kind.
// Returns false otherwise
bool consume_kind(TokenKind kind) {
    if (token->kind != kind)
        return false;
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
    Token *val = token;
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
program    = definition*
definition = ident ("(" (ident (", ident)*)? ")")? "{" stmt* "}"
stmt       = expr ";"
           | "return" expr ";"
           | "if" "(" expr ")" stmt ("else" stmt)?
           | "while" "(" expr ")" stmt
           | "for (expr?; expr?; expr?) stmt"
           | "{" stmt* "}"
expr       = assign
assign     = equality ("=" assign)?
equality   = relational ("==" relational | "!=" relational)*
relational = add ("<" add | "<=" add | ">" add | ">=" add)*
add        = mul ("+" mul | "-" mul)*
mul        = unary ("*" unary | "/" unary)*
unary      = ("+" | "-")? primary | "*" unary | "&" unary
primary    = num | ident ("(" (expr (", expr)*)? ")")? | "(" expr ")"
*/

Node *code[100];

void program() {
    int i = 0;
    while (!at_eof()) {
        code[i++] = definition();
    }
    code[i] = NULL;
}

Node *definition() {
    Token *ident = expect_ident();
    expect("(");
    int n_params = 0;
    char *params[6]; // TODO: 7 or more params
    int params_len[6];
    if (!consume(")")) {
        // with params
        Token *id = expect_ident();
        params[n_params] = id->str;
        params_len[n_params++] = id->len;
        while (true) {
            if (consume(",")) {
                Token *id1 = expect_ident();
                params[n_params] = id1->str;
                params_len[n_params++] = id1->len;
            } else {
                break;
            }
        }
        expect(")");
    }

    consume("{");
    Node *body = calloc(1, sizeof(Node));
    body->kind = ND_BLOCK;
    int n_stmts = 0;
    while (1) {
        if (consume("}")) {
            break;
        }
        body->children[n_stmts++] = stmt();
    }
    body->n_children = n_stmts;

    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_DEF;
    node->funcname = ident->str;
    node->funcname_len = ident->len;
    node->lhs = body;
    memcpy(node->parameters, params, n_params * sizeof(char *));
    memcpy(node->parameters_len, params_len, n_params * sizeof(int));
    node->n_parameters = n_params;
    return node;
}

Node *stmt() {
    Node *node;

    if (consume_kind(TK_IF)) {
        expect("(");
        Node *n1 = expr();
        expect(")");
        Node *n2 = stmt();

        // else clause
        Node *n3 = NULL;
        if (consume_kind(TK_ELSE)) {
            n3 = stmt();
        }

        node = calloc(1, sizeof(Node));
        node->kind = ND_IF;
        node->children[0] = n1;
        node->children[1] = n2;
        if (n3 == NULL) {
            // without else clause
            node->n_children = 2;
        } else {
            // with else clause
            node->children[2] = n3;
            node->n_children = 3;
        }
    } else if (consume_kind(TK_WHILE)) {
        expect("(");
        Node *n1 = expr();
        expect(")");
        Node *n2 = stmt();

        node = calloc(1, sizeof(Node));
        node->kind = ND_WHILE;
        node->children[0] = n1;
        node->children[1] = n2;
        node->n_children = 2;
    } else if (consume_kind(TK_FOR)) {
        expect("(");
        Node *n1 = NULL;
        Node *n2 = NULL;
        Node *n3 = NULL;
        if (!consume(";")) {
            n1 = expr();
            expect(";");
        }
        if (!consume(";")) {
            n2 = expr();
            expect(";");
        }
        if (!consume(")")) {
            n3 = expr();
            expect(")");
        }
        Node *n4 = stmt();

        node = calloc(1, sizeof(Node));
        node->kind = ND_FOR;
        node->children[0] = n1;
        node->children[1] = n2;
        node->children[2] = n3;
        node->children[3] = n4;
        node->n_children = 4;
    } else if (consume_kind(TK_RETURN)) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_RETURN;
        node->lhs = expr();
        expect(";");
    } else if (consume("{")) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_BLOCK;
        int n_stmts = 0;
        while (1) {
            if (consume("}")) {
                break;
            }
            node->children[n_stmts++] = stmt();
        }
        node->n_children = n_stmts;
    } else {
        node = expr();
        expect(";");
    }

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
    if (consume("*"))
        return new_node(ND_DEREF, unary(), NULL);
    if (consume("&"))
        return new_node(ND_ADDR, unary(), NULL);
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

    Token *ident = expect_ident();
    if (consume("(")) {
        // function call
        int n_arguments = 0;
        Node *arguments[6]; // TODO: 7 or more arguments
        if (!consume(")")) {
            // with arguments
            Node *e = expr();
            arguments[n_arguments++] = e;
            while (true) {
                if (consume(",")) {
                    Node *e1 =expr();
                    arguments[n_arguments++] = e1;
                } else {
                    break;
                }
            }
            expect(")");
        }

        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_CALL;
        node->funcname = ident->str;
        node->funcname_len = ident->len;
        memcpy(node->children, arguments, n_arguments * sizeof(Node*));
        node->n_children = n_arguments;
        return node;
    } else {
        // local variable
        return new_node_lvar(ident);
    }
}

static int is_alnum(char c) {
    return ('a' <= c && c <= 'z')
           || ('A' <= c && c <= 'Z')
           || ('0' <= c && c <= '9')
           || (c == '_');
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

        if (*p == '=' || *p == ';') {
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        }

        if (*p == '{' || *p == '}' || *p == ',') {
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        }

        if (*p == '*' || *p == '&') {
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        }

        // return keyword
        if (strncmp(p, "return", 6) == 0 && !is_alnum(p[6])) {
            cur = new_token(TK_RETURN, cur, p, 6);
            p += 6;
            continue;
        }

        // if keyword
        if (strncmp(p, "if", 2) == 0 && !is_alnum(p[2])) {
            cur = new_token(TK_IF, cur, p, 2);
            p += 2;
            continue;
        }

        // else keyword
        if (strncmp(p, "else", 4) == 0 && !is_alnum(p[4])) {
            cur = new_token(TK_ELSE, cur, p, 4);
            p += 4;
            continue;
        }

        // while keyword
        if (strncmp(p, "while", 5) == 0 && !is_alnum(p[5])) {
            cur = new_token(TK_WHILE, cur, p, 5);
            p += 5;
            continue;
        }

        // for keyword
        if (strncmp(p, "for", 3) == 0 && !is_alnum(p[3])) {
            cur = new_token(TK_FOR, cur, p, 3);
            p += 3;
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

LVar *find_lvar_str(const char *name) {
    int len = strlen(name);
    for (LVar *var = locals; var; var = var->next) {
        if (var->len == len && !memcmp(var->name, name, var->len)) {
            return var;
        }
    }
    return NULL;
}
