#include "defines.h"
#include "scanner.h"

#include <iostream>
#include <Windows.h>

int main(int argc, char** argv) {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    std::string fname = "input.txt";
    if (argc > 1) fname = argv[1];

    Scanner sc;
    if (!sc.loadFile(fname)) {
        std::cerr << "Невозможно открыть " << fname << std::endl;
        return 2;
    }

    while (true) {
        std::string lex;
        int code = sc.getNextLex(lex);
        if (code == T_END) {
            std::cout << "T_END=" << code << "\n";
            break;
        }
        if (code == T_ERR) {
            std::cerr << "Лексическая ошибка: \"" << lex << "\"\n";
            break;
        }
    }
    return 0;
}