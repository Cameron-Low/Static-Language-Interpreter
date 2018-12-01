// LL(1) parser for the CAM programming langauge.

#include "parser.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

// -----------------
// Private Functions
// -----------------

void add(ParseTree *p, void *stmt);
Token pNext(Parser *p);
Token prev(Parser *p);
void pError(Parser *p, char *msg);
bool matchKeyword(Parser *p, char *str);
bool match(Parser *p, TokenType t);
bool require(Parser *p, TokenType t, char *msg);
bool requireKeyword(Parser *p, char *str, char *msg);
void printStmt(void *stmt);
void *statement(Parser *p);
void *varDecStmt(Parser *p);
void *varAssignStmt(Parser *p);
void *ifStmt(Parser *p);
void *whileStmt(Parser *p);
void *showStmt(Parser *p);
void *expression(Parser *p);
void *equ(Parser *p);
void *comp(Parser *p);
void *adds(Parser *p);
void *mul(Parser *p);
void *unary(Parser *p);
void *primary(Parser *p);

// -----------------
// Main funcs
// -----------------

void initParser(Parser *p, Lexer *l) {
    p->tokStream = l->tokens;
    p->err = l->err;
    p->index = 1;
    p->current = l->tokens[0];
    p->lookahead = l->tokens[1];
    p->tree = (ParseTree) {0,5,NULL};
}

void parse(Parser *p) {
    while (!(p->err || p->current.type == END)) {
        void *stmt = statement(p);
        if (stmt == (void *)-1) return;
        add(&p->tree, stmt);
    }
}

// -----------------
// Grammar funcs
// These functions follow the EBNF grammar that can be found in the readme.txt.
// -----------------

void *statement(Parser *p) {
    if (matchKeyword(p, "let")) {
        return varDecStmt(p);
    } else if (match(p, ID)) {
        return varAssignStmt(p);
    } else if (matchKeyword(p, "if")) {
        return ifStmt(p);
    } else if (matchKeyword(p, "while")) {
        return whileStmt(p);
    } else if (matchKeyword(p, "show")) {
        return showStmt(p);
    } else {
        if (p->current.type != END) pError(p, "Unrecognised syntax.");
        return (void *) -1;
    }
}

void *varDecStmt(Parser *p) {
    if(!require(p, ID, "Expected identifier.")) return (void *) -1;
    Token id = prev(p);
    if(!requireKeyword(p, "be", "Expected 'be'.")) return (void *) -1;
    if(!require(p, TYPES, "Expected type.")) return (void *) -1;
    Token type = prev(p);
    if(!require(p, SEMICOLON, "Expected semicolon.")) return (void *) -1;
    Type t;
    if (!strcmp(type.lexeme, "bool")) {
        t = BOOL;
    } else if (!strcmp(type.lexeme, "num")) {
        t = NUM;
    } else {
        t = UNKNOWN;
    }  
    VarDecStmt *stmt = malloc(sizeof(VarDecStmt));
    strcpy(stmt->id, id.lexeme);
    stmt->type = t;
    stmt->s = VARDEC;
    return (void *) stmt;
}

void *varAssignStmt(Parser *p) {
    Token id = prev(p);
    if (!require(p, EQUALS, "Missing '=' for assignment.")) return (void *) -1;
    void *expr = expression(p);
    if (!require(p, SEMICOLON, "Expected semicolon.")) return (void *) -1;
    VarAssignStmt *stmt = malloc(sizeof(VarAssignStmt));
    strcpy(stmt->id, id.lexeme);
    stmt->expr = expr;
    stmt->s = VARASSIGN;
    return (void *) stmt;
}

void *ifStmt(Parser *p) {
    void *cond = expression(p);
    if (!requireKeyword(p, "then", "Expected 'then' after condition.")) return (void *) -1;
    IfStmt *stmt = malloc(sizeof(IfStmt));
    stmt->cond = cond;
    stmt->s = IF;
    stmt->trueBranch = (ParseTree) {0,5,NULL};
    void *tb = statement(p);
    if (tb == (void *)-1) return tb;
    add(&stmt->trueBranch, tb);
    int maxRepeat = 100000;
    int repeats = 0;
    while(!matchKeyword(p, "endif") && (repeats != maxRepeat)) {
        repeats++;
        tb = statement(p);
        if (tb == (void *)-1) {
            pError(p, "Expected 'endif' closing if statement.");
            return tb;
        }
        add(&stmt->trueBranch, tb);
    }
    if (repeats == maxRepeat) pError(p, "Expected 'endif' closing if statement.");
    return (void *) stmt;
}

void *whileStmt(Parser *p) {
    void *cond = expression(p);
    if (!requireKeyword(p, "do", "Expected 'do' after condition.")) return (void *) -1;
    WhileStmt *stmt = malloc(sizeof(WhileStmt));
    stmt->cond = cond;
    stmt->s = WHILE;
    stmt->trueBranch = (ParseTree) {0,5,NULL};
    void *tb = statement(p);
    if (tb == (void *)-1) return tb;
    add(&stmt->trueBranch, tb);
    int maxRepeat = 100000;
    int repeats = 0;
    while(!matchKeyword(p, "endwhile") && (repeats != maxRepeat)) {
        repeats++;
        tb = statement(p);
        if (tb == (void *)-1) {
            pError(p, "Expected 'endwhile' closing while statement.");
            return tb;
        }
        add(&stmt->trueBranch, tb);
    }
    if (repeats == maxRepeat) pError(p, "Expected 'endwhile' closing while statement.");
    return (void *) stmt;
}

void *showStmt(Parser *p) {
    void *expr = expression(p);
    if (!require(p, SEMICOLON, "Expected semicolon.")) return (void *) -1;
    ShowStmt *stmt = malloc(sizeof(ShowStmt));
    stmt->s = SHOW;
    stmt->expr = expr;
    return (void *) stmt;
}

void *expression(Parser *p) {
    void *left = equ(p);
    while (match(p, AND) || match(p, OR)) {
        Token op = prev(p);
        void *right = equ(p);
        BinOpExpr *expr = malloc(sizeof(BinOpExpr));
        expr->s = BINOP;
        strcpy(expr->op, op.lexeme);
        expr->left = left;
        expr->right = right;
        left = (void *) expr;
    }
    return left;
}

void *equ(Parser *p) {
    void *left = comp(p);
    while (match(p, EQEQUALS) || match(p, BANGEQ)) {
        Token op = prev(p);
        void *right = comp(p);
        BinOpExpr *expr = malloc(sizeof(BinOpExpr));
        expr->s = BINOP;
        strcpy(expr->op, op.lexeme);
        expr->left = left;
        expr->right = right;
        left = (void *) expr;
    }
    return left;
}

void *comp(Parser *p) {
    void *left = adds(p);
    while (match(p, LTHAN) || match(p, LTHANEQ) || match(p, GTHAN) || match(p, GTHANEQ)) {
        Token op = prev(p);
        void *right = adds(p);
        BinOpExpr *expr = malloc(sizeof(BinOpExpr));
        expr->s = BINOP;
        strcpy(expr->op, op.lexeme);
        expr->left = left;
        expr->right = right;
        left = (void *) expr;
    }
    return left;
}

void *adds(Parser *p) {
    void *left = mul(p);
    while (match(p, PLUS) || match(p, MINUS)) {
        Token op = prev(p);
        void *right = mul(p);
        BinOpExpr *expr = malloc(sizeof(BinOpExpr));
        expr->s = BINOP;
        strcpy(expr->op, op.lexeme);
        expr->left = left;
        expr->right = right;
        left = (void *) expr;
    }
    return left;
}

void *mul(Parser *p) {
    void *left = unary(p);
    while (match(p, STAR) || match(p, SLASH)) {
        Token op = prev(p);
        void *right = unary(p);
        BinOpExpr *expr = malloc(sizeof(BinOpExpr));
        expr->s = BINOP;
        strcpy(expr->op, op.lexeme);
        expr->left = left;
        expr->right = right;
        left = (void *) expr;
    }
    return left;
}

void *unary(Parser *p) {
    if (match(p, BANG)) {
        Token op = prev(p);
        UnOpExpr *expr = malloc(sizeof(UnOpExpr));
        expr->s = UNOP;
        expr->right = unary(p);
        strcpy(expr->op, op.lexeme);
        return (void *) expr;
    }
    return primary(p);
}

void *primary(Parser *p) {
    if (match(p, ID)) {
        VarExpr *expr = malloc(sizeof(VarExpr));
        strcpy(expr->id, prev(p).lexeme);
        expr->s = VAR;
        return (void *) expr;
    } else if (match(p, NUMBER) || match(p, BOOLEAN)) {
        LiteralExpr *expr = malloc(sizeof(LiteralExpr));
        strcpy(expr->val, prev(p).lexeme);
        expr->s = LITERAL;
        return (void *) expr;
    } else if (match(p, LPAREN)) {
        void *expr = expression(p);
        if(!require(p, RPAREN, "Missing closing parenthesis on expression.")) return (void *) -1;
        BracketExpr *brackets = malloc(sizeof(BracketExpr));
        brackets->s = BRACKET;
        brackets->expr = expr;
        return (void *) brackets;
    } else {
        pError(p, "Expected expression.");
        return (void *) -1;
    }
}

// -----------------
// Helper funcs
// -----------------

// Add a statement to the 'parse tree'.
void add(ParseTree *tree, void *stmt) {
    if (tree->index != 0) {
        if (tree->size <= tree->index) {
            tree->size *= 2;
            tree->stmts = realloc(tree->stmts, sizeof(void*) * tree->size);
        }
        tree->stmts[tree->index++] = stmt;
    } else {
        tree->stmts = malloc(sizeof(void*) * tree->size);
        tree->stmts[tree->index++] = stmt;
    }
} 

// Get the next token.
Token pNext(Parser *p) {
    p->current = p->lookahead;
    p->lookahead = p->tokStream[++p->index];
    return p->current;
}

// Get the previous token.
Token prev(Parser *p) {
    return p->tokStream[p->index-2];
}

// Signal an error.
void pError(Parser *p, char *msg) {
    p->err = true;
    printf("Error (%d:%d): %s\n", p->current.line+1, p->current.col+1, msg);
}

// Failure to match results in error.
bool require(Parser *p, TokenType t, char *msg) {
    if (match(p, t)) return true;
    pError(p, msg);
    return false;
}

// Failure to match keyword results in error.
bool requireKeyword(Parser *p, char *key, char *msg) {
    if (matchKeyword(p, key)) return true;
    pError(p, msg);
    return false;
}

// Check if the token type matches the current token.
bool match(Parser *p, TokenType t) {
    if (p->current.type == t) {
        pNext(p);
        return true;
    }
    return false;
}

// Check if the keyword matches the current token.
bool matchKeyword(Parser *p, char *key) {
    if (strcmp(p->current.lexeme, key)) return false;
    pNext(p);
    return true;
}

// -----------------
// Output funcs
// -----------------

// Convert data type to string.
char *typeToString(Type t) {
    char *type = malloc(sizeof(char) * 20);  
    switch(t) {
        case NUM:
            strcpy(type, "NUM");
            break;
        case BOOL:
            strcpy(type, "BOOL");
            break;
        default:
            strcpy(type, "UNKNOWN");
            break;
    }
    return type;
}

// Display a statement in a somewhat human readable format.
void printStmt(void *stmt) {
     Stmt s = ((VarExpr *) stmt)->s;
     printf("(");
     switch (s) {
        case IF: {
            printf("IF {");
            printStmt(((IfStmt *) stmt)->cond);
            printf(" -> ");
            for (int i = 0; i < ((IfStmt *) stmt)->trueBranch.index; i++) {
                printStmt(((IfStmt *) stmt)->trueBranch.stmts[i]);
            }
            printf("}");
            break;
        }
        case WHILE: {
            printf("WHILE {");
            printStmt(((WhileStmt *) stmt)->cond);
            printf(" -> ");
            for (int i = 0; i < ((WhileStmt *) stmt)->trueBranch.index; i++) {
                printStmt(((WhileStmt *) stmt)->trueBranch.stmts[i]);
            }
            printf("}");
            break;
        }
        case SHOW: {
            printf("SHOW {");
            printStmt(((ShowStmt *) stmt)->expr);
            printf("}");
            break;
        }
        case VARDEC: {
            printf("VARDEC {%s %s}", ((VarDecStmt *) stmt)->id, typeToString(((VarDecStmt *) stmt)->type));
            break;
        }
        case VARASSIGN: {
            printf("VARASSIGN {");
            printf("%s <= ", ((VarAssignStmt *) stmt)->id);
            printStmt(((VarAssignStmt *) stmt)->expr);
            printf("}");
            break;
        }
        case BRACKET: {
            printf("BRACKETS {");
            printStmt(((BracketExpr *) stmt)->expr);
            printf("}");
            break;
        }
        case BINOP: {
            printf("BINOP {");
            printStmt(((BinOpExpr *) stmt)->left);
            printf(" %s ", ((BinOpExpr *) stmt)->op);
            printStmt(((BinOpExpr *) stmt)->right);
            printf("}");
            break;
        }
        case UNOP: {
            printf("UNOP {");
            printf("%s ", ((UnOpExpr *) stmt)->op);
            printStmt(((UnOpExpr *) stmt)->right);
            printf("}");
            break;
        }
        case LITERAL: {
            printf("LITERAL {%s}", ((LiteralExpr *) stmt)->val);
            break;
        }
        case VAR: {
            printf("VARIABLE {%s}", ((VarExpr *) stmt)->id);
        }
        default:
            break;
     }
     printf(")");
}

// Display all statements in the program.
void printTree(ParseTree t) {
    for (int i = 0; i < t.index; i++) {
        printStmt(t.stmts[i]);
        printf("\n");
    }
}

// -----------------
// Testing
// -----------------

// Test the parser on the test file.
void run() {
    FILE *f = fopen("/Users/cameron/OneDrive - University of Bristol/Imperative Programming/CAMlang/test.cam", "r");
    if (f == NULL) return; 
    Lexer *l = malloc(sizeof(Lexer));
    initLexer(l, f);
    tokenize(l);

    Parser *p = malloc(sizeof(Parser));
    initParser(p, l);
    parse(p);
    printTree(p->tree);
}
/*
// Don't run with any args.
int main(int argc, char *argk[]) {
    run();
    return 0;
}*/
