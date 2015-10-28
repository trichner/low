/* Copyright (c) 2015 Fabian Schuiki */
#pragma once
#include <stdlib.h>

enum {
	TKN_INVALID = 0,
	TKN_SOF = 1,
	TKN_EOF = 2,

	TKN_ADD,
	TKN_ADD_ASSIGN,
	TKN_AND,
	TKN_AND_ASSIGN,
	TKN_ASSIGN,
	TKN_ATOMIC,
	TKN_BITWISE_AND,
	TKN_BITWISE_NOT,
	TKN_BITWISE_OR,
	TKN_BITWISE_XOR,
	TKN_BREAK,
	TKN_CASE,
	TKN_COLON,
	TKN_COMMA,
	TKN_CONST,
	TKN_CONTINUE,
	TKN_DEC,
	TKN_DEFAULT,
	TKN_DIV,
	TKN_DIV_ASSIGN,
	TKN_DO,
	TKN_ELLIPSIS,
	TKN_ELSE,
	TKN_ENUM,
	TKN_EQ,
	TKN_EXTERN,
	TKN_FOR,
	TKN_GE,
	TKN_GOTO,
	TKN_GT,
	TKN_IDENT,
	TKN_IF,
	TKN_IMPORT,
	TKN_INC,
	TKN_INLINE,
	TKN_LBRACE,
	TKN_LBRACK,
	TKN_LE,
	TKN_LEFT,
	TKN_LEFT_ASSIGN,
	TKN_LPAREN,
	TKN_LT,
	TKN_MOD,
	TKN_MOD_ASSIGN,
	TKN_MUL,
	TKN_MUL_ASSIGN,
	TKN_NE,
	TKN_NOT,
	TKN_NUMBER_LITERAL,
	TKN_OR,
	TKN_OR_ASSIGN,
	TKN_PERIOD,
	TKN_QUESTION,
	TKN_RBRACE,
	TKN_RBRACK,
	TKN_RETURN,
	TKN_RIGHT,
	TKN_RIGHT_ASSIGN,
	TKN_RPAREN,
	TKN_SEMICOLON,
	TKN_SIZEOF,
	TKN_STATIC,
	TKN_STRING_LITERAL,
	TKN_STRUCT,
	TKN_SUB,
	TKN_SUB_ASSIGN,
	TKN_SWITCH,
	TKN_TYPEDEF,
	TKN_UNION,
	TKN_VOID,
	TKN_VOLATILE,
	TKN_WHILE,
	TKN_XOR_ASSIGN,
	TKN_YIELD,

	MAX_TOKENS
};

typedef struct {
	const char *base;
	const char *ptr;
	const char *end;
	int token;
} lexer_t;

void lexer_init(lexer_t *self, const char *ptr, size_t len);
void lexer_next(lexer_t *self);
