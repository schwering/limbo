// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include <check.h>
#include <stdlib.h>
#include "../src/map.h"

#define ABS(x) ((x) < 0 ? -(x) : (x))

static int compar_long_int(long int p, long int q)
{
    return p - q;
}

MAP_DECL(iimap, long int, long int);
MAP_IMPL(iimap, long int, long int, compar_long_int);

START_TEST(test_map_add)
{
    iimap_t map = iimap_init();
    for (int i = -11; i < 10; i += 4) {
        ck_assert(!iimap_contains(&map, ABS(i)));
        ck_assert(iimap_add(&map, ABS(i), ABS(i)));
        ck_assert_int_eq(iimap_lookup(&map, ABS(i)), ABS(i));
        ck_assert(iimap_contains(&map, ABS(i)));
        ck_assert(!iimap_add(&map, ABS(i), ABS(i)));
        ck_assert_int_eq(iimap_add_replace(&map, ABS(i), (2*ABS(i))), ABS(i));
        ck_assert_int_eq(iimap_lookup(&map, ABS(i)), (2*ABS(i)));
    }
    ck_assert_int_eq(iimap_size(&map), 6);
    for (int i = 0; i < 10; i++) {
        if (i == 1 || i == 3 || i == 5 || i == 7 || i == 9) {
            ck_assert(iimap_remove(&map, ABS(i)));
        } else {
            ck_assert(!iimap_remove(&map, ABS(i)));
        }
    }
    ck_assert_int_eq(iimap_size(&map), 1);
    iimap_clear(&map);
    ck_assert_int_eq(iimap_size(&map), 0);
    iimap_cleanup(&map);
}
END_TEST

Suite *iimap_suite(void)
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
    Suite *s = iimap_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
