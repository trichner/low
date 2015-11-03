/* Copyright (c) 2015 Fabian Schuiki */
#pragma once
#include "ast.h"

typedef struct rule rule_t;
typedef struct variant variant_t;
typedef struct token token_t;

typedef void(*reduce_fn_t)(token_t *, const token_t*, void*);

struct variant {
	const rule_t **elements;
	reduce_fn_t reducer;
	const char *reducer_name;
	void *reducer_arg;
};

struct rule {
	const char *name;
	const variant_t *variants;
};

struct token {
	int id;
	const char *first;
	const char *last;
	union {
		func_def_t *func_def;
	};
};

extern const rule_t grammar_root;
