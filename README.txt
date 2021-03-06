The CAM programming language interpreter.

I have created a simple tree walk interpreter for CAM a simple pseudocode-like language.
CAM is strongly typed with two data types (NUM, BOOL) as well as very basic control flow.
Single line comments are also supported and mimic the C style.

This project consists of three main files:
    lexer.c:
        This module tokenises the input source file producing a token stream upon successful execution.
        Errors given will also include what line and column the problem exists on.
    parser.c:
        This module creates a ParseTree object with all the statements that were parsed.
        It is an LL(1) parser that uses a simple grammar that I will provide below.
        Error messages are limited and can be improved.
    interpreter.c:
        This modules walks the ParseTree created by the parser and executes statements.
        Again error messages are limited.

Grammar for CAM:
    program ::= stmt*
    stmt ::= show | if | while | varDec | varAssign
    show ::= "show" expr ";"
    varDec ::= "let" ID "be" type ";"
    type ::= BOOLEAN | NUMBER
    varAssign ::= ID "=" expr ";"
    if ::= "if" expr "then" stmt* "endif"
    while :: "while: expr "do" stmt* "endwhile"
    expr ::= eq ("|" | "&" eq)*
    eq ::= comp ("==" | "!=" comp)*
    comp ::= adds (">" | ">=" | "<" | "<=" adds)*
    adds ::= mul ("+" | "-" mul)*
    mul ::= unary ("*" | "/" unary)*
    unary ::= "!" unary | primary
    primary ::= "(" expr ")" | ID | BOOLEAN | NUMBER

Also included is a test.cam file that is used for testing within the interpreter.
I have not had time to test everything fully however there should be pretty good error catching.

Improvements

If I had more time I would try to compile the code instead of interpreting it. 
Also adding a semantic analyser would be useful allowing me to simplify the interpreter a bit.