#pragma once

// Идентификаторы
#define IDENT                10

// Ключевые слова
#define KW_INT               1
#define KW_SHORT             2
#define KW_LONG              3
#define KW_WHILE             4
#define KW_CONST             5
#define KW_TYPEDEF           6
#define KW_LONGLONG          7
#define KW_MAIN              8

// Константы
#define CONST_DEC            20   // Константа (10 с/с)
#define CONST_HEX            21   // Константа (16 с/с)

// Специальные знаки / разделители
#define SEMI                 30   // ;
#define COMMA                31   // ,
#define LPAREN               32   // (
#define RPAREN               33   // )
#define LBRACE               34   // {
#define RBRACE               35   // }
#define LBRACKET             36   // [
#define RBRACKET             37   // ]

// Операторы
#define EQ                   40   // ==
#define NEQ                  41   // !=
#define LE                   42   // <=
#define GE                   43   // >=
#define LT                   44   // <
#define GT                   45   // >
#define ASSIGN               46   // =

#define PLUS                 47   // +
#define MINUS                48   // -
#define MULT                 49   // *
#define DIV                  50   // /
#define MOD                  51   // %

// Прочие
#define T_END                100  // Конец исходного модуля (EOF)
#define T_ERR                200  // Ошибочный символ