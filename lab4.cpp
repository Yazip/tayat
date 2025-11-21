#include "diagram.h"

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

    Diagram dg(&sc);
    dg.ParseProgram();

    return 0;
}