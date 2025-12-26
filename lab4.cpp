#include <iostream>
#include <string>
#include <Windows.h>

#include "diagram.h"

int main(int argc, char** argv) {

    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    std::string fname = "input.txt";
    if (argc > 1) fname = argv[1];

    Scanner sc;
    if (!sc.loadFile(fname)) {
        std::cerr << "Невозможно открыть " << fname << std::endl;
        return -1;
    }

    Diagram dg(&sc);
    dg.ParseProgram(true);

    return 0;
}