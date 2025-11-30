#pragma once
#include "sem_node.h"
#include <fstream>

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

    // Установить флаг именованной константы
    void SemSetConst(Tree* Addr, int value);

    // Установить базовый тип (для массивов и меток типов)
    void SemSetBasicType(Tree* Addr, DATA_TYPE bt);

    // Установить размерность массива
    void SemSetArrElemCount(Tree* Addr, int aec);

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

private:
    // Печать дерева
    void Print(int depth);
    std::string makeLabel(const Tree* tree) const;
};