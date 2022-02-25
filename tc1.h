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
    ND_NUM, // integer
} NodeKind;

typedef struct Node Node;

struct Node {
    NodeKind kind;
    Node *lhs;
    Node *rhs;
    int val; // when ND_NUM
};

// input program
extern char *user_input;

// token currently waching
extern Token *token;

void error(char *fmt, ...);

void error_at(char *loc, char *fmt, ...);

void gen(Node *node);

Token *tokenize(char *p);

Node *expr();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();
