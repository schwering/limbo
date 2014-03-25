TESTS = check_vector check_set check_map
check_PROGRAMS = check_vector check_set check_map
check_vector_SOURCES = check_vector.c $(top_builddir)/src/vector.h
check_vector_CFLAGS = @CHECK_CFLAGS@
check_vector_LDADD = $(top_builddir)/src/libesl.a -lcheck_pic -lrt -lm @CHECK_LIBS@
check_vector_LDFLAGS = -pthread
check_set_SOURCES = check_set.c $(top_builddir)/src/set.h
check_set_CFLAGS = @CHECK_CFLAGS@
check_set_LDADD = $(top_builddir)/src/libesl.a -lcheck_pic -lrt -lm @CHECK_LIBS@
check_set_LDFLAGS = -pthread
check_map_SOURCES = check_map.c $(top_builddir)/src/map.h
check_map_CFLAGS = @CHECK_CFLAGS@
check_map_LDADD = $(top_builddir)/src/libesl.a -lcheck_pic -lrt -lm @CHECK_LIBS@
check_map_LDFLAGS = -pthread

