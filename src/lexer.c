// Lexer for the CAM programming langauge.

#include "lexer.h"
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// -----------------
// Private Functions
// -----------------

void complexToken(Lexer *l);
void numberToken(Lexer *l);
void stringToken(Lexer *l);
void error(Lexer *l);
void addToken(Lexer *l, TokenType t, char *lexeme);
void printToken(Token t);
void printTokenStream(Token in[]);
char next(Lexer *l);
char peek(Lexer *l);
bool inArray(char *v, char *arr[], int len);

// ------------
// Main Funcs
// ------------

// Initialise a new lexer object.
// Default values can be found here.
void initLexer(Lexer *l, FILE *f) {
    l->f = f;
    l->err = false;
    l->line = 0;
    l->col = 0;
    l->tokens = malloc(sizeof(Token) * 100);
    l->tokLength = 0;
    l->lookahead = getc(f);
}

// Converts an input file into a token stream ready for parsing.
// One lookahead character is used.
void tokenize(Lexer *l) {
    while (next(l) != EOF && !(l->err)) {
        switch (l->current) {
            case '\n': 
                l->line++;
                l->col = -1;
                break;
            case ' ':
                break;
            case '\t':
                l->col += 4;
                break;
            case '(':
                addToken(l, LPAREN, "(");
                break;
            case ')':
                addToken(l, RPAREN, ")");
                break;
            case ';':
                addToken(l, SEMICOLON, ";");
                break;
            case '*':
                addToken(l, STAR, "*");
                break;
            case '+':
                addToken(l, PLUS, "+");
                break;
            case '-':
                addToken(l, MINUS, "-");
                break;
            case '/':
                if (peek(l) == '/') {
                    while (peek(l) != '\n') {
                        next(l);
                    }
                    next(l);
                    l->line++;
                    l->col = 0;
                } else {
                    addToken(l, SLASH, "/");
                }
                break;
            case '&':
                addToken(l, AND, "&");
                break;
            case '|':
                addToken(l, OR, "|");
                break;
            case '=':
                if (peek(l) == '=') {
                    next(l);
                    addToken(l, EQEQUALS, "==");
                } else {
                    addToken(l, EQUALS, "=");
                }
                break;
            case '!':
                if (peek(l) == '=') {
                    next(l);
                    addToken(l, BANGEQ, "!=");
                } else {
                    addToken(l, BANG, "!");
                }
                break;
            case '>':
                if (peek(l) == '=') {
                    next(l);
                    addToken(l, GTHANEQ, ">=");
                } else {
                    addToken(l, GTHAN, ">");
                }
                break;
            case '<':
                if (peek(l) == '=') {
                    next(l);
                    addToken(l, LTHANEQ, "<=");
                } else {
                    addToken(l, LTHAN, "<");
                }
                break;
            default:
                complexToken(l);
        }
        l->col++;
    }
    if (l->err) {
        l->tokLength = 0;
    }

    // Add the end token.
    addToken(l, END, "EOF");
}

// Handle a character that is part of a complex lexeme.
void complexToken(Lexer *l) {
    if (isdigit(l->current)) {
        numberToken(l);
    } else if (isalpha(l->current)) {
        stringToken(l);
    } else {
        error(l);
    }
}

// Handle a number. This includes floating points.
void numberToken(Lexer *l) {
    char num[100];
    int index = 0;
    bool hitPoint = false;
    num[index++] = l->current;
    char c;
    while (isdigit(peek(l)) || (!hitPoint && peek(l) == '.')) {
        c = next(l);
        if (c == '.') hitPoint = true;
        num[index++] = c;
    }
    num[index++] = '\0';
    addToken(l, NUMBER, num);
    l->col += strlen(num)-1;
}

// Handle a string. Test for which type of token it is and act accordingly.
void stringToken(Lexer *l) {
    char str[100];
    int index = 0;
    str[index++] = l->current;
    while (isalpha(peek(l))) {
        next(l);
        str[index++] = l->current;
    }
    str[index++] = '\0';
    
    char *keywords[] = {"if", "let", "while", "be", "then", "endif", "endwhile", "do", "show"}; // Len 9
    char *bools[] = {"true", "false"}; // Len 2
    char *types[] = {"num", "bool"}; // Len 2

    if (inArray(str, keywords, 9)) {
        addToken(l, KEYWORD, str);
    } else if (inArray(str, bools, 2)) {
        addToken(l, BOOLEAN, str);
    }  else if (inArray(str, types, 2)) {
        addToken(l, TYPES, str);
    } else {
        addToken(l, ID, str);
    }
    l->col += strlen(str)-1;
}

// Print useful error message with line,col numbers and error.
void error(Lexer *l) {
    l->err = true;
    printf("Error (%d:%d): Unidentified character '%c'.\n", l->line+1, l->col+1, l->current);
}

// ------------
// Helpers
// ------------

// Create and add a new token to the token stream.
void addToken(Lexer *l, TokenType t, char *lexeme) {
    Token tok = {l->line, l->col, t, ""};
    strcpy(tok.lexeme, lexeme);
    l->tokens[l->tokLength++] = tok; 
}

// Display a token with its type and lexeme.
void printToken(Token t) {
    printf("{%s - %d}\n", t.lexeme, t.type);
}

// Display all tokens in a token stream.
void printTokenStream(Token in[]) {
    int index = 0;
    while (in[index].type != END) {
        printToken(in[index++]);
    }
    printToken(in[index]);
}

// Get the next character in the file.
char next(Lexer *l) {
    l->current = l->lookahead;
    l->lookahead = getc(l->f);
    return l->current;
}

// Return the lookahead character.
char peek(Lexer *l) {
    return l->lookahead;
}

// Check if a string is in a string array.
bool inArray(char *v, char *arr[], int len) {
    for (int i = 0; i < len; i++) {
        if (!strcmp(v, arr[i])) return true;
    }
    return false;
}

// ------------
// Testing
// ------------

// Test the lexer using the default file and some other simple tests.
void test() {
    FILE *f = fopen("/Users/cameron/OneDrive - University of Bristol/Imperative Programming/CAMlang/test.cam", "r");
    if (f == NULL) return; 
    Lexer *l = malloc(sizeof(Lexer));
    initLexer(l, f);
    tokenize(l);
    printTokenStream(l->tokens);
}
/*
// Run with 'cam' and no args for tests.
int main(int argc, char *argk[]) {
    if (argc == 1) {
        printf("Runnning tests...\n");
        test();
        return 0;
    } else {
        printf("Usage: cam. This runs tests.\n");
        return 0;
    }
}*/
