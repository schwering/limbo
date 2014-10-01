# ESBL

ESBL is an implementation of a decidable fragment of the Epistemic Situation Calculus.
It is based on the logic ESL and, for the belief revision part, on the logic ESB:

* Lakemeyer and Levesque: Decidable Reasoning in a Fragment of the Epistemic Situation Calculus, KR-2014
* Schwering and Lakemeyer: A Semantic Account of Iterated Belief Revision in the Situation Calculus, ECAI-2014


## Installation

ESBL is written in C (C99 plus a few GCC extensions).
It uses the Boehm garbage collector.

To compile everything and run the tests, run the following commands:
```
cmake .
make
make test
```
There are also a few examples, which currently are just copy-paste of some unit tests.
I'll add some explanatory output.
They can be run as follows:
```
examples/kr-2014
examples/ecai-2014
```
For a detailed discussion of both examples, see the corresponding papers.
The basic action theories are encoded in Prolog code in the `bats/` directory.
The Prolog code is translated to C code by the following command:
```
$ swipl -f bats/proper-plus.pl
?- compile_all('bats/kr-2014.pl', 'bats/kr-2014.h').
?- compile_all('bats/ecai-2014.pl', 'bats/ecai-2014.h').
```
That should be done by the cmake-generated Makefiles, by the way.


## To-Do List

* Heuristically rank split literals (decision theory).
* Try a B+tree (or red-black tree or whatever) index with shadowing and clones for faster set operations.
* ~~Add an index `literal -> clause` to each setup which gives all clauses that contain a given literal for faster unit propagatation.~~
* For subsumption we currently iterate over the full setup to look for a clause that is a superset of the query.
  Alternatively, we could compute all sub-clauses of the query clause and check if one of them is contained in the setup.
  That should be faster in many scenarios:
  Suppose the setup has `2^n` clauses and the query clause has `m` literals.
  Currently, we perform `2^n` `is-subset` tests.
  With the alternative approach, we get `2^m` sub-clauses and hence `2^m * log(2^n) = 2^m * n` `set-equals` operations.
  Surely `is-subset` is more expensive than `set-equals`.
  But even if not, the current approach is more epensive than the proposed one iff `2^n > 2^m * n` iff `2^n / n > 2^m` iff `n - log(n) > m`.
  For example, if the setup has 1000 (10000) clauses, the alternative approach is cheaper for query clauses with 6 (9) literals.
  We could masking to very efficiently compute sub-clauses.

