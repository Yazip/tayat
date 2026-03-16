#include "pch.h"
#include "CppUnitTest.h"

#include "../SimpleCppCompiler/scanner.cpp" // Реализация лексера
#include "../SimpleCppCompiler/tree.cpp" // Реализация семантического дерева
#include "../SimpleCppCompiler/data_type.h" // Типы данных
#include "../SimpleCppCompiler/defines.h" // Коды лексем

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTests
{
    // Метод для сброса глобального состояния семантического дерева:
    // сброс указателей и флагов, чтобы тесты не влияли друг на друга
    void ResetTree()
    {
        Tree::Reset();

        // После сброса создаём корневую область видимости
        SemNode* root_node = new SemNode();
        root_node->id = "root";
        root_node->DataType = TYPE_SCOPE;
        Tree::Root = new Tree(root_node, nullptr);
        Tree::Cur = Tree::Root;
    }

    // Тестовый класс для тестов лексера
    TEST_CLASS(ScannerTests)
    {
    public:
        // Тестовый метод для проверки распознавания ключевого слова short
        TEST_METHOD(TestKeywordShort)
        {
            Scanner sc;
            sc.loadFromString("short"); // Метод, который тестируем; передаём ему строку
            std::string lex;
            int tok = sc.getNextLex(lex);

            Assert::AreEqual(KW_SHORT, tok); // Сравниваем ожидаемый и фактический коды ключевого слова
        }

        // Тестовый метод для проверки распознавания идентификатора (имя переменной / именованной константы / метки типа)
        TEST_METHOD(TestIdentifier)
        {
            Scanner sc;
            sc.loadFromString("_mYvAR52");
            std::string lex;
            int tok = sc.getNextLex(lex);

            Assert::AreEqual(IDENT, tok);
        }

        // Тестовый метод для проверки распознавания константы в 10 с/с
        TEST_METHOD(TestDecConstant)
        {
            Scanner sc;
            sc.loadFromString("9876543210");
            std::string lex;
            int tok = sc.getNextLex(lex);

            Assert::AreEqual(CONST_DEC, tok);
        }

        // Тестовый метод для проверки распознавания константы в 16 с/с
        TEST_METHOD(TestHexConstant)
        {
            Scanner sc;
            sc.loadFromString("0xAB6");
            std::string lex;
            int tok = sc.getNextLex(lex);

            Assert::AreEqual(CONST_HEX, tok);
        }

        // Тестовый метод для проверки распознавания операторов
        TEST_METHOD(TestOperators)
        {
            Scanner sc;
            sc.loadFromString("+ - * / % == != < <= > >= =");
            std::string lex;
            int tok;

            tok = sc.getNextLex(lex); Assert::AreEqual(PLUS, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(MINUS, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(MULT, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(DIV, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(MOD, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(EQ, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(NEQ, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(LT, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(LE, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(GT, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(GE, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(ASSIGN, tok);
        }
    };

    // Тестовый класс для тестов семантических операций
    TEST_CLASS(SemanticTests)
    {
    public:
        // Инициализирующий тестовый метод (выполняется перед выполнением каждого тестового метода):
        // сбрасывает семантическое дерево в исходное состояние перед каждым тестом
        TEST_METHOD_INITIALIZE(Init)
        {
            ResetTree();
        }

        // Тестовый метод для проверки добавления переменной в текущую область видимости
        TEST_METHOD(TestIncludeVariable)
        {
            Tree::Cur->SemInclude("s", TYPE_SHORT_INT, 1, 1);
            Tree* found = Tree::Cur->FindUp(Tree::Cur, "s");

            Assert::IsNotNull(found);
            Assert::AreEqual(std::string("s"), found->n->id);
            Assert::AreEqual((int)TYPE_SHORT_INT, (int)found->n->DataType);
        }

        // Тестовый метод для проверки повторного объявления переменной в той же области
        TEST_METHOD(TestDuplicateVariable)
        {
            Tree::Cur->SemInclude("s", TYPE_SHORT_INT, 1, 1);
            bool dup = Tree::Cur->DupControl(Tree::Cur, "s");

            Assert::IsTrue(dup); // DupControl должен вернуть true (т.к. повторное использование переменной)
        }

        // Тестовый метод для проверки поиска переменной в родительской области видимости
        TEST_METHOD(TestFindVariableInParentBlock)
        {
            Tree::Cur->SemInclude("s", TYPE_SHORT_INT, 1, 1); // Объявляем переменную в глобальной области видимости
            Tree::Cur->SemEnterBlock(2, 1); // Входим во вложенный блок (область видимости)
            Tree* found = Tree::Cur->FindUp(Tree::Cur, "s"); // Ищем переменную из вложенной области видимости,
                                                             //она должна найтись в родительском блоке

            Assert::IsNotNull(found);
            Assert::AreEqual(std::string("s"), found->n->id);

            Tree::Cur->SemExitBlock(); // Возвращаемся в глобальную область видимости
        }

        // Тестовый метод для проверки присваивания значения переменной
        TEST_METHOD(TestSetVarValue)
        {
            Tree::Cur->SemInclude("l", TYPE_LONG_INT, 1, 1);
            SemNode val;
            val.DataType = TYPE_LONG_INT;
            val.hasValue = true;
            val.Value.v_int32 = 52;

            Tree::SetVarValue("l", val, 1, 1);
            SemNode received = Tree::GetVarValue("l", 1, 1);

            Assert::IsTrue(received.hasValue); // Проверка: есть ли значение у узла
            Assert::AreEqual(52, received.Value.v_int32); // Сравнение ожидаемого значения с фактическим
                                                          // (равно ли полученное значение тому, что мы присвоили)
        }
    };

    // Тестовый класс для тестов вычисления выражений
    TEST_CLASS(ExpressionTests)
    {
    public:
        // Тестовый метод для проверки арифметической операции умножения двух целых
        TEST_METHOD(TestArithmeticMult)
        {
            // Создаём 2 узла: целые числа 7 и 8
            SemNode left, right;
            left.DataType = TYPE_SHORT_INT; left.hasValue = true; left.Value.v_int16 = 7;
            right.DataType = TYPE_SHORT_INT; right.hasValue = true; right.Value.v_int16 = 8;

            SemNode result = Tree::ExecuteArithmeticOp(left, right, "*", 1, 1);

            // Должно получиться такое же целое 56
            Assert::IsTrue(result.hasValue);
            Assert::AreEqual((int16_t)56, result.Value.v_int16);
            Assert::AreEqual((int)TYPE_SHORT_INT, (int)result.DataType);
        }
    };
}
