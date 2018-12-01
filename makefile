run:
	clang -std=c11 -Wall -pedantic -g src/parser.c src/lexer.c src/interpreter.c -o cam -fsanitize=undefined -fsanitize=address

