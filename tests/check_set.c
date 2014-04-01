// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include <check.h>
#include <stdlib.h>
#include "../src/set.h"

static int compar_long_int(long int p, long int q)
{
    return p - q;
}

SET_DECL(iset, long int);
SET_IMPL(iset, long int, compar_long_int);

START_TEST(test_set_add)
{
    iset_t set = iset_init();
    iset_t set_all = iset_init();
    for (int i = 0; i < 10; ++i) {
        iset_add(&set, i);
        iset_add(&set, (i + 10));
        iset_add(&set, i);
    }
    ck_assert_int_eq(iset_size(&set), 20);
    iset_add_all(&set_all, &set);
    for (int i = 0; i < 20; ++i) {
        ck_assert_int_eq(iset_find(&set, i), i);
        ck_assert(iset_contains(&set, i));
    }
    for (int i = 20; i < 30; ++i) {
        ck_assert_int_eq(iset_find(&set, -i), -1);
        ck_assert(!iset_contains(&set, -i));
    }
    for (int i = 5; i < 15; ++i) {
        ck_assert(iset_contains(&set, i));
        iset_remove(&set, i);
        ck_assert(!iset_contains(&set, i));
    }
    ck_assert_int_eq(iset_size(&set), 10);
    iset_add_all(&set_all, &set);
    for (int i = 5; i < 15; ++i) {
        ck_assert(!iset_contains(&set, i));
        iset_add(&set, i);
        ck_assert(iset_contains(&set, i));
    }
    for (int i = 15; i > 5; --i) {
        ck_assert(iset_contains(&set, i));
        iset_remove(&set, i);
        ck_assert(!iset_contains(&set, i));
    }
    ck_assert_int_eq(iset_size(&set), 10);
    iset_add_all(&set_all, &set);
    for (int i = 15; i > 5; --i) {
        ck_assert(!iset_contains(&set, i));
        iset_add(&set, i);
        ck_assert(iset_contains(&set, i));
    }
    ck_assert_int_eq(iset_size(&set), 20);
    iset_add_all(&set_all, &set);
    ck_assert_int_eq(iset_size(&set), 20);
    iset_free(&set_all);
    iset_free(&set);
}
END_TEST

START_TEST(test_set_copy)
{
    iset_t src = iset_init();
    iset_t dst;
    ck_assert_int_eq(iset_size(&src), 0);
    dst = iset_copy(&src);
    ck_assert_int_eq(iset_size(&dst), 0);
    for (int i = 0; i < 100; ++i) {
        iset_add(&src, i);
    }
    iset_free(&dst);
    ck_assert_int_eq(iset_size(&src), 100);
    dst = iset_copy(&src);
    ck_assert_int_eq(iset_size(&dst), 100);
    iset_free(&dst);
    iset_free(&src);
}
END_TEST

START_TEST(test_set_singleton)
{
    iset_t set = iset_singleton(5);
    ck_assert_int_eq(iset_size(&set), 1);
    ck_assert(iset_contains(&set, 5));
    ck_assert(!iset_contains(&set, 4));
    ck_assert(!iset_contains(&set, 6));
    iset_clear(&set);
    ck_assert_int_eq(iset_size(&set), 0);
    iset_free(&set);
}
END_TEST

START_TEST(test_set_union)
{
    iset_t set1 = iset_init();
    iset_t set2 = iset_init();
    iset_t set;
    // left and right empty
    ck_assert_int_eq(iset_size(&set1), 0);
    ck_assert_int_eq(iset_size(&set2), 0);
    set = iset_union(&set1, &set2);
    ck_assert_int_eq(iset_size(&set), 0);
    ck_assert(iset_eq(&set, &set1));
    iset_free(&set);
    iset_clear(&set1);
    iset_clear(&set2);
    // left empty
    ck_assert_int_eq(iset_size(&set1), 0);
    for (int i = 0; i < 10; ++i) {
        iset_add(&set2, i);
    }
    ck_assert_int_eq(iset_size(&set2), 10);
    set = iset_union(&set1, &set2);
    ck_assert_int_eq(iset_size(&set), 10);
    ck_assert(iset_eq(&set, &set2));
    iset_free(&set);
    iset_clear(&set1);
    iset_clear(&set2);
    // right empty
    for (int i = 0; i < 10; ++i) {
        iset_add(&set1, i);
    }
    ck_assert_int_eq(iset_size(&set1), 10);
    ck_assert_int_eq(iset_size(&set2), 0);
    set = iset_union(&set1, &set2);
    ck_assert_int_eq(iset_size(&set), 10);
    ck_assert(iset_eq(&set, &set1));
    iset_free(&set);
    iset_clear(&set1);
    iset_clear(&set2);
    // left same as right
    for (int i = 0; i < 10; ++i) {
        iset_add(&set1, i);
    }
    ck_assert_int_eq(iset_size(&set1), 10);
    for (int i = 0; i < 10; ++i) {
        iset_add(&set2, i);
    }
    ck_assert_int_eq(iset_size(&set2), 10);
    set = iset_union(&set1, &set2);
    ck_assert_int_eq(iset_size(&set), 10);
    ck_assert(iset_eq(&set, &set1));
    iset_free(&set);
    iset_clear(&set1);
    iset_clear(&set2);
    // left subset of right
    for (int i = 0; i < 10; ++i) {
        iset_add(&set1, i);
    }
    ck_assert_int_eq(iset_size(&set1), 10);
    for (int i = 0; i < 20; ++i) {
        iset_add(&set2, i);
    }
    ck_assert_int_eq(iset_size(&set2), 20);
    set = iset_union(&set1, &set2);
    ck_assert_int_eq(iset_size(&set), 20);
    ck_assert(iset_eq(&set, &set2));
    iset_free(&set);
    iset_clear(&set1);
    iset_clear(&set2);
    // left superset of right
    for (int i = 0; i < 20; ++i) {
        iset_add(&set1, i);
    }
    ck_assert_int_eq(iset_size(&set1), 20);
    for (int i = 0; i < 10; ++i) {
        iset_add(&set2, i);
    }
    ck_assert_int_eq(iset_size(&set2), 10);
    set = iset_union(&set1, &set2);
    ck_assert_int_eq(iset_size(&set), 20);
    ck_assert(iset_eq(&set, &set1));
    iset_free(&set);
    iset_clear(&set1);
    iset_clear(&set2);
    // left and right intersect
    for (int i = 0; i < 20; ++i) {
        iset_add(&set1, i);
    }
    ck_assert_int_eq(iset_size(&set1), 20);
    for (int i = 10; i < 30; ++i) {
        iset_add(&set2, i);
    }
    ck_assert_int_eq(iset_size(&set2), 20);
    set = iset_union(&set1, &set2);
    ck_assert_int_eq(iset_size(&set), 30);
    for (int i = 0; i < 30; ++i) {
        ck_assert_int_eq(iset_get(&set, i), i);
    }
    iset_free(&set);
    iset_clear(&set1);
    iset_clear(&set2);
    // left and right intersect twice
    for (int i = 0; i < 20; ++i) {
        iset_add(&set1, i);
        iset_add(&set1, (i + 40));
    }
    ck_assert_int_eq(iset_size(&set1), 40);
    for (int i = 10; i < 30; ++i) {
        iset_add(&set2, i);
        iset_add(&set2, (i + 40));
    }
    ck_assert_int_eq(iset_size(&set2), 40);
    set = iset_union(&set1, &set2);
    ck_assert_int_eq(iset_size(&set), 60);
    for (int i = 0; i < 30; ++i) {
        ck_assert_int_eq(iset_get(&set, i), i);
    }
    iset_free(&set);
    iset_clear(&set1);
    iset_clear(&set2);
    // left and right don't intersect
    for (int i = 0; i < 10; ++i) {
        iset_add(&set1, i);
    }
    ck_assert_int_eq(iset_size(&set1), 10);
    for (int i = 10; i < 20; ++i) {
        iset_add(&set2, i);
    }
    ck_assert_int_eq(iset_size(&set2), 10);
    set = iset_union(&set1, &set2);
    ck_assert_int_eq(iset_size(&set), 20);
    for (int i = 0; i < 20; ++i) {
        ck_assert_int_eq(iset_get(&set, i), i);
    }
    iset_free(&set);
    iset_free(&set1);
    iset_free(&set2);
}
END_TEST

START_TEST(test_set_difference)
{
    iset_t set1 = iset_init();
    iset_t set2 = iset_init();
    iset_t set;
    // left and right empty
    ck_assert_int_eq(iset_size(&set1), 0);
    ck_assert_int_eq(iset_size(&set2), 0);
    set = iset_difference(&set1, &set2);
    ck_assert_int_eq(iset_size(&set), 0);
    ck_assert(iset_eq(&set, &set1));
    iset_free(&set);
    iset_clear(&set1);
    iset_clear(&set2);
    // left empty
    ck_assert_int_eq(iset_size(&set1), 0);
    for (int i = 0; i < 10; ++i) {
        iset_add(&set2, i);
    }
    ck_assert_int_eq(iset_size(&set2), 10);
    set = iset_difference(&set1, &set2);
    ck_assert_int_eq(iset_size(&set), 0);
    iset_free(&set);
    iset_clear(&set1);
    iset_clear(&set2);
    // right empty
    for (int i = 0; i < 10; ++i) {
        iset_add(&set1, i);
    }
    ck_assert_int_eq(iset_size(&set1), 10);
    ck_assert_int_eq(iset_size(&set2), 0);
    set = iset_difference(&set1, &set2);
    ck_assert_int_eq(iset_size(&set), 10);
    ck_assert(iset_eq(&set, &set1));
    iset_free(&set);
    iset_clear(&set1);
    iset_clear(&set2);
    // left same as right
    for (int i = 0; i < 10; ++i) {
        iset_add(&set1, i);
    }
    ck_assert_int_eq(iset_size(&set1), 10);
    for (int i = 0; i < 10; ++i) {
        iset_add(&set2, i);
    }
    ck_assert_int_eq(iset_size(&set2), 10);
    set = iset_difference(&set1, &set2);
    ck_assert_int_eq(iset_size(&set), 0);
    iset_free(&set);
    iset_clear(&set1);
    iset_clear(&set2);
    // left subset of right
    for (int i = 0; i < 10; ++i) {
        iset_add(&set1, i);
    }
    ck_assert_int_eq(iset_size(&set1), 10);
    for (int i = 0; i < 20; ++i) {
        iset_add(&set2, i);
    }
    ck_assert_int_eq(iset_size(&set2), 20);
    set = iset_difference(&set1, &set2);
    ck_assert_int_eq(iset_size(&set), 0);
    iset_free(&set);
    iset_clear(&set1);
    iset_clear(&set2);
    // left superset of right
    for (int i = 0; i < 20; ++i) {
        iset_add(&set1, i);
    }
    ck_assert_int_eq(iset_size(&set1), 20);
    for (int i = 0; i < 10; ++i) {
        iset_add(&set2, i);
    }
    ck_assert_int_eq(iset_size(&set2), 10);
    set = iset_difference(&set1, &set2);
    ck_assert_int_eq(iset_size(&set), 10);
    for (int i = 0; i < 10; ++i) {
        ck_assert_int_eq(iset_get(&set, i), i + 10);
    }
    iset_free(&set);
    iset_clear(&set1);
    iset_clear(&set2);
    // left and right intersect
    for (int i = 0; i < 20; ++i) {
        iset_add(&set1, i);
    }
    ck_assert_int_eq(iset_size(&set1), 20);
    for (int i = 10; i < 30; ++i) {
        iset_add(&set2, i);
    }
    ck_assert_int_eq(iset_size(&set2), 20);
    set = iset_difference(&set1, &set2);
    ck_assert_int_eq(iset_size(&set), 10);
    for (int i = 0; i < 10; ++i) {
        ck_assert_int_eq(iset_get(&set, i), i);
    }
    iset_free(&set);
    iset_clear(&set1);
    iset_clear(&set2);
    // left and right intersect twice
    for (int i = 0; i < 20; ++i) {
        iset_add(&set1, i);
        iset_add(&set1, (i + 40));
    }
    ck_assert_int_eq(iset_size(&set1), 40);
    for (int i = 10; i < 30; ++i) {
        iset_add(&set2, i);
        iset_add(&set2, (i + 40));
    }
    ck_assert_int_eq(iset_size(&set2), 40);
    set = iset_difference(&set1, &set2);
    ck_assert_int_eq(iset_size(&set), 20);
    for (int i = 0; i < 10; ++i) {
        ck_assert_int_eq(iset_get(&set, i), i);
        ck_assert_int_eq(iset_get(&set, i + 10), i + 40);
    }
    iset_free(&set);
    iset_clear(&set1);
    iset_clear(&set2);
    // left and right don't intersect
    for (int i = 0; i < 10; ++i) {
        iset_add(&set1, i);
    }
    ck_assert_int_eq(iset_size(&set1), 10);
    for (int i = 10; i < 20; ++i) {
        iset_add(&set2, i);
    }
    ck_assert_int_eq(iset_size(&set2), 10);
    set = iset_difference(&set1, &set2);
    ck_assert_int_eq(iset_size(&set), 10);
    ck_assert(iset_eq(&set, &set1));
    iset_free(&set);
    iset_free(&set1);
    iset_free(&set2);
}
END_TEST

START_TEST(test_set_remove_all)
{
    iset_t set1 = iset_init();
    iset_t set2 = iset_init();
    iset_t set;
    // left and right empty
    ck_assert_int_eq(iset_size(&set1), 0);
    ck_assert_int_eq(iset_size(&set2), 0);
    set = iset_copy(&set1);
    iset_remove_all(&set, &set2);
    ck_assert_int_eq(iset_size(&set), 0);
    ck_assert(iset_eq(&set, &set1));
    iset_free(&set);
    iset_clear(&set1);
    iset_clear(&set2);
    // left empty
    ck_assert_int_eq(iset_size(&set1), 0);
    for (int i = 0; i < 10; ++i) {
        iset_add(&set2, i);
    }
    ck_assert_int_eq(iset_size(&set2), 10);
    set = iset_copy(&set1);
    iset_remove_all(&set, &set2);
    ck_assert_int_eq(iset_size(&set), 0);
    iset_free(&set);
    iset_clear(&set1);
    iset_clear(&set2);
    // right empty
    for (int i = 0; i < 10; ++i) {
        iset_add(&set1, i);
    }
    ck_assert_int_eq(iset_size(&set1), 10);
    ck_assert_int_eq(iset_size(&set2), 0);
    set = iset_copy(&set1);
    iset_remove_all(&set, &set2);
    ck_assert_int_eq(iset_size(&set), 10);
    ck_assert(iset_eq(&set, &set1));
    iset_free(&set);
    iset_clear(&set1);
    iset_clear(&set2);
    // left same as right
    for (int i = 0; i < 10; ++i) {
        iset_add(&set1, i);
    }
    ck_assert_int_eq(iset_size(&set1), 10);
    for (int i = 0; i < 10; ++i) {
        iset_add(&set2, i);
    }
    ck_assert_int_eq(iset_size(&set2), 10);
    set = iset_copy(&set1);
    iset_remove_all(&set, &set2);
    ck_assert_int_eq(iset_size(&set), 0);
    iset_free(&set);
    iset_clear(&set1);
    iset_clear(&set2);
    // left subset of right
    for (int i = 0; i < 10; ++i) {
        iset_add(&set1, i);
    }
    ck_assert_int_eq(iset_size(&set1), 10);
    for (int i = 0; i < 20; ++i) {
        iset_add(&set2, i);
    }
    ck_assert_int_eq(iset_size(&set2), 20);
    set = iset_copy(&set1);
    iset_remove_all(&set, &set2);
    ck_assert_int_eq(iset_size(&set), 0);
    iset_free(&set);
    iset_clear(&set1);
    iset_clear(&set2);
    // left superset of right
    for (int i = 0; i < 20; ++i) {
        iset_add(&set1, i);
    }
    ck_assert_int_eq(iset_size(&set1), 20);
    for (int i = 0; i < 10; ++i) {
        iset_add(&set2, i);
    }
    ck_assert_int_eq(iset_size(&set2), 10);
    set = iset_copy(&set1);
    iset_remove_all(&set, &set2);
    ck_assert_int_eq(iset_size(&set), 10);
    for (int i = 0; i < 10; ++i) {
        ck_assert_int_eq(iset_get(&set, i), i + 10);
    }
    iset_free(&set);
    iset_clear(&set1);
    iset_clear(&set2);
    // left and right intersect
    for (int i = 0; i < 20; ++i) {
        iset_add(&set1, i);
    }
    ck_assert_int_eq(iset_size(&set1), 20);
    for (int i = 10; i < 30; ++i) {
        iset_add(&set2, i);
    }
    ck_assert_int_eq(iset_size(&set2), 20);
    set = iset_copy(&set1);
    iset_remove_all(&set, &set2);
    ck_assert_int_eq(iset_size(&set), 10);
    for (int i = 0; i < 10; ++i) {
        ck_assert_int_eq(iset_get(&set, i), i);
    }
    iset_free(&set);
    iset_clear(&set1);
    iset_clear(&set2);
    // left and right intersect twice
    for (int i = 0; i < 20; ++i) {
        iset_add(&set1, i);
        iset_add(&set1, (i + 40));
    }
    ck_assert_int_eq(iset_size(&set1), 40);
    for (int i = 10; i < 30; ++i) {
        iset_add(&set2, i);
        iset_add(&set2, (i + 40));
    }
    ck_assert_int_eq(iset_size(&set2), 40);
    set = iset_copy(&set1);
    iset_remove_all(&set, &set2);
    ck_assert_int_eq(iset_size(&set), 20);
    for (int i = 0; i < 10; ++i) {
        ck_assert_int_eq(iset_get(&set, i), i);
        ck_assert_int_eq(iset_get(&set, i + 10), i + 40);
    }
    iset_free(&set);
    iset_clear(&set1);
    iset_clear(&set2);
    // left and right don't intersect
    for (int i = 0; i < 10; ++i) {
        iset_add(&set1, i);
    }
    ck_assert_int_eq(iset_size(&set1), 10);
    for (int i = 10; i < 20; ++i) {
        iset_add(&set2, i);
    }
    ck_assert_int_eq(iset_size(&set2), 10);
    set = iset_copy(&set1);
    iset_remove_all(&set, &set2);
    ck_assert_int_eq(iset_size(&set), 10);
    ck_assert(iset_eq(&set, &set1));
    iset_free(&set);
    iset_free(&set1);
    iset_free(&set2);
}
END_TEST

START_TEST(test_set_intersection)
{
    iset_t set1 = iset_init();
    iset_t set2 = iset_init();
    iset_t set;
    // left and right empty
    ck_assert_int_eq(iset_size(&set1), 0);
    ck_assert_int_eq(iset_size(&set2), 0);
    set = iset_intersection(&set1, &set2);
    ck_assert_int_eq(iset_size(&set), 0);
    ck_assert(iset_eq(&set, &set1));
    iset_free(&set);
    iset_clear(&set1);
    iset_clear(&set2);
    // left empty
    ck_assert_int_eq(iset_size(&set1), 0);
    for (int i = 0; i < 10; ++i) {
        iset_add(&set2, i);
    }
    ck_assert_int_eq(iset_size(&set2), 10);
    set = iset_intersection(&set1, &set2);
    ck_assert_int_eq(iset_size(&set), 0);
    iset_free(&set);
    iset_clear(&set1);
    iset_clear(&set2);
    // right empty
    for (int i = 0; i < 10; ++i) {
        iset_add(&set1, i);
    }
    ck_assert_int_eq(iset_size(&set1), 10);
    ck_assert_int_eq(iset_size(&set2), 0);
    set = iset_intersection(&set1, &set2);
    ck_assert_int_eq(iset_size(&set), 0);
    iset_free(&set);
    iset_clear(&set1);
    iset_clear(&set2);
    // left same as right
    for (int i = 0; i < 10; ++i) {
        iset_add(&set1, i);
    }
    ck_assert_int_eq(iset_size(&set1), 10);
    for (int i = 0; i < 10; ++i) {
        iset_add(&set2, i);
    }
    ck_assert_int_eq(iset_size(&set2), 10);
    set = iset_intersection(&set1, &set2);
    ck_assert_int_eq(iset_size(&set), 10);
    ck_assert(iset_eq(&set, &set1));
    iset_free(&set);
    iset_clear(&set1);
    iset_clear(&set2);
    // left subset of right
    for (int i = 0; i < 10; ++i) {
        iset_add(&set1, i);
    }
    ck_assert_int_eq(iset_size(&set1), 10);
    for (int i = 0; i < 20; ++i) {
        iset_add(&set2, i);
    }
    ck_assert_int_eq(iset_size(&set2), 20);
    set = iset_intersection(&set1, &set2);
    ck_assert_int_eq(iset_size(&set), 10);
    ck_assert(iset_eq(&set, &set1));
    iset_free(&set);
    iset_clear(&set1);
    iset_clear(&set2);
    // left superset of right
    for (int i = 0; i < 20; ++i) {
        iset_add(&set1, i);
    }
    ck_assert_int_eq(iset_size(&set1), 20);
    for (int i = 0; i < 10; ++i) {
        iset_add(&set2, i);
    }
    ck_assert_int_eq(iset_size(&set2), 10);
    set = iset_intersection(&set1, &set2);
    ck_assert_int_eq(iset_size(&set), 10);
    ck_assert(iset_eq(&set, &set2));
    iset_free(&set);
    iset_clear(&set1);
    iset_clear(&set2);
    // left and right intersect
    for (int i = 0; i < 20; ++i) {
        iset_add(&set1, i);
    }
    ck_assert_int_eq(iset_size(&set1), 20);
    for (int i = 10; i < 30; ++i) {
        iset_add(&set2, i);
    }
    ck_assert_int_eq(iset_size(&set2), 20);
    set = iset_intersection(&set1, &set2);
    ck_assert_int_eq(iset_size(&set), 10);
    for (int i = 0; i < 10; ++i) {
        ck_assert_int_eq(iset_get(&set, i), i + 10);
    }
    iset_free(&set);
    iset_clear(&set1);
    iset_clear(&set2);
    // left and right intersect twice
    for (int i = 0; i < 20; ++i) {
        iset_add(&set1, i);
        iset_add(&set1, (i + 40));
    }
    ck_assert_int_eq(iset_size(&set1), 40);
    for (int i = 10; i < 30; ++i) {
        iset_add(&set2, i);
        iset_add(&set2, (i + 40));
    }
    ck_assert_int_eq(iset_size(&set2), 40);
    set = iset_intersection(&set1, &set2);
    ck_assert_int_eq(iset_size(&set), 20);
    for (int i = 0; i < 10; ++i) {
        ck_assert_int_eq(iset_get(&set, i), i + 10);
        ck_assert_int_eq(iset_get(&set, i + 10), i + 50);
    }
    iset_free(&set);
    iset_clear(&set1);
    iset_clear(&set2);
    // left and right don't intersect
    for (int i = 0; i < 10; ++i) {
        iset_add(&set1, i);
    }
    ck_assert_int_eq(iset_size(&set1), 10);
    for (int i = 10; i < 20; ++i) {
        iset_add(&set2, i);
    }
    ck_assert_int_eq(iset_size(&set2), 10);
    set = iset_intersection(&set1, &set2);
    ck_assert_int_eq(iset_size(&set), 0);
    iset_free(&set);
    iset_free(&set1);
    iset_free(&set2);
}
END_TEST

Suite *iset_suite(void)
{
  Suite *s = suite_create("Set");
  TCase *tc_core = tcase_create("Core");
  tcase_add_test(tc_core, test_set_add);
  tcase_add_test(tc_core, test_set_copy);
  tcase_add_test(tc_core, test_set_singleton);
  tcase_add_test(tc_core, test_set_union);
  tcase_add_test(tc_core, test_set_difference);
  tcase_add_test(tc_core, test_set_remove_all);
  tcase_add_test(tc_core, test_set_intersection);
  suite_add_tcase(s, tc_core);
  return s;
}

int main(void)
{
    int number_failed;
    Suite *s = iset_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

