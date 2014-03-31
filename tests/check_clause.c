// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include "../src/clause.h"

static const stdname_t FORWARD = 1;
static const stdname_t SONAR   = 2;

static const pred_t SF = -1;
#define D(i) ((pred_t) i)

static const var_t a = 12345;

#define NEW(expr)	({ typeof(expr) *ptr = malloc(sizeof(expr)); *ptr = expr; ptr; })

static const clause_t *c1(const varmap_t *map)
{
	ck_assert(!varmap_contains(map, 0));
	ck_assert(varmap_contains(map, a));
	ck_assert(!varmap_contains(map, a-1));
	ck_assert(!varmap_contains(map, a+1));
	const stdname_t n = varmap_lookup(map, a);
	const bool cond = n != FORWARD;
	const stdvec_t a_vec = stdvec_from_array(&n, 1);
	const stdvec_t empty_vec = stdvec_init();
	if (!cond) {
		return NULL;
	}
	clause_t *c = NEW(clause_init());
	clause_add(c, NEW(literal_init(&empty_vec, false, SF, &a_vec)));
	clause_add(c, NEW(literal_init(&empty_vec, true, D(0), &empty_vec)));
	clause_add(c, NEW(literal_init(&empty_vec, true, D(1), &empty_vec)));
	return c;
}

static const clause_t *c2(const varmap_t *map)
{
	const stdname_t n = varmap_lookup(map, a);
	const bool cond = n != FORWARD && n != SONAR;
	const stdvec_t a_vec = stdvec_from_array(&n, 1);
	const stdvec_t empty_vec = stdvec_init();
	if (!cond) {
		return NULL;
	}
	return NEW(clause_singleton(NEW(literal_init(&empty_vec, false, SF, &a_vec))));
}

static const clause_t *c3(const varmap_t *map)
{
	const stdname_t n = varmap_lookup(map, a);
	const bool cond = n == FORWARD;
	const stdvec_t a_vec = stdvec_from_array(&n, 1);
	const stdvec_t empty_vec = stdvec_init();
	if (!cond) {
		return NULL;
	}
	return NEW(clause_singleton(NEW(literal_init(&empty_vec, true, SF, &a_vec))));
}

static const clause_t *c4(const varmap_t *map)
{
	const stdname_t n = varmap_lookup(map, a);
	const bool cond = n == SONAR;
	const stdvec_t a_vec = stdvec_from_array(&n, 1);
	const stdvec_t empty_vec = stdvec_init();
	if (!cond) {
		return NULL;
	}
	clause_t *c = NEW(clause_init());
	clause_add(c, NEW(literal_init(&empty_vec, false, D(0), &empty_vec)));
	clause_add(c, NEW(literal_init(&empty_vec, true, SF, &a_vec)));
	return c;
}

static const clause_t *c5(const varmap_t *map)
{
	const stdname_t n = varmap_lookup(map, a);
	const bool cond = n == SONAR;
	const stdvec_t a_vec = stdvec_from_array(&n, 1);
	const stdvec_t empty_vec = stdvec_init();
	if (!cond) {
		return NULL;
	}
	clause_t *c = NEW(clause_init());
	clause_add(c, NEW(literal_init(&empty_vec, false, D(1), &empty_vec)));
	clause_add(c, NEW(literal_init(&empty_vec, true, SF, &a_vec)));
	return c;
}

static const clause_t *gen_c6(int i, const varmap_t *map)
{
	const stdname_t n = varmap_lookup(map, a);
	const bool cond = n == FORWARD;
	const stdvec_t a_vec = stdvec_from_array(&n, 1);
	const stdvec_t empty_vec = stdvec_init();
	if (!cond) {
		return NULL;
	}
	clause_t *c = NEW(clause_init());
	clause_add(c, NEW(literal_init(&empty_vec, false, D(i+1), &empty_vec)));
	clause_add(c, NEW(literal_init(&a_vec, true, D(i), &empty_vec)));
	return c;
}

static const clause_t *gen_c7(int i, const varmap_t *map)
{
	const stdname_t n = varmap_lookup(map, a);
	const bool cond = n != FORWARD;
	const stdvec_t a_vec = stdvec_from_array(&n, 1);
	const stdvec_t empty_vec = stdvec_init();
	if (!cond) {
		return NULL;
	}
	clause_t *c = NEW(clause_init());
	clause_add(c, NEW(literal_init(&empty_vec, false, D(i), &empty_vec)));
	clause_add(c, NEW(literal_init(&a_vec, true, D(i), &empty_vec)));
	return c;
}

static const clause_t *gen_c8(int i, const varmap_t *map)
{
	const stdname_t n = varmap_lookup(map, a);
	const bool cond = n != FORWARD;
	const stdvec_t a_vec = stdvec_from_array(&n, 1);
	const stdvec_t empty_vec = stdvec_init();
	if (!cond) {
		return NULL;
	}
	clause_t *c = NEW(clause_init());
	clause_add(c, NEW(literal_init(&a_vec, false, D(i), &empty_vec)));
	clause_add(c, NEW(literal_init(&empty_vec, true, D(i), &empty_vec)));
	return c;
}

static const clause_t *gen_c9(int i, const varmap_t *map)
{
	const stdname_t n = varmap_lookup(map, a);
	const bool cond = n == FORWARD;
	const stdvec_t a_vec = stdvec_from_array(&n, 1);
	const stdvec_t empty_vec = stdvec_init();
	if (!cond) {
		return NULL;
	}
	clause_t *c = NEW(clause_init());
	clause_add(c, NEW(literal_init(&a_vec, true, D(i), &empty_vec)));
	clause_add(c, NEW(literal_init(&empty_vec, false, D(i+1), &empty_vec)));
	return c;
}

// instantiate the generic clauses for some i > 0

#define MAKE_UNIV_CLAUSES(i)\
	static const clause_t *c6##i(const varmap_t *map) { return gen_c6(i, map); }\
	static const clause_t *c7##i(const varmap_t *map) { return gen_c7(i, map); }\
	static const clause_t *c8##i(const varmap_t *map) { return gen_c8(i, map); }\
	static const clause_t *c9##i(const varmap_t *map) { return gen_c9(i, map); }
#define USE_UNIV_CLAUSES(i)\
	{ .names = names, .vars = vars, .univ_clause = &c6##i },\
	{ .names = names, .vars = vars, .univ_clause = &c7##i },\
	{ .names = names, .vars = vars, .univ_clause = &c8##i },\
	{ .names = names, .vars = vars, .univ_clause = &c9##i }

MAKE_UNIV_CLAUSES(1);
MAKE_UNIV_CLAUSES(2);
MAKE_UNIV_CLAUSES(3);
//MAKE_UNIV_CLAUSES(4);
//MAKE_UNIV_CLAUSES(5);
//MAKE_UNIV_CLAUSES(6);
//MAKE_UNIV_CLAUSES(7);
//MAKE_UNIV_CLAUSES(8);
//MAKE_UNIV_CLAUSES(9);

static void print_stdname(stdname_t n)
{
	if (n == FORWARD)	printf("f");
	else if (n == SONAR)	printf("s");
	else			printf("#%ld", n);
}

static void print_pred(pred_t p)
{
	if (p == SF)		printf("SF");
	else			printf("d%d", p);
}

static void print_literal(const literal_t *l)
{
	if (stdvec_size(literal_z(l)) > 0) {
		printf("[");
		for (int i = 0; i < stdvec_size(literal_z(l)); ++i) {
			if (i > 0) printf(",");
			print_stdname(stdvec_get(literal_z(l), i));
		}
		printf("]");
	}
	if (!literal_sign(l)) {
		printf("~");
	}
	print_pred(literal_pred(l));
	if (stdvec_size(literal_args(l)) > 0) {
		printf("(");
		for (int i = 0; i < stdvec_size(literal_args(l)); ++i) {
			if (i > 0) printf(",");
			print_stdname(stdvec_get(literal_args(l), i));
		}
		printf(")");
	}
}

static void print_clause(const clause_t *c)
{
	printf("{ ");
	for (int i = 0; i < clause_size(c); ++i) {
		if (i > 0) printf(", ");
		print_literal(clause_get(c, i));
	}
	printf(" }\n");
}

static void print_setup(const setup_t *setup)
{
	for (int i = 0; i < setup_size(setup); ++i) {
		print_clause(setup_get(setup, i));
	}
}

START_TEST(test_clause)
{
	stdset_t names = stdset_init();
	stdset_add(&names, FORWARD);
	stdset_add(&names, SONAR);
	const varset_t vars = varset_singleton(a);
	const univ_clause_t univ_clauses[] = {
		{ .names = names, .vars = vars, .univ_clause = &c1 },
		{ .names = names, .vars = vars, .univ_clause = &c2 },
		{ .names = names, .vars = vars, .univ_clause = &c3 },
		{ .names = names, .vars = vars, .univ_clause = &c4 },
		{ .names = names, .vars = vars, .univ_clause = &c5 },
		USE_UNIV_CLAUSES(1),
		USE_UNIV_CLAUSES(2),
		USE_UNIV_CLAUSES(3),
		//USE_UNIV_CLAUSES(4),
		//USE_UNIV_CLAUSES(5),
		//USE_UNIV_CLAUSES(6),
		//USE_UNIV_CLAUSES(7),
		//USE_UNIV_CLAUSES(8),
		//USE_UNIV_CLAUSES(9),
	};
	const stdvec_t z_forward = stdvec_from_array(&FORWARD, 1);
	const stdvec_t z_sonar = stdvec_from_array(&SONAR, 2);
	stdvecset_t query_zs = stdvecset_init();
	stdvecset_add(&query_zs, &z_forward);
	stdvecset_add(&query_zs, &z_sonar);
	stdset_t query_ns = stdset_init();
	stdset_add(&query_ns, FORWARD);
	stdset_add(&query_ns, SONAR);
	const int n_univ_clauses = sizeof(univ_clauses) / sizeof(univ_clauses[0]);
	setup_t setup = ground_clauses(univ_clauses, n_univ_clauses, &query_zs, &query_ns, 0);
	// this one works for this example, but is not sufficient in general as
	// the static clauses may have universal quantifiers as well (see TODO
	// in clause.h for what to do)
	({
		const stdvec_t empty_vec = stdvec_init();
		setup_add(&setup, NEW(clause_singleton(NEW(literal_init(&empty_vec, false, D(0), &empty_vec)))));
		setup_add(&setup, NEW(clause_singleton(NEW(literal_init(&empty_vec, false, D(1), &empty_vec)))));
		clause_t *c = NEW(clause_init());
		clause_add(c, NEW(literal_init(&empty_vec, true, D(2), &empty_vec)));
		clause_add(c, NEW(literal_init(&empty_vec, true, D(3), &empty_vec)));
		setup_add(&setup, c);
	});
	print_setup(&setup);
}
END_TEST

Suite *clause_suite(void)
{
  Suite *s = suite_create("Vector");
  TCase *tc_core = tcase_create("Core");
  tcase_add_test(tc_core, test_clause);
  suite_add_tcase(s, tc_core);
  return s;
}

int main(void)
{
    int number_failed;
    Suite *s = clause_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

