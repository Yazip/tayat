#pragma once
#include <string>

class Scanner {
private:
    std::string text;
    size_t current_pos;

    char peek(size_t offset = 0) const;
    char getChar();
    void ungetChar();

    static bool isLetter(char c);
    static bool isDigit(char c);
    static bool isHexDigit(char c);
    static bool isIdentStart(char c);
    static bool isIdentPart(char c);

    void skipIgnored();
    int checkKeyword(const std::string& s);

public:
    Scanner();

    bool loadFile(const std::string& file_name);
    int getNextLex(std::string& out_lex);
    std::pair<int, int> getLineCol() const;
};