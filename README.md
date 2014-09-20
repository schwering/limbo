# ESBL

ESBL is an implementation of a decidable fragment of the Epistemic Situation Calculus.
It is based on the logic ESL [Lakemeyer and Levesque, KR-2014] and ESB [Schwering and Lakemeyer, ECAI-2014].


## Installation

ESBL is written in C, using a few GCC compiler extensions and the Boehm Garbage Collector.
To compile and run the tests, run the following commands:
```
./configure CFLAGS='-Wall -Wextra -Wno-ignored-qualifiers -O3'
make
make check
```
Currently there's not much to run besides the tests.
The most interesting tests are perhaps `tests/check_query.c` and `tests/check_belief.c`, which, amongst others, test the example BATs from the ESL and ESB papers, which are defined in `tests/ex_bat.h` and `tests/ex_bel.h` (sorry for the naming).


## Generate Proper+ BAT

There's a simple compiler to create proper+ basic action theories.
The original BAT is written in Prolog, the output is a bunch of C statements.
Check `bat-esl.pl` and `bat-esb.pl` for the examples from the ESL and ESB papers.
Then run the following to generate the proper+ BAT:
```
$ swipl -f proper.pl
?- ['bat-esb.pl'].
?- compile_all.
```
The printed C statements heavily use the macros from `src/util.h` to concisely express 
Then replace the definition of `DECL_ALL_CLAUSES` in `src/ex_bat.h` or `src/ex_bel.h`, respectively, with the generated C code.

