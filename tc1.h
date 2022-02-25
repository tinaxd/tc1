typedef enum {
    TK_RESERVED,
    TK_IDENT,
    TK_NUM,
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
    ND_LVAR, // local variable
} NodeKind;

typedef struct Node Node;

struct Node {
    NodeKind kind;
    Node *lhs;
    Node *rhs;
    int val; // when ND_NUM
    int offset; // when ND_LVAR
};

// input program
extern char *user_input;

// token currently waching
extern Token *token;

void error(char *fmt, ...);

void error_at(char *loc, char *fmt, ...);

void gen(Node *node);

Token *tokenize(char *p);

void program();
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
