#pragma once
#include "sem_node.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>

class Tree {
public:
    SemNode* n;    // Данные узла
    Tree* Up;      // Родитель (внешняя область)
    Tree* Left;    // Следующий элемент на том же уровне (левый сосед)
    Tree* Right;   // Первый вложенный элемент (правая ссылка)

    // Текущая позиция (корень/текущий блок) -- статическая
    static Tree* Root;
    static Tree* Cur;

    // Конструкторы / деструктор
    Tree(SemNode* node = nullptr, Tree* up = nullptr);
    ~Tree();

    // Управление деревом: вставка левого/правого дочернего (создают новый узел)
    void SetLeft(SemNode* Data);   // Вставить как левого соседа текущего узла
    void SetRight(SemNode* Data);  // Вставить как первый дочерний элемент текущего узла

    // Поиск: блочная видимость
    Tree* FindUp(Tree* From, const std::string& id);        // Поиск в текущей и внешних областях
    Tree* FindUpOneLevel(Tree* From, const std::string& id);// Поиск только в текущем уровне (среди детей From)

    // Семантические операции
    // Занесение идентификатора a в текущую область
    Tree* SemInclude(const std::string& a, DATA_TYPE t, int line, int col);

    // Занесение константы со значением
    Tree* SemIncludeConstant(const std::string& a, DATA_TYPE t, const std::string& value, int line, int col);

    // Установить флаг именованной константы
    void SemSetConst(Tree* Addr, int value);

    // Установить базовый тип (для массивов и меток типов)
    void SemSetBasicType(Tree* Addr, DATA_TYPE bt);

    // Установить размерность массива
    void SemSetArrElemCount(Tree* Addr, int aec);

    // Установить индекс элемента массива
    void SemSetIndex(Tree* Addr, int index);

    // Найти переменную / именованную константу (не метку типа) с именем a в видимых областях
    Tree* SemGetVar(const std::string& a, int line, int col);

    // Найти метку типа с именем a
    Tree* SemGetType(const std::string& a, int line, int col);

    // Проверка дубля на текущем уровне
    bool DupControl(Tree* Addr, const std::string& a);

    // Вход/выход в/из области (составной оператор)
    // SemEnterBlock создаёт анонимный узел области под Cur и переключает Cur на него
    Tree* SemEnterBlock(int line, int col);
    void SemExitBlock();

    // Установка/получение текущей вершины
    static void SetCur(Tree* a) { Cur = a; }
    static Tree* GetCur() { return Cur; }

    void Print(); // Печать дерева с нулевого отступа

    // Печать ошибки и остановка
    static void SemError(const std::string& msg, const std::string& id = "", int line = -1, int col = -1);

    static void InterpError(const std::string& msg, const std::string& id = "", int line = -1, int col = -1);

    // Установка значения переменной
    static void SetVarValue(const std::string& name, const SemNode& value, int line, int col);

    // Получение значения переменной
    static SemNode GetVarValue(const std::string& name, int line, int col);

    // Выполнение арифметических операций
    static SemNode ExecuteArithmeticOp(const SemNode& left, const SemNode& right, const std::string& op, int line, int col);

    // Выполнение операций сравнения
    static SemNode ExecuteComparisonOp(const SemNode& left, const SemNode& right, const std::string& op, int line, int col);

    // Приведение типов для операций
    static DATA_TYPE GetMaxType(DATA_TYPE t1, DATA_TYPE t2);

    static SemNode CastToType(const SemNode& value, DATA_TYPE targetType, int line, int col, bool showWarning = false);

    // Проверка возможности приведения
    static bool CanImplicitCast(DATA_TYPE from, DATA_TYPE to);

    // Методы для управления интерпретацией
    static void EnableInterpretation();
    static void DisableInterpretation();
    static bool IsInterpretationEnabled();

    static void EnableDebug();
    static void DisableDebug();
    static bool IsDebugEnabled();

    // Методы для вывода
    static void PrintDebugInfo(const std::string& message, int line = 0, int col = 0);
    static void PrintAssignment(const std::string& varName, const SemNode& value, int line, int col);
    static void PrintArithmeticOp(const std::string& op, const SemNode& left, const SemNode& right, const SemNode& result, int line, int col);
    static void PrintTypeConversionWarning(DATA_TYPE from, DATA_TYPE to, const std::string& context, const std::string& expression, int line, int col);

    // Текущая область для контекста
    static Tree* currentArea;

    // Методы для управления текущей областью
    static void SetCurrentArea(Tree* area) { currentArea = area; }
    static Tree* GetCurrentArea() { return currentArea; }

private:
    // Печать дерева
    void Print(int depth);
    std::string makeLabel(const Tree* tree) const;

    // Флаг интерпретации
    static bool interpretationEnabled;
    static bool debug; // Флаг для подробного вывода
};