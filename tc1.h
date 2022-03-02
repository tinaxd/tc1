#include <stddef.h>

typedef enum {
    TK_RESERVED,
    TK_IDENT,
    TK_NUM,
    TK_RETURN,
    TK_IF,
    TK_ELSE,
    TK_WHILE,
    TK_FOR,
    TK_EOF,
} TokenKind;

typedef struct Token Token;

struct Token {
    TokenKind kind;
    Token *next;
    int val; // when TK_NUM
    char *str;
    int len;
};

// Kinds of AST nodes
typedef enum {
    ND_ADD, // +
    ND_SUB, // -
    ND_MUL, // *
    ND_DIV, // /
    ND_LT, // <
    ND_LE, // <=
    // ND_GT, // >
    // ND_GE, // >=
    ND_EQ, // ==
    ND_NEQ, // !=
    ND_ASSIGN, // =
    ND_NUM, // integer
    ND_RETURN, // return
    ND_IF, // if n_children==2 or 3
    ND_WHILE, // while n_children==2
    ND_FOR, // for n_children==4
    ND_BLOCK, // block statement
    ND_CALL, // function call
    ND_DEF, // function definition, parameters to parameters, block statement to lhs
    ND_DEREF, // *
    ND_ADDR, // &
    ND_EMPTY, // lvar declaration
    ND_LVAR, // local variable
} NodeKind;

struct Type {
    enum {T_INT, T_PTR, T_ARRAY} ty;
    struct Type *ptr_to; // when ty is T_PTR
    size_t array_size;
};

typedef struct Type Type;

typedef struct Node Node;

struct Node {
    NodeKind kind;
    Node *lhs;
    Node *rhs;
    int val; // when ND_NUM
    int offset; // when ND_LVAR
    Node *children[200]; // used by control statements
    int n_children;
    char *funcname; // when ND_CALL
    int funcname_len;
    char *parameters[6]; // when ND_DEF
    int parameters_len[6];
    int n_parameters;
    Type ty;
};

typedef struct LVar LVar;

struct LVar {
    LVar *next; // next lvar or null
    char *name;
    int len; // length of name
    int offset; // offset from rbp
    char *funcname;
    int funcname_len;
    Type ty;
};

extern LVar *locals;

// input program
extern char *user_input;

// token currently waching
extern Token *token;

void error(char *fmt, ...);

void error_at(char *loc, char *fmt, ...);

void gen(Node *node);

Token *tokenize(char *p);

void program();
Node *definition();
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

extern Node *code[];

// Find LVar by the name.
// Returns NULL if not found.
LVar *find_lvar(Token *tok, const char *funcname, int funcname_len);
LVar *find_lvar_str(const char *name, const char *funcname, int funcname_len); // null-terminated string

int calculate_offset(Type ty);
