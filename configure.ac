AC_INIT([libesl], [1.0], [schwering@kbsg.rwth-aachen.de])
AM_INIT_AUTOMAKE([foreign -Wall])
PKG_CHECK_MODULES([CHECK], [check >= 0.9.4])
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CC_STDC
AM_PROG_AR
LT_INIT([disable-shared])
AC_ARG_ENABLE(gc,
	      [  --enable-gc             use Boehm GC for memory allocation [default=yes]],
	      [if test x$enableval = xyes
	       then
		       memory_alloc="BOEHM_GC"
		       AC_DEFINE(USE_BOEHM_GC)
	       fi
	      ],
	      [memory_alloc="BOEHM_GC"
	       AC_DEFINE(USE_BOEHM_GC)
	      ])
AC_ARG_ENABLE(nedmalloc,
	      [  --enable-nedmalloc      use nedmalloc for memory allocation [default=no]],
	      [if test x$enableval = xyes
	       then
		       memory_alloc="NEDMALLOC"
		       AC_DEFINE(USE_NEDMALLOC)
	       fi
	      ])
AM_CONDITIONAL(USE_BOEHM_GC, test "$memory_alloc" = "BOEHM_GC")
AM_CONDITIONAL(USE_NEDMALLOC, test "$memory_alloc" = "NEDMALLOC")
AC_CONFIG_FILES([Makefile src/Makefile tests/Makefile])
AC_CONFIG_MACRO_DIR([m4])
AC_OUTPUT

