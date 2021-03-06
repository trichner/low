/* Copyright (c) 2015 Fabian Schuiki */
#include "array.h"
#include "grammar.h"
#include "lexer.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>


typedef struct lead lead_t;
typedef struct rule_action rule_action_t;
typedef struct rule_chain rule_chain_t;
typedef struct state state_t;
typedef struct symbol symbol_t;
typedef struct token_action token_action_t;
typedef struct token_set token_set_t;


struct token_set {
	unsigned bits[(MAX_TOKENS+31)/32];
};

struct state {
	array_t leads;
	array_t token_actions;
	array_t rule_actions;
};

struct symbol {
	int token;
	const rule_t *rule;
};

struct lead {
	const rule_t *rule;
	const variant_t *variant;
	unsigned position;
	token_set_t terminals;
};

struct rule_chain {
	const rule_chain_t *prev;
	const rule_t *rule;
};

struct token_action {
	token_set_t tokens;
	const rule_t *rule;
	const variant_t *variant;
	int state;
};

struct rule_action {
	const rule_t *rule;
	int state;
};


static void
token_set_init(token_set_t *set) {
	bzero(set->bits, sizeof(set->bits));
}

// static int
// token_set_contains(const token_set_t *set, int token) {
// 	return set->bits[token/32] & (1 << (token%32));
// }

static void
token_set_insert(token_set_t *set, int token) {
	set->bits[token/32] |= (1 << (token%32));
}

static void
token_set_merge(token_set_t *set, const token_set_t *other) {
	unsigned i;
	for (i = 0; i < (MAX_TOKENS+31)/32; ++i)
		set->bits[i] |= other->bits[i];
}

// static void
// token_set_remove(token_set_t *set, int token) {
// 	set->bits[token/32] &= ~(1 << (token%32));
// }

static int
equal_token_sets (const token_set_t *s1, const token_set_t *s2) {
	unsigned i;
	for (i = 0; i < (MAX_TOKENS+31)/32; ++i)
		if (s1->bits[i] != s2->bits[i])
			return 0;
	return 1;
}


static symbol_t
variant_get_symbol(const variant_t *self, unsigned index) {
	assert(self);
	const rule_t *e = self->elements[index];
	if ((size_t)e < MAX_TOKENS) {
		return (symbol_t){(int)(size_t)e, 0};
	} else {
		return (symbol_t){0, e};
	}
}


static int
equal_symbols (const symbol_t *t1, const symbol_t *t2) {
	return t1->token == t2->token && t1->rule == t2->rule;
}


static int
equal_leads (const lead_t *l1, const lead_t *l2) {
	return l1->rule == l2->rule &&
	       l1->variant == l2->variant &&
	       l1->position == l2->position &&
	       equal_token_sets(&l1->terminals, &l2->terminals);
}


static int
compare_token_sets (const token_set_t *s1, const token_set_t *s2) {
	unsigned i;
	for (i = 0; i < (MAX_TOKENS+31)/32; ++i) {
		if (s1->bits[i] < s2->bits[i]) return -1;
		if (s1->bits[i] > s2->bits[i]) return  1;
	}
	return 0;
}


static int
compare_leads (const void *p1, const void *p2) {
	const lead_t *l1 = p1;
	const lead_t *l2 = p2;
	if (l1->rule < l2->rule) return -1;
	if (l1->rule > l2->rule) return  1;
	if (l1->variant < l2->variant) return -1;
	if (l1->variant > l2->variant) return  1;
	if (l1->position < l2->position) return -1;
	if (l1->position > l2->position) return  1;
	return compare_token_sets(&l1->terminals, &l2->terminals);
}


static int
equal_set_of_leads (const array_t *a1, const array_t *a2) {
	assert(a1 && a2);
	if (a1->size != a2->size)
		return 0;

	unsigned i;
	for (i = 0; i < a1->size; ++i)
		if (!equal_leads(array_get(a1,i), array_get(a2,i)))
			return 0;

	return 1;
}


// static void
// print_token_set (const token_set_t *set) {
// 	printf("{");
// 	int first = 1;
// 	unsigned i,n;
// 	for (i = 0; i < (MAX_TOKENS+31)/32; ++i) {
// 		if (set->bits[i] == 0)
// 			continue;
// 		for (n = 0; n < 32; ++n) {
// 			if (set->bits[i] & (1 << n)) {
// 				if (!first)
// 					printf(",");
// 				else
// 					first = 0;
// 				printf("%d", i*32+n);
// 			}
// 		}
// 	}
// 	printf("}");
// }


// static void
// print_symbol (const symbol_t *symbol) {
// 	if (symbol->token && !symbol->rule)
// 		printf("%d", symbol->token);
// 	else if (symbol->rule && !symbol->token)
// 		printf("%s", symbol->rule->name);
// 	else if (!symbol->rule && !symbol->token)
// 		printf("<nil>");
// 	else
// 		printf("{%d, %s}", symbol->token, symbol->rule ? symbol->rule->name : "<none>");
// }


// static void
// print_lead (const lead_t *lead) {
// 	printf("{%s ->", lead->rule->name);
// 	unsigned i;
// 	const rule_t **e;
// 	for (e = lead->variant->elements, i = 0; *e; ++e, ++i) {
// 		const rule_t *element = *e;
// 		if (lead->position == i)
// 			printf(" .");
// 		if ((size_t)element < MAX_TOKENS)
// 			printf(" %d", (int)(size_t)element);
// 		else
// 			printf(" %s", element->name);
// 	}
// 	if (lead->position == i)
// 		printf(" .");
// 	printf(", ");
// 	print_token_set(&lead->terminals);
// 	printf("}");
// }


// static void
// print_token_action (const token_action_t *action) {
// 	printf("{");
// 	print_token_set(&action->tokens);

// 	if (action->rule && action->variant) {
// 		printf(": reduce %s %p}", action->rule->name, action->variant);
// 	} else {
// 		printf(": shift %d}", action->state);
// 	}
// }

// static void
// print_rule_action (const rule_action_t *action) {
// 	printf("{%s: goto %d}", action->rule->name, action->state);
// }


/// Gathers a list of terminals that may appear as the first symbol of the given
/// token.
void
gather_first (token_set_t *first, const symbol_t *symbol, const rule_chain_t *chain) {
	assert(first);
	assert(symbol);

	if (symbol->rule) {
		const rule_chain_t *check;
		for (check = chain; check; check = check->prev)
			if (check->rule == symbol->rule)
				return;
		rule_chain_t new_chain = {chain, symbol->rule};

		const variant_t *variant;
		for (variant = symbol->rule->variants; variant->elements; ++variant) {
			symbol_t t = variant_get_symbol(variant, 0);
			gather_first(first, &t, &new_chain);
		}
	} else {
		assert(symbol->token && "no empty rules allowed");
		token_set_insert(first, symbol->token);
	}
}


/// Calculates the closure of the given set of leads by expanding all leads that
/// have a non-terminal as the next symbol.
void
gather_leads (array_t *leads, unsigned index, unsigned length) {
	assert(leads);
	assert(index < leads->size);
	assert(index+length <= leads->size);

	unsigned first_new = leads->size;
	unsigned i;
	for (i = index; i < index+length; ++i) {
		const lead_t *lead = array_get(leads, i);

		symbol_t tc = variant_get_symbol(lead->variant, lead->position);
		if (!tc.rule)
			continue;
		symbol_t tn = variant_get_symbol(lead->variant, lead->position+1);

		token_set_t follow;
		token_set_init(&follow);
		if (tn.token == 0 && tn.rule == 0) {
			token_set_merge(&follow, &lead->terminals);
		} else {
			gather_first(&follow, &tn, 0);
		}

		const variant_t *variant;
		for (variant = tc.rule->variants; variant->elements; ++variant) {

			lead_t nl = {
				tc.rule,
				variant,
				0,
				follow
			};

			unsigned m;
			for (m = 0; m < leads->size; ++m) {
				const lead_t *lead = array_get(leads, m);
				if (equal_leads(lead, &nl))
					break;
			}

			if (m == leads->size) {
				array_add(leads, &nl);
			}
		}
	}

	if (first_new < leads->size)
		gather_leads(leads, first_new, leads->size - first_new);
}


static void
consolidate_leads (array_t *leads) {
	qsort(leads->items, leads->size, sizeof(lead_t), compare_leads);

	if (leads->size >= 2) {
		lead_t *lo = leads->items;
		lead_t *li = lo+1;
		lead_t *end = lo+leads->size;

		for (; li != end; ++li) {
			if (li->rule == lo->rule && li->variant == lo->variant && li->position == lo->position) {
				token_set_merge(&lo->terminals, &li->terminals);
			} else {
				++lo;
				memcpy(lo, li, sizeof(lead_t));
			}
		}
		++lo;

		unsigned new_size = lo - (lead_t*)leads->items;
		array_resize(leads, new_size);
	}
}


static void
gather_states (array_t *states, unsigned index, unsigned length) {
	assert(states);
	assert(index < states->size);
	assert(index+length <= states->size);

	unsigned first_new = states->size;

	unsigned i,n;
	for (i = index; i < index+length; ++i) {
		state_t state = *(state_t*)array_get(states,i); // make a copy since we're modifying the states array
		// printf("gather state %d (of %d, %.0f%%):\n", i, states->size, (float)i/states->size * 100.f);
		// for (n = 0; n < state.leads.size; ++n) {
		// 	printf("  ");
		// 	print_lead(array_get(&state.leads, n));
		// 	printf("\n");
		// }

		char handled[state.leads.size];
		bzero(handled, sizeof(handled));

		for (n = 0; n < state.leads.size; ++n) {
			if (handled[n])
				continue;

			array_t shift_leads;
			array_t reduce_leads;
			array_init(&shift_leads, sizeof(lead_t));
			array_init(&reduce_leads, sizeof(lead_t));

			lead_t *base = array_get(&state.leads, n);
			symbol_t base_symbol = variant_get_symbol(base->variant, base->position);

			unsigned k;
			for (k = n; k < state.leads.size; ++k) {
				lead_t *lead = array_get(&state.leads, k);
				symbol_t lead_symbol = variant_get_symbol(lead->variant, lead->position);

				if (!handled[k] && equal_symbols(&base_symbol, &lead_symbol)) {
					handled[k] = 1;

					if (lead_symbol.token == 0 && lead_symbol.rule == 0) {
						// reduce
						// printf("  ");
						// print_token_set(&lead->terminals);
						// printf(" -> reduce %s\n", lead->rule->name);

						token_action_t *action = array_add(&((state_t*)array_get(states,i))->token_actions, 0);
						bzero(action, sizeof(*action));
						action->rule = lead->rule;
						action->variant = lead->variant;
						action->tokens = lead->terminals;

						array_add(&reduce_leads, lead);
					} else {
						lead_t nl = *lead;
						++nl.position;
						array_add(&shift_leads, &nl);
					}
				}
			}

			assert((shift_leads.size > 0 || reduce_leads.size > 0) && "syntax error");
			assert((shift_leads.size > 0) != (reduce_leads.size > 0) && "shift-reduce conflict");
			assert(reduce_leads.size <= 1 && "reduce-reduce conflict");

			if (shift_leads.size > 0) {
				gather_leads(&shift_leads, 0, shift_leads.size);
				consolidate_leads(&shift_leads);

				int state_index = -1;
				for (k = 0; k < states->size && state_index == -1; ++k) {
					state_t *s = array_get(states, k);
					if (equal_set_of_leads(&shift_leads, &s->leads))
						state_index = k;
				}

				if (state_index == -1) {
					state_index = states->size;

					state_t new_state;
					array_init(&new_state.token_actions, sizeof(token_action_t));
					array_init(&new_state.rule_actions, sizeof(rule_action_t));
					new_state.leads = shift_leads;
					array_add(states, &new_state);
				} else {
					// printf("  discarding redundant state\n");
					array_dispose(&shift_leads);
				}

				// printf("  [");
				// print_symbol(&base_symbol);
				// if (base_symbol.rule == 0)
				// 	printf("] shift %d\n", state_index);
				// else
				// 	printf("] goto %d\n", state_index);

				if (base_symbol.rule) {
					rule_action_t *action = array_add(&((state_t*)array_get(states,i))->rule_actions, 0);
					bzero(action, sizeof(*action));
					action->rule = base_symbol.rule;
					action->state = state_index;
				} else {
					token_action_t *action = array_add(&((state_t*)array_get(states,i))->token_actions, 0);
					bzero(action, sizeof(*action));
					token_set_insert(&action->tokens, base_symbol.token);
					action->state = state_index;
				}
			}

			array_dispose(&reduce_leads);
		}
	}

	if (first_new < states->size)
		gather_states(states, first_new, states->size-first_new);
}


static unsigned
assign_rule_id(array_t *rule_ids, const rule_t *rule) {
	unsigned w;
	for (w = 0; w < rule_ids->size; ++w)
		if (*(rule_t**)array_get(rule_ids,w) == rule)
			return w+MAX_TOKENS;

	array_add(rule_ids, &rule);
	return rule_ids->size+MAX_TOKENS-1;
}


int
main (int argc, char** argv) {
	unsigned i, n;

	lead_t root_lead = {&grammar_root, grammar_root.variants, 0};
	token_set_init(&root_lead.terminals);
	token_set_insert(&root_lead.terminals, TKN_EOF);

	state_t initial;
	array_init(&initial.leads, sizeof(lead_t));
	array_init(&initial.token_actions, sizeof(token_action_t));
	array_init(&initial.rule_actions, sizeof(rule_action_t));
	array_add(&initial.leads, &root_lead);
	gather_leads(&initial.leads, 0, 1);
	consolidate_leads(&initial.leads);

	array_t states;
	array_init(&states, sizeof(state_t));
	array_add(&states, &initial);

	gather_states(&states, 0, 1);


	printf("/* States of the LR(1) parser. Automatically generated. */\n");
	printf("#include \"parser_states.h\"\n");
	printf("\n");


	array_t reducers;
	array_init(&reducers, sizeof(variant_t*));

	for (i = 0; i < states.size; ++i) {
		state_t *state = array_get(&states, i);
		for (n = 0; n < state->token_actions.size; ++n) {
			token_action_t *action = array_get(&state->token_actions, n);
			if (action->rule && action->rule != &grammar_root && action->variant->reducer) {
				unsigned k;
				for (k = 0; k < reducers.size; ++k)
					if (*(variant_t**)array_get(&reducers, k) == action->variant)
						break;
				if (k == reducers.size) {
					printf("extern void %s(token_t *, const token_t*, int);\n", action->variant->reducer);
					array_add(&reducers, &action->variant);
				}
			}
		}
	}

	array_dispose(&reducers);
	printf("\n");


	array_t rule_ids;
	array_init(&rule_ids, sizeof(rule_t*));

	printf("const parser_state_t parser_states[] = {\n");
	for (i = 0; i < states.size; ++i) {
		state_t *state = array_get(&states, i);
		printf("\t/* state %d */ {\n", i);

		unsigned num_actions = 0;
		unsigned num_gotos = 0;

		printf("\t\t(const parser_action_t[]){\n");
		for (n = 0; n < state->token_actions.size; ++n) {
			token_action_t *action = array_get(&state->token_actions, n);
			unsigned u,v;
			for (u = 0; u < (MAX_TOKENS+31)/32; ++u) {
				for (v = 0; v < 32; ++v) {
					if (action->tokens.bits[u] & (1 << v)){
						if (action->rule) {
							unsigned length;
							for (length = 0; action->variant->elements[length]; ++length);
							if (action->rule == &grammar_root) {
								printf("\t\t\t{%d, %d, 0, 0, 0}, /* accept */\n", u*32+v, -length);
							} else {
								printf("\t\t\t{%d, %d, %s, %d, %d}, /* reduce %s */\n",
									u*32+v,
									-length,
									action->variant->reducer ? action->variant->reducer : "0",
									action->variant->reducer_tag,
									assign_rule_id(&rule_ids, action->rule),
									action->rule->name
								);
							}
						} else {
							printf("\t\t\t{%d, %d, 0, 0, 0}, /* shift & goto %d */\n", u*32+v, action->state, action->state);
						}
						++num_actions;
					}
				}
			}
		}
		printf("\t\t},\n");

		printf("\t\t(const parser_goto_t[]){\n");
		for (n = 0; n < state->rule_actions.size; ++n) {
			rule_action_t *action = array_get(&state->rule_actions, n);
			printf("\t\t\t{%d, %d}, /* [%s] goto %d */\n", assign_rule_id(&rule_ids, action->rule), action->state, action->rule->name, action->state);
			++num_gotos;
		}
		printf("\t\t},\n");

		printf("\t\t%d, %d,\n", num_actions, num_gotos);
		printf("\t},\n");
	}
	printf("};\n\n");

	printf("const char *parser_token_names[] = {\n");
	for (i = 0; i < MAX_TOKENS; ++i)
		printf("\t0, /* token */\n");
	for (i = 0; i < rule_ids.size; ++i)
		printf("\t\"%s\", /* rule */\n", ((rule_t**)rule_ids.items)[i]->name);
	printf("};\n");

	array_dispose(&rule_ids);


	for (i = 0; i < states.size; ++i) {
		state_t *state = array_get(&states, i);
		array_dispose(&state->token_actions);
		array_dispose(&state->rule_actions);
		array_dispose(&state->leads);
	}
	array_dispose(&states);

	return 0;
}
