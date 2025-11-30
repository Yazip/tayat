#include "tree.h"

#include <iostream>
#include <sstream>

Tree* Tree::Root = nullptr;
Tree* Tree::Cur = nullptr;

// Печать семантической ошибки
void Tree::SemError(const std::string& msg, const std::string& id, int line, int col) {
    std::cerr << "Семантическая ошибка: " << msg;
    if (!id.empty()) {
        std::cerr << " (около '" << id << "')";
    }
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
    if (FindUpOneLevel(Addr, a) == nullptr) return false;
    return true;
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
    node->Data = nullptr;
    node->FlagConst = 0;
    node->BasicType = TYPE_UNDEFINED;
    node->ArrElemCount = 0;
    node->line = line;
    node->col = col;

    Cur->SetRight(node);
    Tree* added = Cur->Right;
    while (added->Left) added = added->Left;
    return added;
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