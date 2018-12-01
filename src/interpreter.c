// Interpreter for the CAM programming langauge.

#include "interpreter.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// -----------------
// Private Functions
// -----------------

void interpret(Interpreter *i);
Lit interpretStmt(Interpreter *i, void *stmt);
void assignSymbol(Interpreter *i, char *id, Lit v);
Symbol lookupSymbol(Interpreter *i, char *id);
Lit getValueFromString(char *v);
Lit binOpCases(Interpreter *i, char *op, Lit left, Lit right);
void initInterpreter(Interpreter *i, Parser *p);
SymCodes checkSymbol(SymbolTable *t, Symbol s);
void addSymbol(Interpreter *i, Symbol s);
void iError(Interpreter *i, char *msg, Token tok);

// -----------------
// Main Funcs
// -----------------

// Initialise the interpreter, symbol table and environment objects.
void initInterpreter(Interpreter *i, Parser *p) {
    i->err = p->err;
    i->tree = p->tree;
    SymbolTable s = {8, 0, NULL};
    s.syms = malloc(sizeof(Symbol)*s.size);
    i->env = (Environment) {0, s};
}

// Interpret statements in order.
void interpret(Interpreter *i) {
    for (int j = 0; j < i->tree.index; j++) {
        if (i->err) return;
        interpretStmt(i, i->tree.stmts[j]);
    }
}

// Interpret a single statement.
Lit interpretStmt(Interpreter *i, void *stmt) {
    if (i->err) return (Lit) {UNKNOWN, 0};
    Stmt s = ((VarExpr *) stmt)->s;
    switch (s) {
        case IF: {
            i->env.currentScope++;
            if (interpretStmt(i, ((IfStmt *) stmt)->cond).value && !i->err) {
                for (int j = 0; j < ((IfStmt *) stmt)->trueBranch.index; j++) {
                    interpretStmt(i, ((IfStmt *) stmt)->trueBranch.stmts[j]);
                }
            }
            i->env.currentScope--;
            break;
        }
        case WHILE: {
            i->env.currentScope++;
            while (interpretStmt(i, ((WhileStmt *) stmt)->cond).value && !i->err) {
                for (int j = 0; j < ((WhileStmt *) stmt)->trueBranch.index; j++) {
                    interpretStmt(i, ((WhileStmt *) stmt)->trueBranch.stmts[j]);
                }
            }
            i->env.currentScope--;
            break;
        }
        case SHOW: {
            Lit val = interpretStmt(i, ((ShowStmt *) stmt)->expr);
            if (i->err) return (Lit) {UNKNOWN, 0};
            //printf("%d\n", val.type);
            if (val.type == NUM) {
                printf("%f\n", val.value);
            } else {
                if (val.value) {
                    printf("true\n");
                } else {
                    printf("false\n");
                }
            }
            break;
        }
        case VARDEC: {
            Symbol sym = {i->env.currentScope, ((Token) {0, 0, END, ""}), "", ((VarDecStmt *) stmt)->type};
            strcpy(sym.tok.lexeme, ((VarDecStmt *) stmt)->id);
            //printf("%d\n", i->env.currentScope);
            addSymbol(i, sym);
            break;
        }
        case VARASSIGN: {
            Lit val = interpretStmt(i, ((VarAssignStmt *) stmt)->expr);
            assignSymbol(i, ((VarAssignStmt *) stmt)->id, val);
            break;
        }
        case BRACKET: {
            return interpretStmt(i, ((BracketExpr *) stmt)->expr);
        }
        case BINOP: {
            Lit left = interpretStmt(i, ((BinOpExpr *) stmt)->left);
            Lit right = interpretStmt(i, ((BinOpExpr *) stmt)->right);
            return binOpCases(i, ((BinOpExpr *) stmt)->op, left, right);
        }
        case UNOP: {
            Lit r = interpretStmt(i, ((UnOpExpr *) stmt)->right);
            if (r.type != BOOL) {
                Token tok = {0,0,END,""};
                strcpy(tok.lexeme, "!");
                iError(i, "'!' does not support non BOOL values.", tok);
            } else if (!strcmp(((UnOpExpr *) stmt)->op, "!")) {
                return (Lit) {BOOL, !(r.value)};    
            }
            break;
        }
        case LITERAL: {
            return getValueFromString(((LiteralExpr *) stmt)->val);
        }
        case VAR: {
            Symbol symbol = lookupSymbol(i, ((VarExpr *) stmt)->id);
            if (symbol.type == UNKNOWN) return (Lit) {UNKNOWN, 0};
            return getValueFromString(symbol.value);
        }
        default:
            break;
     }
     return (Lit) {UNKNOWN, 0};
}

// ------------------
// Symbol table Funcs
// ------------------

// Assign a value to an existing symbol.
void assignSymbol(Interpreter *i, char *id, Lit v) {
    for (int k = i->env.currentScope ; k >= 0; k--) {
        for (int j = 0; j<i->env.table.index; j++) {
            Symbol cs = i->env.table.syms[j];
            if (!strcmp(cs.tok.lexeme, id) && cs.scope == k) {
                if (cs.type != v.type) {
                    iError(i, "Type mismatch", cs.tok);
                    return;
                }
                char buf[30];
                if (cs.type != BOOL) {
                    strcpy(cs.value, gcvt(v.value, 11, buf));
                } else {
                    if (v.value) {
                        strcpy(cs.value, "true");
                    } else {
                        strcpy(cs.value, "false");
                    }
                }
                i->env.table.syms[j] = cs;
                return;
            }
        }
    }
    Token tok = (Token) {-1, -1, END, ""};
    strcpy(tok.lexeme, id);
    iError(i, "Variable not declared.", tok);
}

// Retrieve a symbol from the symbol table.
Symbol lookupSymbol(Interpreter *i, char *id) {
    for (int k = i->env.currentScope ; k >= 0; k--) {
        for (int j = 0; j<i->env.table.index; j++) {
            Symbol cs = i->env.table.syms[j];
            if (!strcmp(cs.tok.lexeme, id) && cs.scope == k) {
                return cs;
            }
        }
    }
    Token tok = (Token) {-1, -1, END, ""};
    strcpy(tok.lexeme, id);
    iError(i, "Variable not declared.", tok);
    return (Symbol) {0, ((Token) {0,0,END,""}), "", UNKNOWN};
}

// Check if a symbol exists in the symbol table.
SymCodes checkSymbol(SymbolTable *t, Symbol s) {
    for (int i = 0; i < t->index; i++) {
        if (!strcmp(t->syms[i].tok.lexeme, s.tok.lexeme) && s.scope == t->syms[i].scope) {
            if (t->syms[i].type != s.type) return TYPECHANGE;
            return IN;
        }
    }
    return NOTIN;
}

// Add a new symbol to the symbol table.
void addSymbol(Interpreter *i, Symbol s) {
    SymCodes code = checkSymbol(&i->env.table, s);
    switch (code) {
        case IN: break;
        case NOTIN: i->env.table.syms[i->env.table.index++] = s; break;
        case TYPECHANGE: iError(i, "Redeclaration of existing variable with different type.", s.tok); break;
    }
}

// -----------------
// Helper Funcs
// -----------------

// Error function
void iError(Interpreter *i, char *msg, Token tok) {
    i->err = true;
    printf("Error: %s - {%s}\n", msg, tok.lexeme);
}

// Convert a literal string value to a Lit.
Lit getValueFromString(char *v) {
    if (!strcmp(v,"true")) {
        return (Lit) {BOOL, 1};
    } else if (!strcmp(v,"false")) {
        return (Lit) {BOOL, 0};
    } else {
        return (Lit) {NUM, atof(v)};
    }
}

// Handle all possible binary operations.
Lit binOpCases(Interpreter *i, char *op, Lit left, Lit right) {
        Token tok = {0,0,END,""};
        strcpy(tok.lexeme, op);
        if (!strcmp("|", op)) {
            if (left.type != BOOL || right.type != BOOL) {
                iError(i, "'|' does not support non BOOL values.", tok);
            } 
            return (Lit) {BOOL, left.value || right.value};
        }
        if (!strcmp("&", op)) {
            if (left.type != BOOL || right.type != BOOL) {
                iError(i, "'&' does not support non BOOL values.", tok);
            } 
            return (Lit) {BOOL, left.value && right.value};
        }
        if (!strcmp("==", op)) {
            if (!((left.type == BOOL && right.type == BOOL) || (left.type == NUM && right.type == NUM))) {
                iError(i, "'==' cannot handle different types.", tok);
            } 
            return (Lit) {BOOL, left.value == right.value};
        }
        if (!strcmp("!=", op)){
            if (!((left.type == BOOL && right.type == BOOL) || (left.type == NUM && right.type == NUM))) {
                iError(i, "'!=' cannot handle different types.", tok);
            } 
            return (Lit) {BOOL, left.value != right.value};
        }
        if (!strcmp(">", op)) {
            if (left.type != NUM || right.type != NUM) {
                iError(i, "'>' does not support non NUM values.", tok);
            } 
            return (Lit) {BOOL, left.value > right.value};
        }
        if (!strcmp(">=", op)) {
            if (left.type != NUM || right.type != NUM) {
                iError(i, "'>=' does not support non NUM values.", tok);
            } 
            return (Lit) {BOOL, left.value >= right.value};
        }
        if (!strcmp("<", op)) {
            if (left.type != NUM || right.type != NUM) {
                iError(i, "'<' does not support non NUM values.", tok);
            } 
            return (Lit) {BOOL, left.value < right.value};
        }
        if (!strcmp("<=", op)) {
            if (left.type != NUM || right.type != NUM) {
                iError(i, "'<=' does not support non NUM values.", tok);
            } 
            return (Lit) {BOOL, left.value <= right.value};
        }
        if (!strcmp("+", op)) {
            if (left.type != NUM || right.type != NUM) {
                iError(i, "'+' does not support non NUM values.", tok);
            } 
            return (Lit) {NUM, left.value + right.value};
        }
        if (!strcmp("-", op)) {
            if (left.type != NUM || right.type != NUM) {
                iError(i, "'-' does not support non NUM values.", tok);
            } 
            return (Lit) {NUM, left.value - right.value};
        }
        if (!strcmp("*", op)) {
            if (left.type != NUM || right.type != NUM) {
                iError(i, "'*' does not support non NUM values.", tok);
            } 
            return (Lit) {NUM, left.value * right.value};
        }
        if (!strcmp("/", op)) {
            if (left.type != NUM || right.type != NUM) {
                iError(i, "'/' does not support non NUM values.", tok);
            } 
            return (Lit) {NUM, left.value / right.value};
        }
        return (Lit) {UNKNOWN, 0};
}

// -----------------
// Testing
// -----------------

// Test on test file.
void execute() {
    FILE *f = fopen("test.cam", "r");
    if (f == NULL) return; 

    Lexer *l = malloc(sizeof(Lexer));
    initLexer(l, f);
    tokenize(l);

    Parser *p = malloc(sizeof(Parser));
    initParser(p, l);
    parse(p);
    //printTree(p->tree);
    
    Interpreter *i = malloc(sizeof(Interpreter));
    initInterpreter(i, p);
    interpret(i);
}

// Run with no args.
int main(int argc, char *argk[]) {
    execute();
    return 0;
}
