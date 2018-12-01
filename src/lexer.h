#include <stdbool.h>
#include <stdio.h>

// -----------------
// Public Objects
// -----------------

// Define all token types in the CAM language.
typedef enum TokenType {
    END, ID, KEYWORD, NUMBER, BOOLEAN,
    SEMICOLON, LPAREN, RPAREN, EQUALS,
    EQEQUALS, BANG, BANGEQ, LTHAN, GTHAN,
    GTHANEQ, LTHANEQ, STAR, PLUS, MINUS,
    SLASH, AND, OR, TYPES
} TokenType;

// Basic token structure.
typedef struct Token {
    int line;
    int col;
    TokenType type;
    char lexeme[100];
} Token;

// Lexer structure.
typedef struct Lexer {
    FILE *f;
    char current;
    char lookahead;
    bool err;
    int line;
    int col;
    int tokLength;
    Token *tokens;   
} Lexer;

// -----------------
// Public Functions
// -----------------

void tokenize(Lexer *l);
void initLexer(Lexer *l, FILE *f);
