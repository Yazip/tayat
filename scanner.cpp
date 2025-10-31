#include "scanner.h"
#include "defines.h"
#include <fstream>
#include <sstream>
#include <iostream>

#define MAX_CONST_LEN 20 // Максимальная длина числовой константы и идентификатора

Scanner::Scanner() : text(), current_pos(0) {}

bool Scanner::loadFile(const std::string& file_name) {
    std::ifstream in(file_name);
    if (!in) return false;

    std::ostringstream ss;
    ss << in.rdbuf();
    text = ss.str();
    text.push_back('\0');
    current_pos = 0;
    return true;
}

char Scanner::peek(size_t offset) const {
    size_t pos = current_pos + offset;
    return (pos < text.size()) ? text[pos] : '\0';
}

char Scanner::getChar() {
    return (current_pos < text.size()) ? text[current_pos++] : '\0';
}

void Scanner::ungetChar() {
    if (current_pos > 0) --current_pos;
}

bool Scanner::isLetter(char c) {
    return ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z'));
}
bool Scanner::isDigit(char c) {
    return ((c >= '0') && (c <= '9'));
}
bool Scanner::isHexDigit(char c) {
    return isDigit(c) || ((c >= 'a') && (c <= 'f')) || ((c >= 'A') && (c <= 'F'));
}
bool Scanner::isIdentStart(char c) {
    return (isLetter(c) || (c == '_'));
}
bool Scanner::isIdentPart(char c) {
    return (isLetter(c) || isDigit(c) || (c == '_'));
}

int Scanner::checkKeyword(const std::string& s) {
    if (s == "const") return KW_CONST;
    if (s == "typedef") return KW_TYPEDEF;
    if (s == "short") return KW_SHORT;
    if (s == "long") return KW_LONG;
    if (s == "longlong") return KW_LONGLONG;
    if (s == "int") return KW_INT;
    if (s == "main") return KW_MAIN;
    if (s == "while") return KW_WHILE;
    return IDENT;
}

void Scanner::skipIgnored() {
    for (;;) {
        char c = peek();
        if ((c == ' ') || (c == '\t') || (c == '\n') || (c == '\r')) {
            getChar();
            continue;
        }
        if ((c == '/') && (peek(1) == '/')) {
            while ((peek() != '\n') && (peek() != '\0')) getChar();
            continue;
        }
        break;
    }
}

int Scanner::getNextLex(std::string& out_lex) {
    out_lex.clear();
    skipIgnored();

    char c = peek();
    if (c == '\0') return T_END;

    int token = T_ERR; // По умолчанию ошибочный символ

    // Идентификаторы / Ключевые слова
    if (isIdentStart(c)) {
        std::string lex;
        lex.push_back(getChar());
        while (isIdentPart(peek())) {
            lex.push_back(getChar());
        }  
        out_lex = lex;
        token = checkKeyword(lex);

        if ((token == IDENT) && (out_lex.length() > MAX_CONST_LEN)) {
            token = T_ERR;
        }
    }

    // Константы
    else if (isDigit(c)) {
        char first = getChar();
        if (first == '0') {
            char next = peek();
            if ((next == 'x') || (next == 'X')) {
                // Требуется >=1 16-ричных цифр
                getChar();
                std::string lex = "0";
                lex.push_back(next);
                if (!isHexDigit(peek())) {
                    // Ошибка: 0x без цифр
                    out_lex = lex;
                    token = T_ERR;
                }
                else {
                    while (isHexDigit(peek())) {
                        lex.push_back(getChar());
                    }
                    out_lex = lex;

                    if (lex.length() > MAX_CONST_LEN) {
                        token = T_ERR;
                    }
                    else {
                        token = CONST_HEX;
                    }
                }
            }
            else if (isDigit(next)) {
                // Цифра после 0 - читаем как десятичную константу
                std::string lex = "0";
                while (isDigit(peek())) {
                    lex.push_back(getChar());
                }
                out_lex = lex;

                if (lex.length() > MAX_CONST_LEN) {
                    token = T_ERR;
                }
                else {
                    token = CONST_DEC;
                }
            }
            else {
                out_lex = "0";
                token = CONST_DEC;
            }
        }
        else {
            std::string lex;
            lex.push_back(first);
            while (isDigit(peek())) {
                lex.push_back(getChar());
            }
            out_lex = lex;

            if (lex.length() > MAX_CONST_LEN) {
                token = T_ERR;
            }
            else {
                token = CONST_DEC;
            }
        }
    }

    // Операторы / специальные знаки / разделители
    else {
        switch (c) {
        case '+':
            getChar();
            out_lex = "+";
            token = PLUS;
            break;

        case '-':
            getChar();
            out_lex = "-";
            token = MINUS;
            break;

        case '*':
            getChar();
            out_lex = "*";
            token = MULT;
            break;

        case '/':
            getChar();
            out_lex = "/";
            token = DIV;
            break;

        case '%':
            getChar();
            out_lex = "%";
            token = MOD;
            break;

        case '=':
            getChar();
            if (peek() == '=') {
                getChar();
                out_lex = "==";
                token = EQ;
                break;
            }
            out_lex = "=";
            token = ASSIGN;
            break;

        case '!':
            getChar();
            if (peek() == '=') {
                getChar();
                out_lex = "!=";
                token = NEQ;
                break;
            }
            out_lex = "!";
            token = T_ERR;
            break;

        case '<':
            getChar();
            if (peek() == '=') {
                getChar();
                out_lex = "<=";
                token = LE;
                break;
            }
            out_lex = "<";
            token = LT;
            break;

        case '>':
            getChar();
            if (peek() == '=') {
                getChar();
                out_lex = ">=";
                token = GE;
                break;
            }
            out_lex = ">";
            token = GT;
            break;

        case ';':
            getChar();
            out_lex = ";";
            token = SEMI;
            break;
        
        case ',':
            getChar();
            out_lex = ",";
            token = COMMA;
            break;
        
        case '(':
            getChar();
            out_lex = "(";
            token = LPAREN;
            break;
        
        case ')':
            getChar();
            out_lex = ")";
            token = RPAREN;
            break;
        
        case '{':
            getChar();
            out_lex = "{";
            token = LBRACE;
            break;
        
        case '}':
            getChar();
            out_lex = "}";
            token = RBRACE;
            break;
        
        case '[':
            getChar();
            out_lex = "[";
            token = LBRACKET;
            break;
        
        case ']':
            getChar();
            out_lex = "]";
            token = RBRACKET;
            break;

        default:
            out_lex = std::string(1, getChar());
            token = T_ERR;
            break;
        }
    }

    // Вывод информации в консоль
    if (token != T_END) {
        std::cout << "Код: " << token << ", Лексема: '" << out_lex << "'" << std::endl;
    }

    return token;
}

// Подсчёт строки и столбца
std::pair<int, int> Scanner::getLineCol() const {
    int line = 1, col = 0;
    for (size_t i = 0; (i < current_pos) && (i < text.size()); i++) {
        if (text[i] == '\n') {
            ++line;
            col = 0;
        }
        else {
            ++col;
        }
    }
    return { line, col };
}