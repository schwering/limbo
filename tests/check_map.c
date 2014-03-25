// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include <check.h>
#include <stdlib.h>
#include "../src/map.h"

#define ABS(x) ((x) < 0 ? -(x) : (x))

static int compar_int(const kv_t *p, const kv_t *q)
{
    return (long int) p->key - (long int) q->key;
}

START_TEST(test_map_add)
{
    map_t map;
    map_init(&map, compar_int);
    for (int i = -11; i < 10; i += 4) {
        ck_assert(!map_contains(&map, (void *) (long int) ABS(i)));
        ck_assert(map_add(&map, (void *) (long int) ABS(i), (void *) (long int) ABS(i)));
        ck_assert_int_eq((long int) map_lookup(&map, (void *) (long int) ABS(i)), (long int) ABS(i));
        ck_assert(map_contains(&map, (void *) (long int) ABS(i)));
        ck_assert(!map_add(&map, (void *) (long int) ABS(i), (void *) (long int) ABS(i)));
        ck_assert_int_eq((long int) map_add_replace(&map, (void *) (long int) ABS(i), (void *) (long int) (2*ABS(i))), (long int) ABS(i));
        ck_assert_int_eq((long int) map_lookup(&map, (void *) (long int) ABS(i)), (long int) (2*ABS(i)));
    }
    ck_assert_int_eq(map_size(&map), 6);
    for (int i = 0; i < 10; i++) {
        if (i == 1 || i == 3 || i == 5 || i == 7 || i == 9) {
            ck_assert_int_eq((long int) map_remove(&map, (void *) (long int) ABS(i)), (long int) (2*ABS(i)));
        } else {
            ck_assert_int_eq(map_remove(&map, (void *) (long int) ABS(i)), NULL);
        }
    }
    ck_assert_int_eq(map_size(&map), 1);
    map_clear(&map);
    ck_assert_int_eq(map_size(&map), 0);
    map_free(&map);
}
END_TEST

Suite *map_suite(void)
{
  Suite *s = suite_create("Map");
  TCase *tc_core = tcase_create("Core");
  tcase_add_test(tc_core, test_map_add);
  suite_add_tcase(s, tc_core);
  return s;
}

int main(void)
{
    int number_failed;
    Suite *s = map_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
