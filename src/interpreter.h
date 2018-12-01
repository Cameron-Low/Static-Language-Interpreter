#include "parser.h"
#include <stdbool.h>

// -----------------
// Public Objects
// -----------------

typedef struct Symbol {
    int scope;
    Token tok;
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

typedef struct Lit {
    Type type;
    double value;
} Lit;

typedef enum SymbolTableCodes {
    TYPECHANGE, IN, NOTIN
} SymCodes;
