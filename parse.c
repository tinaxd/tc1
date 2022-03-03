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

static struct {
    char *funcname;
    int len;
} current_function;

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

Node *new_node_with_ty(NodeKind kind, Node *lhs, Node *rhs, Type ty) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    node->ty = ty;
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

    LVar *var = find_lvar(tok, current_function.funcname, current_function.len);
    if (var) {
        node->offset = var->offset;
        node->ty = var->ty;
    } else {
        char varname[100];
        memcpy(varname, tok->str, tok->len);
        varname[tok->len] = 0;
        error("lvar %s is not declared\n", varname);
    }
    // fprintf(stderr, "tok %c offset %d\n", tok->str[0], var->offset);

    return node;
}

static void register_new_lvar_str(char *name, int len, Type ty);

static void register_new_lvar(Token *tok, Type ty) {
    register_new_lvar_str(tok->str, tok->len, ty);
}

static void register_new_lvar_str(char *name, int len, Type ty) {
    LVar *var = calloc(1, sizeof(LVar));
    var->next = locals;
    var->name = name;
    var->len = len;
    if (locals == NULL) {
        // first LVar
        var->offset = calculate_offset(ty);
    } else {
        int max_offset = 0;
        LVar *that = NULL;
        for (LVar *other=var->next; other; other=other->next) {
            if (other->funcname_len == current_function.len
                && memcmp(other->funcname, current_function.funcname, current_function.len) == 0) {
                    if (max_offset <= other->offset) {
                        max_offset = other->offset;
                        that = other;
                    }
                }
        }
        var->offset = max_offset + calculate_offset(ty);
    }
    var->funcname = current_function.funcname;
    var->funcname_len = current_function.len;
    var->ty = ty;
    locals = var;

    /*
    fprintf(stderr, "-- locals dump --\n");
    for (LVar *var=locals; var; var=var->next) {
        char funcname[100];
        memcpy(funcname, var->funcname, var->funcname_len);
        funcname[var->funcname_len] = 0;
        char varname[100];
        memcpy(varname, var->name, var->len);
        varname[var->len] = 0;

        fprintf(stderr, "in function %s: lvar %s, offset: %d\n", funcname, varname, var->offset);
    }
    */
}

/*
program    = definition*
definition = "int" ident ("(" ("int" ident ("," "int" ident)*)? ")")? "{" stmt* "}"
stmt       = expr ";"
           | "return" expr ";"
           | "if" "(" expr ")" stmt ("else" stmt)?
           | "while" "(" expr ")" stmt
           | "for (expr?; expr?; expr?) stmt"
           | "{" stmt* "}"
           | "int" "*"* ident ("[" num "]")? ";"
expr       = assign
assign     = equality ("=" assign)?
equality   = relational ("==" relational | "!=" relational)*
relational = add ("<" add | "<=" add | ">" add | ">=" add)*
add        = mul ("+" mul | "-" mul)*
mul        = unary ("*" unary | "/" unary)*
unary      = ("+" | "-")? subscript | "*" unary | "&" unary | "sizeof" unary
subscript  = primary ("[" expr "]")?
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
    expect("int");
    Token *ident = expect_ident();
    expect("(");
    int n_params = 0;
    char *params[6]; // TODO: 7 or more params
    int params_len[6];
    if (!consume(")")) {
        // with params
        expect("int");
        Token *id = expect_ident();
        params[n_params] = id->str;
        params_len[n_params++] = id->len;
        while (true) {
            if (consume(",")) {
                expect("int");
                Token *id1 = expect_ident();
                params[n_params] = id1->str;
                params_len[n_params++] = id1->len;
            } else {
                break;
            }
        }
        expect(")");
    }

    // set current function name
    current_function.funcname = ident->str;
    current_function.len = ident->len;

    // register params as lvars
    for (int i=0; i<n_params; i++) {
        Type ty;
        ty.ty = T_INT;
        register_new_lvar_str(params[i], params_len[i], ty);
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
    } else if (consume("int")) {
        // count the number of stars
        int n_stars = 0;
        while (consume("*")) {
            n_stars++;
        }

        Token *tok = expect_ident();

        // array check
        int is_array = 0;
        int array_size = 0;
        if (consume("[")) {
            array_size = expect_number();
            expect("]");
            is_array = 1;
        }

        expect(";");

        // construct the type based on the number of stars
        Type *p_ty = NULL;
        for (int i=0; i<n_stars+1; i++) {
            if (p_ty == NULL) {
                p_ty = malloc(sizeof(Type));
                p_ty->ty = T_INT;
            } else {
                Type *old_p_ty = p_ty;
                p_ty = malloc(sizeof(Type));
                p_ty->ty = T_PTR;
                p_ty->ptr_to = old_p_ty;
            }
        }
        if (is_array) {
            Type *old_p_ty = p_ty;
            p_ty = malloc(sizeof(Type));
            p_ty->ty = T_ARRAY;
            p_ty->ptr_to = old_p_ty;
            p_ty->array_size = array_size;
        }
        Type ty = *p_ty;
        free(p_ty);
        p_ty = NULL;

        register_new_lvar(tok, ty);
        node = calloc(1, sizeof(Node));
        node->kind = ND_EMPTY;
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

static Type make_int_type() {
    Type ty;
    ty.ty = T_INT;
    return ty;
}

Node *equality() {
    Node *node = relational();
    
    while (true) {
        if (consume("=="))
            node = new_node_with_ty(ND_EQ, node, relational(), make_int_type());
        else if (consume("!="))
            node = new_node_with_ty(ND_NEQ, node, relational(), make_int_type());
        else
            return node;
    }
}

Node *relational() {
    Node *node = add();
    
    while (true) {
        if (consume("<"))
            node = new_node_with_ty(ND_LT, node, add(), make_int_type());
        else if (consume("<="))
            node = new_node_with_ty(ND_LE, node, add(), make_int_type());
        else if (consume(">")) {
            Node *a = add();
            node = new_node_with_ty(ND_LT, a, node, make_int_type());
        } else if (consume(">=")) {
            Node *a = add();
            node = new_node_with_ty(ND_LE, a, node, make_int_type());
        } else if (consume("=="))
            node = new_node_with_ty(ND_EQ, node, add(), make_int_type());
        else if (consume("!="))
            node = new_node_with_ty(ND_NEQ, node, add(), make_int_type());
        else
            return node;
    }
}

static Type compute_add_type(Type *t1, Type *t2) {
    Type ty;
    if (t1->ty == T_ARRAY) {
        // t2->ty = T_ARRAY;
        // t2->array_size = t1->array_size;
        // t2->ptr_to = t1->ptr_to;
        memcpy(&ty, t1, sizeof(Type));
        return ty;
    } else if (t2->ty == T_ARRAY) {
        // t1->ty = T_ARRAY;
        // t1->array_size = t2->array_size;
        // t1->ptr_to = t2->ptr_to;
        memcpy(&ty, t2, sizeof(Type));
        return ty;
    } else if (t1->ty == T_PTR) {
        // t2->ty = T_PTR;
        // t2->ptr_to = t1->ptr_to;
        memcpy(&ty, t1, sizeof(Type));
        return ty;
    } else if (t2->ty == T_PTR) {
        // t1->ty = T_PTR;
        // t1->ptr_to = t2->ptr_to;
        memcpy(&ty, t2, sizeof(Type));
        return ty;
    }
    ty.ty = T_INT;
    return ty;
}

Node *add() {
    Node *node = mul();
    
    while (true) {
        if (consume("+")) {
            Node *m = mul();
            node = new_node_with_ty(ND_ADD, node, m, compute_add_type(&node->ty, &m->ty));
            // node = new_node_with_ty(ND_ADD, node, m, node->ty);
        } else if (consume("-")) {
            Node *m = mul();
            node = new_node_with_ty(ND_SUB, node, m, compute_add_type(&node->ty, &m->ty));
            // node = new_node_with_ty(ND_SUB, node, m, node->ty);
        } else {
            return node;
        }
    }
}

Node *mul() {
    Node *node = unary();

    while (true) {
        if (consume("*"))
            node = new_node_with_ty(ND_MUL, node, unary(), node->ty);
        else if (consume("/"))
            node = new_node_with_ty(ND_DIV, node, unary(), node->ty);
        else
            return node;
    }
}

int calculate_sizeof(Type ty) {
    switch (ty.ty) {
    case T_INT:
        return 4;
    case T_PTR:
        return 8;
    }
}

Node *unary() {
    if (consume("+"))
        return subscript();
    if (consume("-")) {
        Node *p = subscript();
        return new_node_with_ty(ND_SUB, new_node_num(0), p, p->ty);
    }
    if (consume("*"))
        return new_node(ND_DEREF, unary(), NULL);
    if (consume("&"))
        return new_node(ND_ADDR, unary(), NULL);
    if (consume("sizeof")) {
        Node *u = unary();
        switch (u->kind) {
        case ND_ADD:
        case ND_SUB:
        case ND_MUL:
        case ND_DIV:
        case ND_LT:
        case ND_LE:
        case ND_EQ:
        case ND_NEQ:
        case ND_ASSIGN:
        case ND_NUM:
        case ND_LVAR:
            return new_node_num(calculate_sizeof(u->ty));
        case ND_DEREF: {
            Type *ref = u->lhs->ty.ptr_to;
            if (ref == NULL) {
                error("invalid dereference");
            }
            return new_node_num(calculate_sizeof(*ref));
        }
        case ND_ADDR: {
            return new_node_num(8);
        }
        default:
            error("cannot sizeof");
        }
    }
    return subscript();
}

Node *subscript() {
    Node *p = primary();
    Node *index;
    if (consume("[")) {
        Node *index = expr();
        expect("]");
        Node *add = new_node_with_ty(ND_ADD, p, index, p->ty);
        Node *deref = new_node_with_ty(ND_DEREF, add, NULL, *add->ty.ptr_to);
        return deref;
    } else {
        return p;
    }
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

        if (*p == '[' || *p == ']') {
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

        // sizeof keyword
        if (strncmp(p, "sizeof", 6) == 0 && !is_alnum(p[6])) {
            cur = new_token(TK_RESERVED, cur, p, 6);
            p += 6;
            continue;
        }

        // int keyword
        if (strncmp(p, "int", 3) == 0 && !is_alnum(p[3])) {
            cur = new_token(TK_RESERVED, cur, p, 3);
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
        while ((i == 0 && 'a' <= *p && *p <= 'z') || (i != 0 && is_alnum(*p))) {
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

LVar *find_lvar(Token *tok, const char *funcname, int funcname_len) {
    for (LVar *var = locals; var; var = var->next) {
        if (var->len == tok->len && !memcmp(var->name, tok->str, var->len) && var->funcname_len == funcname_len && !memcmp(var->funcname, funcname, funcname_len)) {
            return var;
        }
    }
    return NULL;
}

LVar *find_lvar_str(const char *name, const char *funcname, int funcname_len) {
    int len = strlen(name);
    for (LVar *var = locals; var; var = var->next) {
        if (var->len == len && !memcmp(var->name, name, var->len) && var->funcname_len == funcname_len && !memcmp(var->funcname, funcname, funcname_len)) {
            return var;
        }
    }
    return NULL;
}
