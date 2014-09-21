# ESBL

ESBL is an implementation of a decidable fragment of the Epistemic Situation Calculus.
It is based on the logic ESL and, for the belief revision part, on the logic
ESB:

* Lakemeyer and Levesque: Decidable Reasoning in a Fragment of the Epistemic Situation Calculus, KR-2014
* Schwering and Lakemeyer: A Semantic Account of Iterated Belief Revision in the Situation Calculus, ECAI-2014


## Installation

ESBL is written in C, using a few GCC compiler extensions and the Boehm Garbage
Collector.
To compile and run the tests, run the following commands:
```
autoreconf --install
./configure CFLAGS='-Wall -Wextra -Wno-ignored-qualifiers -O3'
make
make check
```
Currently there's not much to run besides the tests.
The most interesting tests are perhaps `tests/check_query.c` and
`tests/check_belief.c`, which, amongst others, test the example BATs from the
ESL and ESB papers, which are defined in `tests/bat-esl.h` and
`tests/bat-esb.h`.


## Generate Proper+ BAT

There's a simple tool to bring a basic action theory in proper+ form.
The tool is written in Prolog and generates C code.
The basic action theories from the ESL and ESB papers are stored in `bat-esl.pl`
and `bat-esb.pl`, and the corresponding proper+ versions are in the headers
`tests/bat-esl.h` and `tests/bat-esb.h`.
They can be updated as follows:
```
$ swipl -f proper.pl
?- compile_all('bat-esl.pl', `tests/bat-esl.h`).
?- compile_all('bat-esb.pl', `tests/bat-esb.h`).
```

