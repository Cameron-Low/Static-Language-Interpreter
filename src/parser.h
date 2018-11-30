#include <stdbool.h>
#include "lexer.h"

// -----------------
// Public Objects
// -----------------

// Represents the types a variable can take;
typedef enum Type {
    NUM, BOOL, UNKNOWN
} Type;

// All possible statements;
typedef enum Stmt {
    IF, WHILE, VARDEC, VARASSIGN, SHOW, BINOP, UNOP, BRACKET, LITERAL, VAR
} Stmt;

// Not really a tree but a dynamic array containing all statements in the program, in order of execution.
typedef struct ParseTree {
    int index;
    int size;
    void **stmts;
} ParseTree;

// ----------------
// Statement types
// ----------------

typedef struct IfStmt {
    Stmt s;
    void *cond;
    ParseTree trueBranch;
} IfStmt;

typedef struct WhileStmt {
    Stmt s;
    void *cond;
    ParseTree trueBranch;
} WhileStmt;

typedef struct ShowStmt {
    Stmt s;
    void *expr;
} ShowStmt;

typedef struct VarDecStmt {
    Stmt s;
    char id[100];
    Type type;
} VarDecStmt;

typedef struct VarAssignStmt {
    Stmt s;
    char id[100];
    void *expr;
} VarAssignStmt;

typedef struct BinOpExpr {
    Stmt s;
    void *left;
    void *right;
    char op[5];
} BinOpExpr;

typedef struct UnOpExpr {
    Stmt s;
    void *right;
    char op[5];
} UnOpExpr;

typedef struct BracketExpr {
    Stmt s;
    void *expr;
} BracketExpr;

typedef struct LiteralExpr {
    Stmt s;
    char val[100];
} LiteralExpr;

typedef struct VarExpr {
    Stmt s;
    char id[100];
} VarExpr;

// LL(1) parser object.
typedef struct Parser {
    int index;
    Token lookahead;
    Token current;
    bool err;
    ParseTree tree;
    Token *tokStream;
} Parser;

// -----------------
// Public Functions
// -----------------

void initParser(Parser *p, Lexer *l);
void parse(Parser *p);
void printTree(ParseTree t);
