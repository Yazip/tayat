#include "diagram.h"
#include "tree.h"

#include <iostream>

Diagram::Diagram(Scanner* scanner) : sc(scanner), cur_tok(0), cur_lex(), current_decl_type(TYPE_UNDEFINED), current_arr_elem_count(0) {
    push_tok.clear();
    push_lex.clear();
}

void Diagram::synError(const std::string& msg) {
    std::pair<int, int> lc = sc->getLineCol();
    std::cerr << "Синтаксическая ошибка: " << msg;
    if (!cur_lex.empty()) std::cerr << " (около '" << cur_lex << "')";
    std::cerr << std::endl << "(строка " << lc.first << ":" << lc.second << ")" << std::endl;
    std::exit(1);
}

void Diagram::lexError() {
    std::pair<int, int> lc = sc->getLineCol();
    std::cerr << "Лексическая ошибка: неизвестная лексема '" << cur_lex << "'";
    std::cerr << std::endl << "(строка " << lc.first << ":" << lc.second << ")" << std::endl;
    std::exit(1);
}

void Diagram::semError(const std::string& msg) {
    std::pair<int, int> lc = sc->getLineCol();
    Tree::SemError(msg, cur_lex, lc.first, lc.second);
}

void Diagram::interpError(const std::string& msg) {
    auto lc = sc->getLineCol();
    Tree::InterpError(msg, cur_lex, lc.first, lc.second);
}

int Diagram::nextToken() {
    if (!push_tok.empty()) {
        int t = push_tok.back();
        push_tok.pop_back();
        std::string lx = push_lex.back();
        push_lex.pop_back();
        cur_tok = t;
        cur_lex = lx;
        if (cur_tok == T_ERR) {
            lexError();
        }
        return cur_tok;
    }

    std::string lex;
    cur_tok = sc->getNextLex(lex);
    cur_lex = lex;

    if (cur_tok == T_ERR) {
        lexError();
    }
    return cur_tok;
}

int Diagram::peekToken() {
    int t = nextToken();
    pushBack(t, cur_lex);
    return t;
}

void Diagram::pushBack(int tok, const std::string& lex) {
    push_tok.push_back(tok);
    push_lex.push_back(lex);
}

// Вспомогательные методы для интерпретации
void Diagram::pushValue(const SemNode& node) {
    eval_stack.push(node);
}

SemNode Diagram::popValue() {
    if (eval_stack.empty()) {
        semError("Внутренняя ошибка: стек вычислений пуст");
    }
    SemNode node = eval_stack.top();
    eval_stack.pop();
    return node;
}

SemNode Diagram::evaluateConstant(const std::string& value, DATA_TYPE type) {
    SemNode result;
    result.DataType = type;
    result.hasValue = true;

    try {
        // Обрабатываем отрицательные числа напрямую
        std::string value_to_parse = value;
        bool isNegative = false;

        if (!value_to_parse.empty() && value_to_parse[0] == '-') {
            isNegative = true;
            value_to_parse = value_to_parse.substr(1);
        }

        long long val = std::stoll(value_to_parse, nullptr, 0);

        if (isNegative) {
            val = -val;
        }

        if (type == TYPE_SHORT_INT) {
            result.Value.v_int16 = static_cast<int16_t>(val);
        }
        else if (type == TYPE_INT) {
            result.Value.v_int32 = static_cast<int32_t>(val);
        }
        else if (type == TYPE_LONG_INT) {
            result.Value.v_int32 = static_cast<int32_t>(val);
        }
        else if (type == TYPE_LONG_LONG_INT) {
            result.Value.v_int64 = static_cast<int64_t>(val);
        }
    }
    catch (const std::exception& e) {
        semError("Неверный формат константы: " + std::string(e.what()));
    }

    return result;
}

void Diagram::executeAssignment(const std::string& varName, DATA_TYPE exprType, int line, int col) {
    SemNode value = popValue();
    Tree* varNode = Tree::Cur->SemGetVar(varName, line, col);
    DATA_TYPE varType = varNode->n->DataType;

    bool varIsInt = (varType == TYPE_INT || varType == TYPE_SHORT_INT || varType == TYPE_LONG_INT || varType == TYPE_LONG_LONG_INT);
    bool exprIsInt = (exprType == TYPE_INT || exprType == TYPE_SHORT_INT || exprType == TYPE_LONG_INT || exprType == TYPE_LONG_LONG_INT);

    if (!(varIsInt && exprIsInt)) {
        semError("Несоответствие типов при присваивании для '" + varName + "'");
    }

    Tree::SetVarValue(varName, value, line, col);
}

// Точка входа
void Diagram::ParseProgram(bool isInterp, bool isDebug) {
    // Создаём корень семантического дерева (область верхнего уровня)
    SemNode* root_node = new SemNode();
    root_node->id = "<глобальная область видимости>";
    root_node->DataType = TYPE_SCOPE;
    root_node->line = 0;
    root_node->col = 0;
    Tree* root_tree = new Tree(root_node, nullptr);
    Tree::SetCur(root_tree);

    if (isInterp) {
        Tree::EnableInterpretation();
    }
    else {
        Tree::DisableInterpretation();
    }

    if (isDebug) {
        Tree::EnableDebug();
    }
    else {
        Tree::DisableDebug();
    }

    Program();

    // Проверим, что в конце файла действительно конец
    int t = peekToken();
    if (t != T_END) {
        synError("Лишний текст в конце программы");
    }

    if (!isInterp) {
        root_tree->Print();
    }
}

// Program -> TopDecl*
void Diagram::Program() {
    int t = peekToken();
    while (t == KW_INT || t == KW_SHORT || t == KW_LONG || t == KW_LONGLONG || t == IDENT || t == KW_TYPEDEF || t == KW_CONST) {
        TopDecl();
        t = peekToken();
    }
}

// TopDecl -> MainFunc | TypeDefinition | VarDecl | ConstDecl
void Diagram::TopDecl() {
    int t = peekToken();
    if (t == KW_INT) {
        nextToken();
        t = peekToken();
        if (t == KW_MAIN) {
            nextToken();
            MainFunc();
        }
        else {
            current_decl_type = TYPE_INT;
            current_arr_elem_count = 0;
            VarDecl();
        }
    }
    else if (t == KW_TYPEDEF) {
        current_decl_type = TYPE_TYPEDEF_NAME;

        nextToken();
        TypeDefinition();
    }
    else if (t == KW_CONST) {
        nextToken();
        ConstDecl();
    }
    else {
        if (t == KW_SHORT) {
            current_decl_type = TYPE_SHORT_INT;
            current_arr_elem_count = 0;
        }
        else if (t == KW_LONG) {
            current_decl_type = TYPE_LONG_INT;
            current_arr_elem_count = 0;
        }
        else if (t == KW_LONGLONG) {
            current_decl_type = TYPE_LONG_LONG_INT;
            current_arr_elem_count = 0;
        }
        else {
            Tree* saved_cur = Tree::Cur;
            Tree::SetCur(Tree::Root);

            t = nextToken();
            std::string typedef_name = cur_lex;
            pushBack(t, typedef_name);

            std::pair<int, int> lc = sc->getLineCol();
            
            Tree* typedef_node = Tree::Cur->SemGetType(typedef_name, lc.first, lc.second);

            current_decl_type = typedef_node->n->BasicType;
            current_arr_elem_count = typedef_node->n->ArrElemCount;

            Tree::SetCur(saved_cur);
        }

        nextToken();
        VarDecl();
    }
}

// MainFunc -> 'int' main '('')' Block
void Diagram::MainFunc() {
    // Запомним текущую область, чтобы потом восстановить
    Tree* saved_cur = Tree::Cur;

    int t = peekToken();
    if (t != LPAREN) {
        synError("Ожидалась '(' после main");
    }

    nextToken();
    t = peekToken();
    if (t != RPAREN) {
        synError("Ожидалась ')' после '('");
    }
    nextToken();

    // Тело main
    Block();


    // Восстанавливаем предыдущую область
    Tree::SetCur(saved_cur);
    Tree::SetCurrentArea(saved_cur);
}

// VarDecl -> Type IdInitList ;
void Diagram::VarDecl() {
    IdInitList();
    int t = peekToken();
    if (t != SEMI) {
        synError("Ожидалась ';' в конце объявления переменных");
    }
    nextToken();
}

// ConstDecl -> const Type IdInitList ;
void Diagram::ConstDecl() {
    int t = peekToken();
    if (t != KW_INT && t != KW_SHORT && t != KW_LONG && t != KW_LONGLONG && t != IDENT) {
        synError("Ожидался тип данных после const");
    }

    if (t == KW_INT) {
        current_decl_type = TYPE_INT;
        current_arr_elem_count = 0;
    }
    else if (t == KW_SHORT) {
        current_decl_type = TYPE_SHORT_INT;
        current_arr_elem_count = 0;
    }
    else if (t == KW_LONG) {
        current_decl_type = TYPE_LONG_INT;
        current_arr_elem_count = 0;
    }
    else if (t == KW_LONGLONG) {
        current_decl_type = TYPE_LONG_LONG_INT;
        current_arr_elem_count = 0;
    }
    else {
        Tree* saved_cur = Tree::Cur;
        Tree::SetCur(Tree::Root);

        t = nextToken();
        std::string typedef_name = cur_lex;
        pushBack(t, typedef_name);

        std::pair<int, int> lc = sc->getLineCol();

        Tree* typedef_node = Tree::Cur->SemGetType(typedef_name, lc.first, lc.second);

        current_decl_type = typedef_node->n->BasicType;
        current_arr_elem_count = typedef_node->n->ArrElemCount;

        if (current_arr_elem_count > 0) {
            semError("Нельзя объявить именованную константу-массив");
        }

        Tree::SetCur(saved_cur);
    }

    nextToken();
    IdInitList(true);
    t = peekToken();
    if (t != SEMI) {
        synError("Ожидалась ';' в конце объявления именованных констант");
    }
    nextToken();
}

// TypeDefinition -> typedef Type IDENT [[Const]] ;
void Diagram::TypeDefinition() {
    int t = peekToken();
    if (t != KW_INT && t != KW_SHORT && t != KW_LONG && t != KW_LONGLONG && t != IDENT) {
        synError("Ожидался тип данных после typedef");
    }

    DATA_TYPE basic_type;
    int basic_typedef_arr_elem_count = 0;
    if (t == KW_INT) {
        basic_type = TYPE_INT;
    }
    else if (t == KW_SHORT) {
        basic_type = TYPE_SHORT_INT;
    }
    else if (t == KW_LONG) {
        basic_type = TYPE_LONG_INT;
    }
    else if (t == KW_LONGLONG) {
        basic_type = TYPE_LONG_LONG_INT;
    }
    else {
        Tree* saved_cur = Tree::Cur;
        Tree::SetCur(Tree::Root);

        t = nextToken();
        std::string typedef_name = cur_lex;
        pushBack(t, typedef_name);

        std::pair<int, int> lc = sc->getLineCol();

        Tree* basic_typedef_node = Tree::Cur->SemGetType(typedef_name, lc.first, lc.second);

        basic_type = basic_typedef_node->n->BasicType;
        basic_typedef_arr_elem_count = basic_typedef_node->n->ArrElemCount;

        Tree::SetCur(saved_cur);
    }

    nextToken();
    t = peekToken();
    if (t != IDENT) {
        synError("Ожидался идентификатор в определении метки типа");
    }
    nextToken();
    std::string typedef_name = cur_lex;
    t = peekToken();

    int arr_elem_count = basic_typedef_arr_elem_count;

    if (t == LBRACKET) {
        nextToken();
        t = peekToken();
        if (t != CONST_DEC && t != CONST_HEX) {
            synError("Ожидалась константа после '['");
        }
        t = nextToken();
        int base = 10;
        if (t == CONST_HEX) {
            base = 16;
        }
        try {
            arr_elem_count = std::stoi(cur_lex, nullptr, base);
        }
        catch (const std::out_of_range& e) {
            semError("Размерность массива не может превышать диапазон типа int");
        }

        if (arr_elem_count <= 0) {
            semError("Размерность массива должна быть больше 0");
        }

        t = peekToken();
        if (t != RBRACKET) {
            synError("Ожидалась ']' после константы");
        }
        nextToken();
        t = peekToken();

        if (basic_typedef_arr_elem_count > 0) {
            semError("При объявлении массива через метку типа тип не может быть меткой типа для массива, объявленного ранее");
        }
    }

    std::pair<int, int> lc = sc->getLineCol();
    Tree* typedef_node = Tree::Cur->SemInclude(typedef_name, TYPE_TYPEDEF_NAME, lc.first, lc.second);
    typedef_node->SemSetBasicType(typedef_node, basic_type);
    typedef_node->SemSetArrElemCount(typedef_node, arr_elem_count);

    if (t != SEMI) {
        synError("Ожидалась ';' после определения метки");
    }
    nextToken();
}

// IdInitList -> IdInit (',' IdInit)*
void Diagram::IdInitList(bool const_flag) {
    IdInit(const_flag);
    int t = peekToken();
    while (t == COMMA) {
        nextToken(); // ','
        IdInit(const_flag);
        t = peekToken();
    }
}

// IdInit -> IDENT [ = Expr ]
void Diagram::IdInit(bool const_flag) {
    int t = peekToken();
    if (t != IDENT) {
        synError("Ожидался идентификатор в списке объявлений");
    }
    nextToken();

    std::string name = cur_lex;
    std::pair<int, int> lc = sc->getLineCol();
    Tree* node;
    
    if (current_arr_elem_count > 0) {
        node = Tree::Cur->SemInclude(name, TYPE_ARRAY, lc.first, lc.second);
        node->SemSetBasicType(node, current_decl_type);
        node->SemSetArrElemCount(node, current_arr_elem_count);

        for (int i = 0; i < current_arr_elem_count; i++) {
            Tree::Cur->SemInclude((name + "_" + std::to_string(i)), current_decl_type, lc.first, lc.second);
            node->SemSetIndex(node, i);
        }
    }
    else {
        node = Tree::Cur->SemInclude(name, current_decl_type, lc.first, lc.second);
        if (const_flag) {
            node->SemSetConst(node, const_flag);
        }
    }

    t = peekToken();
    if (t == ASSIGN) {
        if (node->n->DataType == TYPE_ARRAY) {
            semError("Нельзя ничего присваивать массиву целиком");
        }

        nextToken();
        DATA_TYPE expr_type = Expr();

        SemNode value = popValue();

        bool node_is_int = (node->n->DataType == TYPE_INT || node->n->DataType == TYPE_SHORT_INT || node->n->DataType == TYPE_LONG_INT || node->n->DataType == TYPE_LONG_LONG_INT);
        bool expr_is_int = (expr_type == TYPE_INT || expr_type == TYPE_SHORT_INT || expr_type == TYPE_LONG_INT || expr_type == TYPE_LONG_LONG_INT);

        if (!(node_is_int && expr_is_int)) {
            semError("Несоответствие типов при инициализации переменной / именованной константы '" + name + "'");
        }

        Tree::SetVarValue(node->n->id, value, sc->getLineCol().first, sc->getLineCol().second);
    }
    else {
        if (const_flag) {
            synError("Ожидалось '=' в определении именованной константы");
        }
    }
}

// Block -> '{' BlockItems '}'
void Diagram::Block() {
    int t = peekToken();
    if (t != LBRACE) {
        synError("Ожидалась '{' для начала блока");
    }

    auto lc = sc->getLineCol();
    Tree::Cur->SemEnterBlock(lc.first, lc.second);
    Tree::SetCurrentArea(Tree::Cur);

    t = nextToken();
    BlockItems();
    t = peekToken();
    if (t != RBRACE) {
        synError("Ожидалась '}' для конца блока");
    }

    Tree::Cur->SemExitBlock();
    Tree::SetCurrentArea(Tree::Cur);
    nextToken();
}

// BlockItems -> ( ConstDecl | VarDecl | Stmt )* 
void Diagram::BlockItems() {
    int t = peekToken();
    while (t != RBRACE && t != T_END) {
        if (t == KW_INT || t == KW_SHORT || t == KW_LONG || t == KW_LONGLONG || t == IDENT) {
            int t2 = nextToken();
            std::string type_name = cur_lex;

            t = peekToken();
            if (t == ASSIGN || t == LBRACKET) {
                pushBack(t2, type_name);
                Stmt();
            }
            else {
                if (t2 == KW_INT) {
                    current_decl_type = TYPE_INT;
                    current_arr_elem_count = 0;
                }
                else if (t2 == KW_SHORT) {
                    current_decl_type = TYPE_SHORT_INT;
                    current_arr_elem_count = 0;
                }
                else if (t2 == KW_LONG) {
                    current_decl_type = TYPE_LONG_INT;
                    current_arr_elem_count = 0;
                }
                else if (t2 == KW_LONGLONG) {
                    current_decl_type = TYPE_LONG_LONG_INT;
                    current_arr_elem_count = 0;
                }
                else {
                    Tree* saved_cur = Tree::Cur;
                    Tree::SetCur(Tree::Root);

                    std::pair<int, int> lc = sc->getLineCol();

                    Tree* typedef_node = Tree::Cur->SemGetType(type_name, lc.first, lc.second);

                    current_decl_type = typedef_node->n->BasicType;
                    current_arr_elem_count = typedef_node->n->ArrElemCount;

                    Tree::SetCur(saved_cur);
                }

                VarDecl();
            }
        }
        else if (t == KW_CONST) {
            nextToken();
            ConstDecl();
        }
        else {
            Stmt();
        }
        t = peekToken();
    }
}

// Stmt -> ';' | Block | Assign | While
void Diagram::Stmt() {
    int t = peekToken();
    if (t == SEMI) { 
        nextToken();
        return;
    }
    if (t == LBRACE) {
        Block();
        return;
    }
    if (t == KW_WHILE) {
        nextToken();
        WhileStmt();
        return;
    }

    if (t == IDENT) {
        t = nextToken();

        std::string name = cur_lex;
        std::pair<int, int> lc = sc->getLineCol();
        Tree* node = Tree::Cur->SemGetVar(name, lc.first, lc.second);

        if (node->n->FlagConst) {
            semError("Именованной константе может быть присвоено значение только при её объявлении");
        }

        t = peekToken();

        if (t == LBRACKET) {
            if (node->n->DataType != TYPE_ARRAY) {
                semError("Операция индексирования ([]) применима только к идентификаторам, объявленным как массив");
            }

            nextToken();
            t = peekToken();
            if (t != CONST_DEC && t != CONST_HEX) {
                synError("Ожидалась константа после '['");
            }
            t = nextToken();

            int index;
            int base = 10;
            if (t == CONST_HEX) {
                base = 16;
            }
            try {
                index = std::stoi(cur_lex, nullptr, base);
            }
            catch (const std::out_of_range& e) {
                semError("Индекс при обращении к массиву не может превышать диапазон типа int");
            }

            if ((index < 0) || (index >= node->n->ArrElemCount)) {
                semError("Индекс при обращении к массиву должен быть больше или равен 0 и меньше указанного при объявлении размера");
            }

            t = peekToken();
            if (t != RBRACKET) {
                synError("Ожидалась ']' после константы");
            }

            name = (name + "_" + std::to_string(index));

            nextToken();
            t = peekToken();
        }
        else {
            if (node->n->DataType == TYPE_ARRAY) {
                semError("Нельзя использовать массив целиком в качестве операнда");
            }
        }

        if (t == ASSIGN) {
            nextToken();
            
            DATA_TYPE expr_type = Expr();

            bool node_is_int;
            if (node->n->DataType == TYPE_ARRAY) {
                node_is_int = (node->n->BasicType == TYPE_INT || node->n->BasicType == TYPE_SHORT_INT || node->n->BasicType == TYPE_LONG_INT || node->n->BasicType == TYPE_LONG_LONG_INT);
            }
            else {
                node_is_int = (node->n->DataType == TYPE_INT || node->n->DataType == TYPE_SHORT_INT || node->n->DataType == TYPE_LONG_INT || node->n->DataType == TYPE_LONG_LONG_INT);
            }
            bool expr_is_int = (expr_type == TYPE_INT || expr_type == TYPE_SHORT_INT || expr_type == TYPE_LONG_INT || expr_type == TYPE_LONG_LONG_INT);

            if (!(node_is_int && expr_is_int)) {
                semError("Несоответствие типов в операторе присваивания");
            }

            // Выполняем присваивание
            executeAssignment(name, expr_type, sc->getLineCol().first, sc->getLineCol().second);

            t = peekToken();
            if (t != SEMI) {
                synError("Ожидалась ';' после оператора присваивания");
            }
            nextToken();
            return;
        }
        else {
            synError("Ожидалось '=' после идентификатора (присваивание)");
        }
    }

    synError("Неизвестная форма оператора");
}

// WhileStmt -> while ( Expr ) Stmt
void Diagram::WhileStmt() {
    int t = peekToken();
    if (t != LPAREN) {
        synError("Ожидалась '(' после while");
    }
    nextToken();

    DATA_TYPE cond = Expr();
    bool is_cond_int = (cond == TYPE_INT || cond == TYPE_SHORT_INT || cond == TYPE_LONG_INT || cond == TYPE_LONG_LONG_INT);
    if (!(is_cond_int)) {
        semError("Выражение-условие должно иметь тип int / short / long / longlong");
    }

    t = peekToken();
    if (t != RPAREN) {
        synError("Ожидалась ')' после выражения");
    }
    nextToken();
    Stmt();
}

// Expr -> ['+'|'-'] Rel ( ('==' | '!=') Rel )*
DATA_TYPE Diagram::Expr() {
    int t = peekToken();

    bool has_unary = false;
    std::string unary_op = "";

    // Пропускаем унарные операции для констант - они обрабатываются в Prim()
    // Оставляем только для случаев, когда это не константа
    if (t == PLUS || t == MINUS) {
        // Смотрим вперед, чтобы определить, константа ли это
        int saved_tok = nextToken();
        std::string saved_lex = cur_lex;
        int nextTok = peekToken();

        // Если следующий токен - константа, то унарную операцию обработает Prim()
        if (nextTok == CONST_DEC || nextTok == CONST_HEX) {
            pushBack(saved_tok, saved_lex);
        }
        else {
            // Если не константа, то обрабатываем как унарную операцию
            has_unary = true;
            unary_op = (saved_tok == PLUS) ? "+" : "-";
        }
    }
    DATA_TYPE left = Rel();

    // Обработка унарной операции (только для не-констант)
    if (has_unary) {
        if (!(left == TYPE_INT || left == TYPE_SHORT_INT || left == TYPE_LONG_INT || left == TYPE_LONG_LONG_INT)) {
            semError("Унарный '+'/'-' применим только к целым типам");
        }

        SemNode operand = popValue();
        SemNode result;

        if (unary_op == "-") {
            SemNode minusOne;
            minusOne.DataType = left;
            minusOne.hasValue = true;

            switch (left) {
            case TYPE_SHORT_INT: minusOne.Value.v_int16 = -1; break;
            case TYPE_INT: minusOne.Value.v_int32 = -1; break;
            case TYPE_LONG_INT: minusOne.Value.v_int32 = -1; break;
            case TYPE_LONG_LONG_INT: minusOne.Value.v_int64 = -1; break;
            default: break;
            }

            result = Tree::ExecuteArithmeticOp(operand, minusOne, "*", sc->getLineCol().first, sc->getLineCol().second);
        }
        else {
            result = operand;
        }

        pushValue(result);
        left = result.DataType;
    }

    t = peekToken();
    while (t == EQ || t == NEQ) {
        std::string op = (t == EQ) ? "==" : "!=";
        nextToken();

        DATA_TYPE right = Rel();

        SemNode right_val = popValue();
        SemNode left_val = popValue();

        bool is_left_int = (left == TYPE_INT || left == TYPE_SHORT_INT || left == TYPE_LONG_INT || left == TYPE_LONG_LONG_INT);
        bool is_right_int = (right == TYPE_INT || right == TYPE_SHORT_INT || right == TYPE_LONG_INT || right == TYPE_LONG_LONG_INT);

        if (is_left_int && is_right_int) {
            SemNode result = Tree::ExecuteComparisonOp(left_val, right_val, op, sc->getLineCol().first, sc->getLineCol().second);
            pushValue(result);
            left = TYPE_INT;
        }
        else {
            semError("Операнды для '=='/ '!=' должны быть одного типа (int)");
        }

        t = peekToken();
    }
    return left;
}

// Rel -> Add ( ('<' | '<=' | '>' | '>=') Add )*
DATA_TYPE Diagram::Rel() {
    DATA_TYPE left = Add();
    int t = peekToken();
    while (t == LT || t == LE || t == GT || t == GE) {
        std::string op;
        switch (t) {
        case LT: op = "<"; break;
        case LE: op = "<="; break;
        case GT: op = ">"; break;
        case GE: op = ">="; break;
        }
        
        nextToken();

        DATA_TYPE right = Add();

        SemNode right_val = popValue();
        SemNode left_val = popValue();

        bool is_left_int = (left == TYPE_INT || left == TYPE_SHORT_INT || left == TYPE_LONG_INT || left == TYPE_LONG_LONG_INT);
        bool is_right_int = (right == TYPE_INT || right == TYPE_SHORT_INT || right == TYPE_LONG_INT || right == TYPE_LONG_LONG_INT);

        if (!(is_left_int && is_right_int)) {
            semError("Операнды для '<, <=, >, >=' должны быть целыми (int / short / long / longlong)");
        }

        SemNode result = Tree::ExecuteComparisonOp(left_val, right_val, op, sc->getLineCol().first, sc->getLineCol().second);
        pushValue(result);
        left = TYPE_INT;

        t = peekToken();
    }
    return left;
}

// Add -> Mul ( ('+' | '-') Mul )*
DATA_TYPE Diagram::Add() {
    DATA_TYPE left = Mul();
    int t = peekToken();
    while (t == PLUS || t == MINUS) {
        std::string op = (t == PLUS) ? "+" : "-";

        nextToken();

        DATA_TYPE right = Mul();

        SemNode right_val = popValue();
        SemNode left_val = popValue();

        bool is_left_int = (left == TYPE_INT || left == TYPE_SHORT_INT || left == TYPE_LONG_INT || left == TYPE_LONG_LONG_INT);
        bool is_right_int = (right == TYPE_INT || right == TYPE_SHORT_INT || right == TYPE_LONG_INT || right == TYPE_LONG_LONG_INT);

        if (!(is_left_int && is_right_int)) {
            semError("Операнды для '+'/'-' должны быть целыми (int / short / long / longlong)");
        }

        SemNode result = Tree::ExecuteArithmeticOp(left_val, right_val, op, sc->getLineCol().first, sc->getLineCol().second);
        pushValue(result);
        left = result.DataType;

        t = peekToken();
    }
    return left;
}

// Mul -> Prim ( ('*' | '/' | '%') Prim )*
DATA_TYPE Diagram::Mul() {
    DATA_TYPE left = Prim();
    int t = peekToken();
    while (t == MULT || t == DIV || t == MOD) {
        std::string op;
        switch (t) {
        case MULT: op = "*"; break;
        case DIV: op = "/"; break;
        case MOD: op = "%"; break;
        }

        nextToken();

        DATA_TYPE right = Prim();

        SemNode right_val = popValue();
        SemNode left_val = popValue();

        bool is_left_int = (left == TYPE_INT || left == TYPE_SHORT_INT || left == TYPE_LONG_INT || left == TYPE_LONG_LONG_INT);
        bool is_right_int = (right == TYPE_INT || right == TYPE_SHORT_INT || right == TYPE_LONG_INT || right == TYPE_LONG_LONG_INT);

        if (!(is_left_int && is_right_int)) {
            semError("Операнды для '*', '/', '%' должны быть целыми (int / short / long / longlong)");
        }

        SemNode result = Tree::ExecuteArithmeticOp(left_val, right_val, op, sc->getLineCol().first, sc->getLineCol().second);
        pushValue(result);
        left = result.DataType;

        t = peekToken();
    }
    return left;
}

// Prim -> IDENT | Const | IDENT '[' Const ']' | '(' Expr ')'
DATA_TYPE Diagram::Prim() {
    int t = peekToken();

    // Сначала проверяем унарный минус для отрицательных констант
    if (t == MINUS) {
        nextToken();
        t = peekToken();
        if (t == CONST_DEC || t == CONST_HEX) {
            nextToken();
            
            DATA_TYPE const_type = TYPE_SHORT_INT;

            // Определяем, нужно ли автоматически повышать тип до long
            std::string value_str = "-" + cur_lex;
            try {
                long long val = std::stoll(value_str, nullptr, 0);

                // Если значение выходит за пределы short, автоматически делаем его int
                if (val > 32767 || val < -32768) {
                    const_type = TYPE_INT;
                }

                // Если значение выходит за пределы int, автоматически делаем его longlong
                if (val > 2147483647LL || val < -2147483648LL) {
                    const_type = TYPE_LONG_LONG_INT;
                }
            }
            catch (...) {
                // Оставляем как TYPE_INT в случае ошибки
            }

            SemNode const_node = evaluateConstant("-" + cur_lex, const_type);
            pushValue(const_node);
            return const_type;
        }
        else {
            // Если после минуса не константа, то это унарная операция над выражением
            // Помещаем токен обратно и обрабатываем как обычное выражение в скобках
            pushBack(MINUS, "-");
            // Обрабатываем как выражение в скобках
            nextToken();
            t = peekToken();
            if (t != LPAREN) {
                synError("Ожидалась константа или выражение в скобках после '-'");
            }
            nextToken();
            DATA_TYPE dt = Expr();
            t = peekToken();
            if (t != RPAREN) synError("Ожидался ')' после выражения");
            nextToken();
            return dt;
        }
    }

    if (t == CONST_DEC || t == CONST_HEX) {
        nextToken();
        
        DATA_TYPE const_type = TYPE_SHORT_INT;

        // Определяем, нужно ли автоматически повышать тип до long
        std::string value_str = cur_lex;
        try {
            long long val = std::stoll(value_str, nullptr, 0);

            // Если значение выходит за пределы short, автоматически делаем его int
            if (val > 32767 || val < -32768) {
                const_type = TYPE_INT;
            }

            // Если значение выходит за пределы int, автоматически делаем его longlong
            if (val > 2147483647LL || val < -2147483648LL) {
                const_type = TYPE_LONG_LONG_INT;
            }
        }
        catch (...) {
            // Оставляем как TYPE_INT в случае ошибки
        }

        SemNode const_node = evaluateConstant(cur_lex, const_type);
        pushValue(const_node);
        return const_type;
    }
    if (t == LPAREN) {
        nextToken();
        DATA_TYPE dt = Expr();
        t = peekToken();
        if (t != RPAREN) {
            synError("Ожидалась ')' после выражения");
        }
        nextToken();
        return dt;
    }
    if (t == IDENT) {
        nextToken();

        std::string name = cur_lex;
        std::pair<int, int> lc = sc->getLineCol();
        Tree* node = Tree::Cur->SemGetVar(name, lc.first, lc.second);

        t = peekToken();
        if (t == LBRACKET) {
            if (node->n->DataType != TYPE_ARRAY) {
                semError("Операция индексирования ([]) применима только к идентификаторам, объявленным как массив");
            }
            nextToken();
            t = peekToken();
            if (t == CONST_DEC || t == CONST_HEX) {
                t = nextToken();

                int index;
                int base = 10;
                if (t == CONST_HEX) {
                    base = 16;
                }
                try {
                    index = std::stoi(cur_lex, nullptr, base);
                }
                catch (const std::out_of_range& e) {
                    semError("Индекс при обращении к массиву не может превышать диапазон типа int");
                }

                if ((index < 0) || (index >= node->n->ArrElemCount)) {
                    semError("Индекс при обращении к массиву должен быть больше или равен 0 и меньше указанного при объявлении размера");
                }

                t = peekToken();
                if (t != RBRACKET) {
                    synError("Ожидалась ']' после константы");
                }
                nextToken();

                name = (name + "_" + std::to_string(index));
                node = Tree::Cur->SemGetVar(name, sc->getLineCol().first, sc->getLineCol().second);

                if (!node->n->hasValue) {
                    interpError("Использование неинициализированного элемента массива '" + name + "'");
                }

                SemNode value;
                value.DataType = node->n->DataType;
                value.hasValue = true;
                value.Value = node->n->Value;
                pushValue(value);

                return node->n->DataType;
            }
            else {
                synError("Ожидалась константа после '['");
            }
        }
        else {
            if (node->n->DataType == TYPE_ARRAY) {
                semError("Нельзя использовать массив целиком в качестве операнда");
            }

            if (!node->n->hasValue) {
                interpError("Использование неинициализированной переменной/именованной константы '" + name + "'");
            }

            SemNode value;
            value.DataType = node->n->DataType;
            value.hasValue = true;
            value.Value = node->n->Value;
            pushValue(value);

            return node->n->DataType;
        }
    }
    synError("Ожидалось первичное выражение (IDENT, константа или скобки)");
    return TYPE_INT;
}