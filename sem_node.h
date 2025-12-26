#pragma once
#include <string>
#include "data_type.h"

struct SemNode {
	std::string id; // Имя идентификатора
	DATA_TYPE DataType; // Тип объекта

	bool hasValue = false; // Есть ли известное константное значение

	union {
		int16_t v_int16;
		int32_t v_int32;
		int64_t v_int64;
	} Value; // Само значение

	int FlagConst; // Признак константы
	DATA_TYPE BasicType; // Базовый тип (тип элемента массива или тип для которого создаётся метка)
	int ArrElemCount; // Размерность массива (для метки типа для массива и для переменной-массива)
	int index; // Для элемента массива, чтобы знать его индекс в массиве
	int line; // Строка объявления (для сообщений об ошибках)
	int col; // Позиция в строке (для сообщений об ошибках)
};