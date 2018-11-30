#include "parser.h"
#include <stdbool.h>

// -----------------
// Public Objects
// -----------------

typedef struct Symbol {
    int scope;
    char id[100];
    char value[100];
    Type type;
} Symbol;

typedef struct SymbolTable {
    int size;
    int index;
    Symbol *syms;
} SymbolTable;

typedef struct Environment {
    int currentScope;
    SymbolTable table;
} Environment;


typedef struct Interpreter {
    Environment env;
    ParseTree tree;
    bool err;
} Interpreter;
