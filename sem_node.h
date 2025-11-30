#pragma once
#include <string>
#include "data_type.h"

struct SemNode {
	std::string id; // Имя идентификатора
	DATA_TYPE DataType; // Тип объекта
	char* Data; // Ссылка на значение
	int FlagConst; // Признак константы
	DATA_TYPE BasicType; // Базовый тип (тип элемента массива или тип для которого создаётся метка)
	int ArrElemCount; // Размерность массива (для метки типа для массива и для переменной-массива)
	int line; // Строка объявления (для сообщений об ошибках)
	int col; // Позиция в строке (для сообщений об ошибках)
};