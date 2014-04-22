if USE_BOEHM_GC
LIBGC = -lgc
else
LIBGC =
endif

if USE_NEDMALLOC
NEDMALLOCH = ext/nedmalloc.h
NEDMALLOCC = ext/nedmalloc.c
else
NEDMALLOCH =
NEDMALLOCC =
endif

noinst_LTLIBRARIES = libesl.la
include_HEADERS = belief.h literal.h map.h memory.h query.h set.h setup.h term.h vector.h $(NEDMALLOCH)
libesl_la_SOURCES = belief.c literal.c map.c memory.c query.c set.c setup.c term.c vector.c $(NEDMALLOCC)
libesl_la_LIBADD = -lm -lrt -lpthread $(LIBGC)

