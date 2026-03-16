#pragma once

enum DATA_TYPE {
	TYPE_INT = 1, // Целый
	TYPE_SHORT_INT, // Целый short
	TYPE_LONG_INT, // Целый long
	TYPE_LONG_LONG_INT, // Целый long long
	TYPE_ARRAY, // Массив
	TYPE_TYPEDEF_NAME, // Метка типа
	TYPE_SCOPE, // Составной оператор
	TYPE_UNDEFINED // Неопределённый
};