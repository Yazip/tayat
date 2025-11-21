#include "diagram.h"

#include <iostream>

Diagram::Diagram(Scanner* scanner) : sc(scanner), cur_tok(0), cur_lex() {
    push_tok.clear();
    push_lex.clear();
}

int Diagram::nextToken() {
    if (!push_tok.empty()) {
        cur_tok = push_tok.back();
        push_tok.pop_back();
        cur_lex = push_lex.back();
        push_lex.pop_back();
        return cur_tok;
    }
    cur_tok = sc->getNextLex(cur_lex);
    return cur_tok;
}

int Diagram::peekToken() {
    int t = nextToken();
    pushBack(t, cur_lex);
    return t;
}

void Diagram::pushBack(int tok, const std::string& lex) {
    push_tok.push_back(tok);
    push_lex.push_back(lex);
}

void Diagram::error(std::string msg) {
    std::pair<int, int> lc = sc->getLineCol();

    std::cerr << "Синтаксическая ошибка: " << msg;
    if (!cur_lex.empty()) {
        std::cerr << " (около '" << cur_lex << "')";
    }
    std::cerr << std::endl << "(строка " << lc.first << ":" << lc.second << ")" << std::endl;
    std::exit(1);
}

// Точка входа
void Diagram::ParseProgram() {
    Program();
}

// Program -> TopDecl*
void Diagram::Program() {
    int t = peekToken();
    while (t == KW_INT || t == KW_SHORT || t == KW_LONG || t == KW_LONGLONG || t == IDENT || t == KW_TYPEDEF || t == KW_CONST) {
        TopDecl();
        t = peekToken();
    }

    if (t != T_END) {
        error("Ожидалось объявление переменной, именованной константы, метки типа или определение функции main");
    }
}

// TopDecl -> MainFunc | TypeDefinition | VarDecl | ConstDecl
void Diagram::TopDecl() {
    int t = peekToken();
    if (t == KW_INT) {
        nextToken();
        t = peekToken();
        if (t == KW_MAIN) {
            nextToken();
            MainFunc();
        }
        else {
            VarDecl();
        }
    }
    else if (t == KW_TYPEDEF) {
        nextToken();
        TypeDefinition();
    }
    else if (t == KW_CONST) {
        nextToken();
        ConstDecl();
    }
    else {
        nextToken();
        VarDecl();
    }
}

// MainFunc -> 'int' main '('')' Block
void Diagram::MainFunc() {
    int t = peekToken();
    if (t != LPAREN) {
        error("Ожидалась '(' после main");
    }

    nextToken();
    t = peekToken();
    if (t != RPAREN) {
        error("Ожидалась ')' после '('");
    }
    nextToken();
    Block();
}

// VarDecl -> Type IdInitList ;
void Diagram::VarDecl() {
    IdInitList();
    int t = peekToken();
    if (t != SEMI) {
        error("Ожидалась ';' в конце объявления переменных");
    }
    nextToken();
}

// ConstDecl -> const Type IdInitList ;
void Diagram::ConstDecl() {
    int t = peekToken();
    if (t != KW_INT && t != KW_SHORT && t != KW_LONG && t != KW_LONGLONG && t != IDENT) {
        error("Ожидался тип данных после const");
    }

    nextToken();
    IdInitList(true);
    t = peekToken();
    if (t != SEMI) {
        error("Ожидалась ';' в конце объявления именованных констант");
    }
    nextToken();
}

// TypeDefinition -> typedef Type IDENT [[Const]] ;
void Diagram::TypeDefinition() {
    int t = peekToken();
    if (t != KW_INT && t != KW_SHORT && t != KW_LONG && t != KW_LONGLONG && t != IDENT) {
        error("Ожидался тип данных после typedef");
    }
    nextToken();
    t = peekToken();
    if (t != IDENT) {
        error("Ожидался идентификатор в определении метки типа");
    }
    nextToken();
    t = peekToken();

    if (t == LBRACKET) {
        nextToken();
        t = peekToken();
        if (t != CONST_DEC && t != CONST_HEX) {
            error("Ожидалась константа после '['");
        }
        nextToken();
        t = peekToken();
        if (t != RBRACKET) {
            error("Ожидалась ']' после константы");
        }
        nextToken();
        t = peekToken();
    }

    if (t != SEMI) {
        error("Ожидалась ';' после определения метки");
    }
    nextToken();
}

// IdInitList -> IdInit (',' IdInit)*
void Diagram::IdInitList(bool const_flag) {
    IdInit(const_flag);
    int t = peekToken();
    while (t == COMMA) {
        nextToken(); // ','
        IdInit(const_flag);
        t = peekToken();
    }
}

// IdInit -> IDENT [ = Expr ]
void Diagram::IdInit(bool const_flag) {
    int t = peekToken();
    if (t != IDENT) {
        error("Ожидался идентификатор в списке объявлений");
    }
    nextToken();
    t = peekToken();
    if (t == ASSIGN) {
        nextToken();
        Expr();
    }
    else {
        if (const_flag) {
            error("Ожидалось '=' в определении именованной константы");
        }
    }
}

// Block -> '{' BlockItems '}'
void Diagram::Block() {
    int t = peekToken();
    if (t != LBRACE) {
        error("Ожидалась '{' для начала блока");
    }
    t = nextToken();
    BlockItems();
    t = peekToken();
    if (t != RBRACE) {
        error("Ожидалась '}' для конца блока");
    }
    nextToken();
}

// BlockItems -> ( ConstDecl | VarDecl | Stmt )* 
void Diagram::BlockItems() {
    int t = peekToken();
    while (t != RBRACE && t != T_END) {
        if (t == KW_INT || t == KW_SHORT || t == KW_LONG || t == KW_LONGLONG || t == IDENT) {
            int t2 = nextToken();
            t = peekToken();
            if (t == ASSIGN || t == LBRACKET) {
                pushBack(t2, cur_lex);
                Stmt();
            }
            else {
                VarDecl();
            }
        }
        else if (t == KW_CONST) {
            nextToken();
            ConstDecl();
        }
        else {
            Stmt();
        }
        t = peekToken();
    }
}

// Stmt -> ';' | Block | Assign | While
void Diagram::Stmt() {
    int t = peekToken();
    if (t == SEMI) { 
        nextToken();
        return;
    }
    if (t == LBRACE) {
        Block();
        return;
    }
    if (t == KW_WHILE) {
        nextToken();
        WhileStmt();
        return;
    }

    if (t == IDENT) {
        nextToken();
        t = peekToken();

        if (t == LBRACKET) {
            nextToken();
            t = peekToken();
            if (t != CONST_DEC && t != CONST_HEX) {
                error("Ожидалась константа после '['");
            }
            nextToken();
            t = peekToken();
            if (t != RBRACKET) {
                error("Ожидалась ']' после константы");
            }
            nextToken();
            t = peekToken();
        }

        if (t == ASSIGN) {
            nextToken();
            Expr();
            t = peekToken();
            if (t != SEMI) {
                error("Ожидалась ';' после оператора присваивания");
            }
            nextToken();
            return;
        }
        else {
            error("Ожидалось '=' после идентификатора (присваивание)");
        }
    }

    error("Неизвестная форма оператора");
}

// WhileStmt -> while ( Expr ) Stmt
void Diagram::WhileStmt() {
    int t = peekToken();
    if (t != LPAREN) {
        error("Ожидалась '(' после while");
    }
    nextToken();
    Expr();
    t = peekToken();
    if (t != RPAREN) {
        error("Ожидалась ')' после выражения");
    }
    nextToken();
    Stmt();
}

// Expr -> ['+'|'-'] Rel ( ('==' | '!=') Rel )*
void Diagram::Expr() {
    int t = peekToken();
    if (t == PLUS || t == MINUS) {
        nextToken();
    }
    Rel();
    t = peekToken();
    while (t == EQ || t == NEQ) {
        nextToken();
        Rel();
        t = peekToken();
    }
}

// Rel -> Add ( ('<' | '<=' | '>' | '>=') Add )*
void Diagram::Rel() {
    Add();
    int t = peekToken();
    while (t == LT || t == LE || t == GT || t == GE) {
        nextToken();
        Add();
        t = peekToken();
    }
}

// Add -> Mul ( ('+' | '-') Mul )*
void Diagram::Add() {
    Mul();
    int t = peekToken();
    while (t == PLUS || t == MINUS) {
        nextToken();
        Mul();
        t = peekToken();
    }
}

// Mul -> Prim ( ('*' | '/' | '%') Prim )*
void Diagram::Mul() {
    Prim();
    int t = peekToken();
    while (t == MULT || t == DIV || t == MOD) {
        nextToken();
        Prim();
        t = peekToken();
    }
}

// Prim -> IDENT | Const | IDENT '[' Const ']' | '(' Expr ')'
void Diagram::Prim() {
    int t = peekToken();
    if (t == CONST_DEC || t == CONST_HEX) {
        nextToken();
        return;
    }
    if (t == LPAREN) {
        nextToken();
        Expr();
        t = peekToken();
        if (t != RPAREN) {
            error("Ожидалась ')' после выражения");
        }
        nextToken();
        return;
    }
    if (t == IDENT) {
        nextToken();
        t = peekToken();
        if (t == LBRACKET) {
            nextToken();
            t = peekToken();
            if (t == CONST_DEC || t == CONST_HEX) {
                nextToken();
                t = peekToken();
                if (t != RBRACKET) {
                    error("Ожидалась ']' после константы");
                }
                nextToken();
                return;
            }
            else {
                error("Ожидалась константа после '['");
            }
        }
        else {
            return;
        }
    }
    error("Ожидалось первичное выражение (IDENT, константа или скобки)");
}