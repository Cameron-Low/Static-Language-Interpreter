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
double interpretStmt(Interpreter *i, void *stmt);
void assignSymbol(Interpreter *i, char *id, double v);
Symbol lookupSymbol(Interpreter *i, char *id);
double getValueFromString(char *v);
double binOpCases(char *op, double left, double right);
void initInterpreter(Interpreter *i, Parser *p);
bool checkSymbol(SymbolTable *t, Symbol s);
void addSymbol(SymbolTable *t, Symbol s);

// -----------------
// Main Funcs
// -----------------

// Initialise the interpreter, symbol table and environment objects.
void initInterpreter(Interpreter *i, Parser *p) {
    i->err = false;
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
double interpretStmt(Interpreter *i, void *stmt) {
     Stmt s = ((VarExpr *) stmt)->s;
     switch (s) {
        case IF: {
            i->env.currentScope++;
            if (interpretStmt(i, ((IfStmt *) stmt)->cond)) {
                for (int j = 0; j < ((IfStmt *) stmt)->trueBranch.index; j++) {
                    interpretStmt(i, ((IfStmt *) stmt)->trueBranch.stmts[j]);
                }
            }
            i->env.currentScope--;
            break;
        }
        case WHILE: {
            i->env.currentScope++;
            while (interpretStmt(i, ((WhileStmt *) stmt)->cond)) {
                for (int j = 0; j < ((WhileStmt *) stmt)->trueBranch.index; j++) {
                    interpretStmt(i, ((WhileStmt *) stmt)->trueBranch.stmts[j]);
                }
            }
            i->env.currentScope--;
            break;
        }
        case SHOW: {
            printf("%f\n", interpretStmt(i, ((ShowStmt *) stmt)->expr));
            break;
        }
        case VARDEC: {
            Symbol sym = {i->env.currentScope, "", "", ((VarDecStmt *) stmt)->type};
            strcpy(sym.id, ((VarDecStmt *) stmt)->id);
            addSymbol(&i->env.table, sym);
            break;
        }
        case VARASSIGN: {
            double val = interpretStmt(i, ((VarAssignStmt *) stmt)->expr);
            assignSymbol(i, ((VarAssignStmt *) stmt)->id, val);
            //printf("%f\n", val);
            break;
        }
        case BRACKET: {
            return interpretStmt(i, ((BracketExpr *) stmt)->expr);
        }
        case BINOP: {
            double left = interpretStmt(i, ((BinOpExpr *) stmt)->left);
            double right = interpretStmt(i, ((BinOpExpr *) stmt)->right);
            return binOpCases(((BinOpExpr *) stmt)->op, left, right);
        }
        case UNOP: {
            double r = interpretStmt(i, ((UnOpExpr *) stmt)->right);
            if (!strcmp(((UnOpExpr *) stmt)->op, "!")) {
                return !(r);    
            }
        }
        case LITERAL: {
            return getValueFromString(((LiteralExpr *) stmt)->val);
        }
        case VAR: {
            Symbol symbol = lookupSymbol(i, ((VarExpr *) stmt)->id);
            return getValueFromString(symbol.value);
        }
        default:
            break;
     }
     return -1;
}

// Assign a value to an existing symbol.
void assignSymbol(Interpreter *i, char *id, double v) {
    for (int k = i->env.currentScope ; k >= 0; k--) {
        for (int j = 0; j<i->env.table.index; j++) {
            Symbol cs = i->env.table.syms[j];
            if (!strcmp(cs.id, id) && cs.scope == k) {
                char buf[30];
                strcpy(cs.value, gcvt(v, 11, buf));
                i->env.table.syms[j] = cs;
                return;
            }
        }
    }
}

// Retrieve a symbol from the symbol table.
Symbol lookupSymbol(Interpreter *i, char *id) {
    for (int k = i->env.currentScope ; k >= 0; k--) {
        for (int j = 0; j<i->env.table.index; j++) {
            Symbol cs = i->env.table.syms[j];
            if (!strcmp(cs.id, id) && cs.scope == k) {
                return cs;
            }
        }
    }
    return (Symbol) {0, "", "", UNKNOWN};
}

// Check if a symbol exists in the symbol table.
bool checkSymbol(SymbolTable *t, Symbol s) {
    for (int i = 0; i < t->index; i++) {
        if (!strcmp(t->syms[i].id, s.id) && s.scope == t->syms[i].scope) {
            return true;
        }
    }
    return false;
}

// Add a new symbol to the symbol table.
void addSymbol(SymbolTable *t, Symbol s) {
    if (checkSymbol(t, s)) return;
    t->syms[t->index++] = s;
}

// Convert a literal string value to a double.
double getValueFromString(char *v) {
    if (!strcmp(v,"true")) {
        return 1;
    } else if (!strcmp(v,"false")) {
        return 0;
    } else {
        return atof(v);
    }
}

// Handle all possible binary operations.
double binOpCases(char *op, double left, double right) {
        if (!strcmp("|", op)) return left || right;
        if (!strcmp("&", op)) return left && right;
        if (!strcmp("==", op)) return left == right;
        if (!strcmp("!=", op)) return left != right;
        if (!strcmp(">", op)) return left > right;
        if (!strcmp(">=", op)) return left >= right;
        if (!strcmp("<", op)) return left < right;
        if (!strcmp("<=", op)) return left <= right;
        if (!strcmp("+", op)) return left + right;
        if (!strcmp("-", op)) return left - right;
        if (!strcmp("*", op)) return left * right;
        if (!strcmp("/", op)) return left / right;
        return 0;
}

// -----------------
// Testing
// -----------------

// Test on test file.
void execute() {
    FILE *f = fopen("/Users/cameron/OneDrive - University of Bristol/Imperative Programming/CAMlang/test.cam", "r");
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
