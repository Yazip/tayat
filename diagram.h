#pragma once

#include "scanner.h"
#include "defines.h"
#include <string>
#include <vector>

class Diagram {
private:
    Scanner* sc;

    // Буфер для токенов
    std::vector<int> push_tok;
    std::vector<std::string> push_lex;

    // Текущий токен
    int cur_tok;
    std::string cur_lex;

    int nextToken();
    int peekToken();
    void pushBack(int tok, const std::string& lex);
    void error(std::string msg);

    void Program(); // Верхнеуровневая программа (TopDecl*)
    void TopDecl(); // Одно верхнеуровневое объявление (MainFunc | TypeDefinition | VarDecl | ConstDecl)
    void MainFunc(); // Разбор функции main: int main () Block
    void TypeDefinition(); // Определение метки типа: typedef Type IDENT [[Const]] ;
    void VarDecl(); // Объявление переменных: Type IdInitList ;
    void ConstDecl(); // Объявление именованных констант: const Type: IdInitList ;
    void IdInitList(bool const_flag = false); // Список имен с инициализацией: IdInit (',' IdInit)*
    void IdInit(bool const_flag = false); // Один элемент списка: IDENT [= Expr]
    void Block(); // Cоставной оператор: '{' BlockItems '}'
    void BlockItems(); // Содержимое блока: (ConstDecl | VarDecl | Stmt)*
    void Stmt(); // Оператор: ';' | Block | Assign | While
    void WhileStmt(); // while (Expr) Stmt
    void Expr(); // Выражение (уровень равенств; поддержка унарного +/-)
    void Rel(); // Уровень отношений (<, <=, >, >=)
    void Add(); // Аддитивные (+, -)
    void Mul(); // Мультипликативные (*, /, %)
    void Prim(); // Первичное выражение: IDENT | Const | IDENT[Const] | (Expr)

public:
    Diagram(Scanner* scanner);

    // Точка входа: разбор всей программы
    void ParseProgram();
};