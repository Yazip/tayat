#pragma once

#include "scanner.h"
#include "defines.h"
#include "data_type.h"
#include "tree.h"
#include <string>
#include <vector>
#include <stack>

class Diagram {
private:
    Scanner* sc;

    // Буфер для токенов
    std::vector<int> push_tok;
    std::vector<std::string> push_lex;

    // Текущий токен
    int cur_tok;
    std::string cur_lex;

    DATA_TYPE current_decl_type; // Текущий тип при объявлении переменных, массивов и именованных констант
    int current_arr_elem_count; // Текущая размерность массива (для определения: массив или нет)

    // Стек для вычисления выражений
    std::stack<SemNode> eval_stack;

    int nextToken();
    int peekToken();
    void pushBack(int tok, const std::string& lex);

    // Базовые лексические/синтаксические/семантические ошибки
    void lexError();
    void synError(const std::string& msg);
    void semError(const std::string& msg);
    void interpError(const std::string& msg);

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
    DATA_TYPE Expr(); // Выражение (уровень равенств; поддержка унарного +/-)
    DATA_TYPE Rel(); // Уровень отношений (<, <=, >, >=)
    DATA_TYPE Add(); // Аддитивные (+, -)
    DATA_TYPE Mul(); // Мультипликативные (*, /, %)
    DATA_TYPE Prim(); // Первичное выражение: IDENT | Const | IDENT[Const] | (Expr)

    // Вспомогательные методы интерпретации
    void pushValue(const SemNode& node);
    SemNode popValue();
    SemNode evaluateConstant(const std::string& value, DATA_TYPE type);
    void executeAssignment(const std::string& varName, DATA_TYPE exprType, int line, int col);

public:
    Diagram(Scanner* scanner);

    // Точка входа: разбор всей программы
    void ParseProgram(bool isInterp = true, bool isDebug = false);
};