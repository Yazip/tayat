#include "tree.h"

#include <iostream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <functional>

Tree* Tree::Root = nullptr;
Tree* Tree::Cur = nullptr;
bool Tree::interpretationEnabled = true; // По умолчанию включена
bool Tree::debug = true; // По умолчанию включен подробный вывод
Tree* Tree::currentArea = nullptr;

// Печать предупреждения о преобразовании типов
void Tree::PrintTypeConversionWarning(DATA_TYPE from, DATA_TYPE to, const std::string& context,
    const std::string& expression, int line, int col) {
    
    // Выводим только если включен debug режим
    if (!debug) return;

    std::cerr << "Предупреждение: неявное преобразование типа ";

    switch (from) {
    case TYPE_SHORT_INT: std::cerr << "short"; break;
    case TYPE_INT: std::cerr << "int"; break;
    case TYPE_LONG_INT: std::cerr << "long"; break;
    case TYPE_LONG_LONG_INT: std::cerr << "longlong"; break;
    default: std::cerr << "unknown"; break;
    }

    std::cerr << " к ";

    switch (to) {
    case TYPE_SHORT_INT: std::cerr << "short"; break;
    case TYPE_INT: std::cerr << "int"; break;
    case TYPE_LONG_INT: std::cerr << "long"; break;
    case TYPE_LONG_LONG_INT: std::cerr << "longlong"; break;
    default: std::cerr << "unknown"; break;
    }

    std::cerr << " в " << context;
    if (!expression.empty()) {
        std::cerr << " выражения " << expression;
    }
    std::cerr << std::endl << "(строка " << line << ":" << col << ")" << std::endl;
}

// Печать семантической ошибки
void Tree::SemError(const std::string& msg, const std::string& id, int line, int col) {
    std::cerr << "Семантическая ошибка: " << msg;
    if (!id.empty()) {
        std::cerr << " (около '" << id << "')";
    }
    std::cerr << std::endl << "(строка " << line << ":" << col << ")" << std::endl;
    std::exit(1);
}

void Tree::InterpError(const std::string& msg, const std::string& id, int line, int col) {
    std::cerr << "Ошибка при интерпретации: " << msg;
    if (!id.empty()) std::cerr << " (около '" << id << "')";
    std::cerr << std::endl << "(строка " << line << ":" << col << ")" << std::endl;
    std::exit(1);
}

Tree::Tree(SemNode* node, Tree* up) : n(node), Up(up), Left(nullptr), Right(nullptr) {
    if (Root == nullptr) {
        Root = this;
        Cur = this;
    }
}

Tree::~Tree() {
    if (n) {
        delete n;
    }
    // Рекурсивно очищаем детей/соседей
    if (Right) { 
        delete Right; Right = nullptr;
    }
    if (Left) {
        delete Left; Left = nullptr;
    }
}

// Вставка первого дочернего элемента (правая ссылка)
// Если Right==nullptr, создаём новый узел и присваиваем Right
// Иначе добавляем в конец списка Left-соседей
void Tree::SetRight(SemNode* Data) {
    Tree* newNode = new Tree(Data, this);
    if (this->Right == nullptr) {
        this->Right = newNode;
    }
    else {
        // Найти последнего левого соседа среди детей
        Tree* p = this->Right;
        while (p->Left) {
            p = p->Left;
        }
        p->Left = newNode;
        newNode->Up = this;
    }
}

// Вставка левого соседа относительно THIS
// Обычно вызывается на некотором узле: this->Left = newNode
void Tree::SetLeft(SemNode* Data) {
    Tree* newNode = new Tree(Data, this->Up);
    // Вставляем после текущего узла
    newNode->Left = this->Left;
    this->Left = newNode;
    newNode->Up = this->Up; // Тот же родитель, что и у текущего узла
}

// FindUpOneLevel: ищет имя id среди дочерних элементов узла From (т.е. в текущем уровне)
Tree* Tree::FindUpOneLevel(Tree* From, const std::string& id) {
    if (From == nullptr) {
        return nullptr;
    }
    Tree* p = From->Right;
    while (p != nullptr) {
        if ((p->n) && (p->n->id == id)) {
            return p;
        }
        p = p->Left;
    }
    return nullptr;
}

// FindUp: поиск с подъёмом по областям (блочная видимость)
Tree* Tree::FindUp(Tree* From, const std::string& id) {
    Tree* cur = From;
    while (cur != nullptr) {
        Tree* found = FindUpOneLevel(cur, id);
        if (found) return found;
        cur = cur->Up;
    }
    return nullptr;
}

// DupControl: проверка дубля на уровне Addr (Addr — текущая область)
bool Tree::DupControl(Tree* Addr, const std::string& a) {
    return FindUpOneLevel(Addr, a) != nullptr;
}

// SemInclude: добавляет идентификатор в текущую область Cur
Tree* Tree::SemInclude(const std::string& a, DATA_TYPE t, int line, int col) {
    if (Cur == nullptr) {
        SemError("Внутренняя ошибка: текущая область не установлена при SemInclude", a, line, col);
    }

    if (DupControl(Cur, a)) {
        SemError("Повторное описание идентификатора", a, line, col);
    }

    SemNode* node = new SemNode();
    node->id = a;
    node->DataType = t;
    node->hasValue = false;
    node->FlagConst = 0;
    node->BasicType = TYPE_UNDEFINED;
    node->ArrElemCount = 0;
    node->index = 0;
    node->line = line;
    node->col = col;

    Cur->SetRight(node);
    Tree* added = Cur->Right;
    while (added->Left) added = added->Left;
    return added;
}

// Занесение константы со значением
Tree* Tree::SemIncludeConstant(const std::string& a, DATA_TYPE t, const std::string& value, int line, int col) {
    Tree* node = SemInclude(a, t, line, col);
    if (node && node->n) {
        node->n->hasValue = true;
        // Преобразование строкового значения в соответствующий тип
        try {
            long long val = std::stoll(value, nullptr, 0);
            if (t == TYPE_SHORT_INT) {
                node->n->Value.v_int16 = static_cast<int16_t>(val);
            }
            else if (t == TYPE_INT) {
                node->n->Value.v_int32 = static_cast<int32_t>(val);
            }
            else if (t == TYPE_LONG_INT) {
                node->n->Value.v_int32 = static_cast<int32_t>(val);
            }
            else if (t == TYPE_LONG_LONG_INT) {
                node->n->Value.v_int64 = static_cast<int64_t>(val);
            }
        }
        catch (const std::exception& e) {
            SemError("Неверный формат константы: " + std::string(e.what()), a, line, col);
        }
    }
    return node;
}

// SemSetConst: установить флаг именованной константы
void Tree::SemSetConst(Tree* Addr, int value) {
    if (Addr == nullptr || Addr->n == nullptr) {
        SemError("SemSetConst: неверный адрес именованной константы");
    }
    Addr->n->FlagConst = value;
}

// SemSetBasicType: установить базовый тип (для массивов и меток типов)
void Tree::SemSetBasicType(Tree* Addr, DATA_TYPE bt) {
    if (Addr == nullptr || Addr->n == nullptr) {
        SemError("SemSetBasicType: неверный адрес массива или метки типа");
    }
    Addr->n->BasicType = bt;
}

// SemSetArrElemCount: установить размерность массива
void Tree::SemSetArrElemCount(Tree* Addr, int aec) {
    if (Addr == nullptr || Addr->n == nullptr) {
        SemError("SemSetArrElemCount: неверный адрес массива");
    }
    Addr->n->ArrElemCount = aec;
}

// SemSetIndex: установить индекс элемента массива
void Tree::SemSetIndex(Tree* Addr, int index) {
    if (Addr == nullptr || Addr->n == nullptr) {
        SemError("SemSetIndex: неверный адрес элемента массива");
    }
    Addr->n->index = index;
}

// SemGetVar: найти переменную / именованную константу (не метку типа) по имени (в видимых областях)
Tree* Tree::SemGetVar(const std::string& a, int line, int col) {
    Tree* v = FindUp(Cur, a);
    if (v == nullptr) {
        SemError("Отсутствует описание идентификатора", a, line, col);
    }
    if (v->n->DataType == TYPE_TYPEDEF_NAME) {
        SemError("Неверное использование - идентификатор является меткой типа", a, line, col);
    }
    return v;
}

// SemGetType: найти метку типа по имени
Tree* Tree::SemGetType(const std::string& a, int line, int col) {
    Tree* v = FindUp(Cur, a);
    if (v == nullptr) {
        SemError("Отсутствует описание метки типа", a, line, col);
    }
    if (v->n->DataType != TYPE_TYPEDEF_NAME) {
        SemError("Идентификатор не является меткой типа", a, line, col);
    }
    return v;
}

// SemEnterBlock: создать узел-область и перейти в него
Tree* Tree::SemEnterBlock(int line, int col) {
    if (Cur == nullptr) {
        SemError("SemEnterBlock: текущая область не установлена");
    }
    SemNode* sn = new SemNode();
    sn->id = "";
    sn->DataType = TYPE_SCOPE;
    sn->FlagConst = 0;
    sn->BasicType = TYPE_UNDEFINED;
    sn->ArrElemCount = 0;
    sn->index = 0;
    sn->line = line;
    sn->col = col;

    // Вставляем новую область как дочерний элемент текущего Cur
    // (т.е. она будет видимой как локальная область для последующих SemInclude)
    SetRight(sn);

    // Возвращаем указатель на созданный узел (последний дочерний)
    Tree* created = Cur->Right;
    while (created->Left) {
        created = created->Left;
    }

    // переключаем текущую область на созданную
    Cur = created;
    return created;
}

// SemExitBlock: вернуться на уровень вверх
void Tree::SemExitBlock() {
    if (Cur == nullptr) {
        SemError("SemExitBlock: текущая область не установлена");
    }
    if (Cur->Up == nullptr) {
        SemError("SemExitBlock: попытка выйти из корневой области");
    }
    Cur = Cur->Up;
}

void Tree::SetVarValue(const std::string& name, const SemNode& value, int line, int col) {
    Tree* varNode = Cur->SemGetVar(name, line, col);

    if (value.hasValue) {
        if (CanImplicitCast(value.DataType, varNode->n->DataType)) {
            // Проверяем обрезку значений для всех типов
            bool needsTruncationWarning = false;
            long long originalValue = 0;

            // Получаем оригинальное значение в long long
            switch (value.DataType) {
            case TYPE_SHORT_INT: originalValue = value.Value.v_int16; break;
            case TYPE_INT: originalValue = value.Value.v_int32; break;
            case TYPE_LONG_INT: originalValue = value.Value.v_int32; break;
            case TYPE_LONG_LONG_INT: originalValue = value.Value.v_int64; break;
            default: break;
            }

            // Проверяем обрезку для типа переменной
            if (varNode->n->DataType == TYPE_SHORT_INT) {
                if (originalValue < -32768 || originalValue > 32767) {
                    needsTruncationWarning = true;
                }
            }
            else if (varNode->n->DataType == TYPE_INT) {
                if (originalValue < -2147483648LL || originalValue > 2147483647LL) {
                    needsTruncationWarning = true;
                }
            }
            else if (varNode->n->DataType == TYPE_LONG_INT) {
                if (originalValue < -2147483648LL || originalValue > 2147483647LL) {
                    needsTruncationWarning = true;
                }
            }

            // Выводим предупреждение об обрезке всегда (независимо от debug)
            if (needsTruncationWarning) {
                std::string text_type = "long";
                if (varNode->n->DataType == TYPE_SHORT_INT) {
                    text_type = "short";
                }
                else if (varNode->n->DataType == TYPE_INT) {
                    text_type = "int";
                }

                std::cerr << "Предупреждение: значение " << originalValue
                    << " обрезается при преобразовании к "
                    << text_type;
                std::cerr << std::endl << "(строка " << line << ":" << col << ")" << std::endl;
            }
            // Выводим предупреждение о преобразовании типов только в debug режиме
            else if (value.DataType != varNode->n->DataType && debug) {
                PrintTypeConversionWarning(value.DataType, varNode->n->DataType,
                    "присваивании", name + " = ...", line, col);
            }

            SemNode converted = CastToType(value, varNode->n->DataType, line, col);
            varNode->n->Value = converted.Value;
            varNode->n->hasValue = true;

            PrintAssignment(name, converted, line, col);
        }
        else {
            SemError("Несовместимые типы при присваивании", name, line, col);
        }
    }
    else {
        InterpError("Попытка присвоить NULL", name, line, col);
    }
}

SemNode Tree::GetVarValue(const std::string& name, int line, int col) {
    Tree* varNode = Cur->SemGetVar(name, line, col); // Используем Cur->
    if (!varNode->n->hasValue) {
        SemError("Использование неинициализированной переменной", name, line, col);
    }
    return *(varNode->n);
}

// Определение максимального типа
DATA_TYPE Tree::GetMaxType(DATA_TYPE t1, DATA_TYPE t2) {
    if (t1 == TYPE_LONG_LONG_INT || t2 == TYPE_LONG_LONG_INT) return TYPE_LONG_LONG_INT;
    if (t1 == TYPE_LONG_INT || t2 == TYPE_LONG_INT) return TYPE_LONG_INT;
    if (t1 == TYPE_INT || t2 == TYPE_INT) return TYPE_INT;
    return TYPE_SHORT_INT;
}

// Проверка возможности неявного приведения
bool Tree::CanImplicitCast(DATA_TYPE from, DATA_TYPE to) {
    // Разрешено между целочисленными типами
    bool fromIsInt = (from == TYPE_SHORT_INT || from == TYPE_INT || from == TYPE_LONG_INT || from == TYPE_LONG_LONG_INT);
    bool toIsInt = (to == TYPE_SHORT_INT || to == TYPE_INT || to == TYPE_LONG_INT || to == TYPE_LONG_LONG_INT);
    return (fromIsInt && toIsInt);
}

// Приведение типов
SemNode Tree::CastToType(const SemNode& value, DATA_TYPE targetType, int line, int col, bool showWarning) {
    // Если типы уже совпадают, возвращаем без изменений
    if (value.DataType == targetType) {
        return value;
    }

    SemNode result;
    result.DataType = targetType;
    result.hasValue = value.hasValue;

    if (!value.hasValue) return result;

    // Получаем значение в long long для безопасного приведения
    long long originalValue = 0;
    switch (value.DataType) {
    case TYPE_SHORT_INT: originalValue = value.Value.v_int16; break;
    case TYPE_INT: originalValue = value.Value.v_int32; break;
    case TYPE_LONG_INT: originalValue = value.Value.v_int32; break;
    case TYPE_LONG_LONG_INT: originalValue = value.Value.v_int64; break;
    default: break;
    }

    // Выполняем приведение
    switch (targetType) {
    case TYPE_SHORT_INT:
        result.Value.v_int16 = static_cast<int16_t>(originalValue);
        break;

    case TYPE_INT:
        result.Value.v_int32 = static_cast<int32_t>(originalValue);
        break;

    case TYPE_LONG_INT:
        result.Value.v_int32 = static_cast<int32_t>(originalValue);
        break;

    case TYPE_LONG_LONG_INT:
        result.Value.v_int64 = static_cast<int64_t>(originalValue);
        break;

    default:
        SemError("Неизвестный тип для приведения", "", line, col);
    }

    return result;
}

// Арифметические операции
SemNode Tree::ExecuteArithmeticOp(const SemNode& left, const SemNode& right, const std::string& op, int line, int col) {
    if (!left.hasValue || !right.hasValue) {
        SemError("Операция с неинициализированными значениями", "", line, col);
    }

    // Выводим предупреждение если операнды разных типов
    if (left.DataType != right.DataType && debug) {
        PrintTypeConversionWarning(left.DataType, right.DataType,
            "арифметической операции", "", line, col);
    }

    DATA_TYPE resultType = GetMaxType(left.DataType, right.DataType);
    SemNode leftConv = CastToType(left, resultType, line, col);
    SemNode rightConv = CastToType(right, resultType, line, col);

    SemNode result;
    result.DataType = resultType;
    result.hasValue = true;

    switch (resultType) {
    case TYPE_SHORT_INT:
        if (op == "+") result.Value.v_int16 = leftConv.Value.v_int16 + rightConv.Value.v_int16;
        else if (op == "-") result.Value.v_int16 = leftConv.Value.v_int16 - rightConv.Value.v_int16;
        else if (op == "*") result.Value.v_int16 = leftConv.Value.v_int16 * rightConv.Value.v_int16;
        else if (op == "/") {
            if (rightConv.Value.v_int16 == 0) InterpError("Деление на ноль", "", line, col);
            result.Value.v_int16 = leftConv.Value.v_int16 / rightConv.Value.v_int16;
        }
        else if (op == "%") {
            if (rightConv.Value.v_int16 == 0) InterpError("Деление на ноль", "", line, col);
            result.Value.v_int16 = leftConv.Value.v_int16 % rightConv.Value.v_int16;
        }
        break;

    case TYPE_INT:
        if (op == "+") result.Value.v_int32 = leftConv.Value.v_int32 + rightConv.Value.v_int32;
        else if (op == "-") result.Value.v_int32 = leftConv.Value.v_int32 - rightConv.Value.v_int32;
        else if (op == "*") result.Value.v_int32 = leftConv.Value.v_int32 * rightConv.Value.v_int32;
        else if (op == "/") {
            if (rightConv.Value.v_int32 == 0) InterpError("Деление на ноль", "", line, col);
            result.Value.v_int32 = leftConv.Value.v_int32 / rightConv.Value.v_int32;
        }
        else if (op == "%") {
            if (rightConv.Value.v_int32 == 0) InterpError("Деление на ноль", "", line, col);
            result.Value.v_int32 = leftConv.Value.v_int32 % rightConv.Value.v_int32;
        }
        break;

    case TYPE_LONG_INT:
        if (op == "+") result.Value.v_int32 = leftConv.Value.v_int32 + rightConv.Value.v_int32;
        else if (op == "-") result.Value.v_int32 = leftConv.Value.v_int32 - rightConv.Value.v_int32;
        else if (op == "*") result.Value.v_int32 = leftConv.Value.v_int32 * rightConv.Value.v_int32;
        else if (op == "/") {
            if (rightConv.Value.v_int32 == 0) InterpError("Деление на ноль", "", line, col);
            result.Value.v_int32 = leftConv.Value.v_int32 / rightConv.Value.v_int32;
        }
        else if (op == "%") {
            if (rightConv.Value.v_int32 == 0) InterpError("Деление на ноль", "", line, col);
            result.Value.v_int32 = leftConv.Value.v_int32 % rightConv.Value.v_int32;
        }
        break;

    case TYPE_LONG_LONG_INT:
        if (op == "+") result.Value.v_int64 = leftConv.Value.v_int64 + rightConv.Value.v_int64;
        else if (op == "-") result.Value.v_int64 = leftConv.Value.v_int64 - rightConv.Value.v_int64;
        else if (op == "*") result.Value.v_int64 = leftConv.Value.v_int64 * rightConv.Value.v_int64;
        else if (op == "/") {
            if (rightConv.Value.v_int64 == 0) InterpError("Деление на ноль", "", line, col);
            result.Value.v_int64 = leftConv.Value.v_int64 / rightConv.Value.v_int64;
        }
        else if (op == "%") {
            if (rightConv.Value.v_int64 == 0) InterpError("Деление на ноль", "", line, col);
            result.Value.v_int64 = leftConv.Value.v_int64 % rightConv.Value.v_int64;
        }
        break;

    default:
        SemError("Неподдерживаемый тип для арифметической операции", "", line, col);
    }

    // Вывод информации об операции (отладочный)
    if (debug && interpretationEnabled) {
        PrintArithmeticOp(op, leftConv, rightConv, result, line, col);
    }

    return result;
}

// Операции сравнения
SemNode Tree::ExecuteComparisonOp(const SemNode& left, const SemNode& right, const std::string& op, int line, int col) {
    if (!left.hasValue || !right.hasValue) {
        SemError("Операция с неинициализированными значениями", "", line, col);
    }

    DATA_TYPE resultType = GetMaxType(left.DataType, right.DataType);
    SemNode leftConv = CastToType(left, resultType, line, col);
    SemNode rightConv = CastToType(right, resultType, line, col);

    SemNode result;
    result.DataType = TYPE_INT;
    result.hasValue = true;

    switch (resultType) {
    case TYPE_SHORT_INT:
        if (op == "<") result.Value.v_int32 = leftConv.Value.v_int16 < rightConv.Value.v_int16;
        else if (op == "<=") result.Value.v_int32 = leftConv.Value.v_int16 <= rightConv.Value.v_int16;
        else if (op == ">") result.Value.v_int32 = leftConv.Value.v_int16 > rightConv.Value.v_int16;
        else if (op == ">=") result.Value.v_int32 = leftConv.Value.v_int16 >= rightConv.Value.v_int16;
        else if (op == "==") result.Value.v_int32 = leftConv.Value.v_int16 == rightConv.Value.v_int16;
        else if (op == "!=") result.Value.v_int32 = leftConv.Value.v_int16 != rightConv.Value.v_int16;
        break;

    case TYPE_INT:
        if (op == "<") result.Value.v_int32 = leftConv.Value.v_int32 < rightConv.Value.v_int32;
        else if (op == "<=") result.Value.v_int32 = leftConv.Value.v_int32 <= rightConv.Value.v_int32;
        else if (op == ">") result.Value.v_int32 = leftConv.Value.v_int32 > rightConv.Value.v_int32;
        else if (op == ">=") result.Value.v_int32 = leftConv.Value.v_int32 >= rightConv.Value.v_int32;
        else if (op == "==") result.Value.v_int32 = leftConv.Value.v_int32 == rightConv.Value.v_int32;
        else if (op == "!=") result.Value.v_int32 = leftConv.Value.v_int32 != rightConv.Value.v_int32;
        break;

    case TYPE_LONG_INT:
        if (op == "<") result.Value.v_int32 = leftConv.Value.v_int32 < rightConv.Value.v_int32;
        else if (op == "<=") result.Value.v_int32 = leftConv.Value.v_int32 <= rightConv.Value.v_int32;
        else if (op == ">") result.Value.v_int32 = leftConv.Value.v_int32 > rightConv.Value.v_int32;
        else if (op == ">=") result.Value.v_int32 = leftConv.Value.v_int32 >= rightConv.Value.v_int32;
        else if (op == "==") result.Value.v_int32 = leftConv.Value.v_int32 == rightConv.Value.v_int32;
        else if (op == "!=") result.Value.v_int32 = leftConv.Value.v_int32 != rightConv.Value.v_int32;
        break;

    case TYPE_LONG_LONG_INT:
        if (op == "<") result.Value.v_int32 = leftConv.Value.v_int64 < rightConv.Value.v_int64;
        else if (op == "<=") result.Value.v_int32 = leftConv.Value.v_int64 <= rightConv.Value.v_int64;
        else if (op == ">") result.Value.v_int32 = leftConv.Value.v_int64 > rightConv.Value.v_int64;
        else if (op == ">=") result.Value.v_int32 = leftConv.Value.v_int64 >= rightConv.Value.v_int64;
        else if (op == "==") result.Value.v_int32 = leftConv.Value.v_int64 == rightConv.Value.v_int64;
        else if (op == "!=") result.Value.v_int32 = leftConv.Value.v_int64 != rightConv.Value.v_int64;
        break;

    default:
        SemError("Неподдерживаемый тип для операции сравнения", "", line, col);
    }

    return result;
}

// Создать строковое представление узла для печати
std::string Tree::makeLabel(const Tree* tree) const {
    if (tree == nullptr) {
        return std::string("<null-tree>");
    }
    SemNode* n = tree->n;
    if (!n) {
        return std::string("<null-node>");
    }

    std::ostringstream oss;

    if (!n->id.empty()) {
        oss << n->id;
    }
    else {
        oss << "{}";
    }

    // Тип
    switch (n->DataType) {
    case TYPE_INT:              oss << " (int)"; break;
    case TYPE_SHORT_INT:        oss << " (short)"; break;
    case TYPE_LONG_INT:         oss << " (long)"; break;
    case TYPE_LONG_LONG_INT:    oss << " (longlong)"; break;
    case TYPE_ARRAY:            oss << " (массив)[" << n->ArrElemCount << "] (типа "; break;
    case TYPE_TYPEDEF_NAME:     oss << " (метка типа) (для типа "; break;
    case TYPE_SCOPE:            oss << " (область)"; break;
    default:                    oss << " (?)"; break;
    }

    if (n->FlagConst) {
        oss << " (const)";
    }

    if (n->DataType == TYPE_ARRAY) {
        switch (n->BasicType) {
        case TYPE_INT: oss << "int)"; break;
        case TYPE_SHORT_INT: oss << "short)"; break;
        case TYPE_LONG_INT: oss << "long)"; break;
        case TYPE_LONG_LONG_INT: oss << "longlong)"; break;
        default: oss << "?)"; break;
        }
    }

    if (n->DataType == TYPE_TYPEDEF_NAME) {
        switch (n->BasicType) {
        case TYPE_INT: oss << "int)"; break;
        case TYPE_SHORT_INT: oss << "short)"; break;
        case TYPE_LONG_INT: oss << "long)"; break;
        case TYPE_LONG_LONG_INT: oss << "longlong)"; break;
        default: oss << "?)"; break;
        }

        if (n->ArrElemCount > 0) {
            oss << " (массив как тип)[" << n->ArrElemCount << "]";
        }
    }

    // Берём имена правого и левого потомков
    auto childName = [](const Tree* t)->std::string {
        if (!t || !t->n) {
            return std::string("-");
        }
        if (t->n->DataType == TYPE_SCOPE) {
            return std::string("{}");
        }
        if (t->n->id.empty()) {
            return std::string("?");
        }
        return t->n->id;
    };
     
    std::string rname = childName(tree->Right);
    std::string lname = childName(tree->Left);

    oss << " (L = " << lname << ", R = " << rname << ")";

    return oss.str();
}

void Tree::Print(int depth) {
    if (this == nullptr) return;

    // Печатаем текущий узел
    std::string label = makeLabel(this);
    for (int i = 0; i < depth; ++i) {
        std::cout << ' ';
    }
    std::cout << label << '\n';

    // Затем рекурсивно печатаем всех потомков
    Tree* child = this->Right;
    while (child) {
        child->Print(depth + 4); // Сдвиг для уровня (4 пробела)
        child = child->Left;
    }
}

// Печать дерева с нулевого отступа
void Tree::Print() {
    Print(0);
}

void Tree::EnableInterpretation() { interpretationEnabled = true; }
void Tree::DisableInterpretation() { interpretationEnabled = false; }
bool Tree::IsInterpretationEnabled() { return interpretationEnabled; }

// Метод для вывода отладочной информации
void Tree::PrintDebugInfo(const std::string& message, int line, int col) {
    if (!debug || !interpretationEnabled) return;

    // Определяем контекст
    std::string context = "глобальная область";

    // Используем currentArea для определения контекста
    if (currentArea && currentArea->n) {
        context = currentArea->n->id;
    }

    // Выводим сообщение с контекстом и позицией
    std::cout << "DEBUG: [" << context << "]";
    if (line > 0) {
        std::cout << " (строка " << line << ":" << col << ")";
    }
    std::cout << " " << message << std::endl;
}

// Метод для вывода присваивания
void Tree::PrintAssignment(const std::string& varName, const SemNode& value, int line, int col) {
    if (!debug || !interpretationEnabled) return;

    std::ostringstream oss;
    oss << "Присваивание: " << varName << " = ";

    if (value.hasValue) {
        switch (value.DataType) {
        case TYPE_SHORT_INT:
            oss << value.Value.v_int16 << " (short)";
            break;
        case TYPE_INT:
            oss << value.Value.v_int32 << " (int)";
            break;
        case TYPE_LONG_INT:
            oss << value.Value.v_int32 << " (long)";
            break;
        case TYPE_LONG_LONG_INT:
            oss << value.Value.v_int64 << " (longlong)";
            break;
        default:
            oss << "unknown";
        }
    }
    else {
        oss << "неинициализирована";
    }

    PrintDebugInfo(oss.str(), line, col);
}

// Метод для вывода арифметической операции
void Tree::PrintArithmeticOp(const std::string& op, const SemNode& left, const SemNode& right, const SemNode& result, int line, int col) {
    if (!debug || !interpretationEnabled) return;

    std::ostringstream oss;
    oss << "Арифметическая операция: ";

    // Левый операнд
    if (left.hasValue) {
        switch (left.DataType) {
        case TYPE_SHORT_INT: oss << left.Value.v_int16 << " (short)"; break;
        case TYPE_INT: oss << left.Value.v_int32 << " (int)"; break;
        case TYPE_LONG_INT: oss << left.Value.v_int32 << " (long)"; break;
        case TYPE_LONG_LONG_INT: oss << left.Value.v_int64 << " (longlong)"; break;
        default: oss << "unknown";
        }
    }
    else {
        oss << "неинициализирована";
    }

    oss << " " << op << " ";

    // Правый операнд
    if (right.hasValue) {
        switch (right.DataType) {
        case TYPE_SHORT_INT: oss << right.Value.v_int16 << " (short)"; break;
        case TYPE_INT: oss << right.Value.v_int32 << " (int)"; break;
        case TYPE_LONG_INT: oss << right.Value.v_int32 << " (long)"; break;
        case TYPE_LONG_LONG_INT: oss << right.Value.v_int64 << " (longlong)"; break;
        default: oss << "unknown";
        }
    }
    else {
        oss << "неинициализирована";
    }

    oss << " = ";

    // Результат
    if (result.hasValue) {
        switch (result.DataType) {
        case TYPE_SHORT_INT: oss << result.Value.v_int16 << " (short)"; break;
        case TYPE_INT: oss << result.Value.v_int32 << " (int)"; break;
        case TYPE_LONG_INT: oss << result.Value.v_int32 << " (long)"; break;
        case TYPE_LONG_LONG_INT: oss << result.Value.v_int64 << " (longlong)"; break;
        default: oss << "unknown";
        }
    }
    else {
        oss << "неинициализирована";
    }

    PrintDebugInfo(oss.str(), line, col);
}

void Tree::EnableDebug() { debug = true; }
void Tree::DisableDebug() { debug = false; }
bool Tree::IsDebugEnabled() { return debug; }