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

The `bats` directory contains a few example basic action theories.
More a detailed discussions, see the corresponding papers.
Each basic action theory is written in Prolog (a `*.pl` file).
At compile time, it is translated to a proper+ basic action theory in C code (a `*.h` and a `*.c` file).

The `examples` directory contains a few examples which evaulate queries wrt the basic action theories.

An interface to ECLiPSe-CLP is coming up.


## To-Do List

* Avoid needless splits (need proofs):
  * Let `PEL(c)` be the least set such that (1) if `a` or `~a` is in `c`, then `a` is in `PEL(c)` and (2) if `c'` is in the setup, `a` is in `PEL(c)` and `a` or `~a` is in `c'`, then `PEL(c')` is a subset of `PEL(c)`.
    Then only the literals from `PEL(c)` are relevant for splitting unless the initial setup is inconsistent.
  * Suppose `c` is a clause from the setup, `d` is the query clause, `k` splits remaining.
    If `|c| > k+1` and `|c\d| > k`, then the PEL of c are not split-relevant at the current `k` (unless for some other clause `c'` from the setup), because (1) it won't trigger any unit propagation and (2) it won't lead to subsumption.
    This check perhaps should be performed in setup_with_splits_subsumes().
    As they depend on `k` and hence on the current setup (after unit propatations), it cannot be precomputed (except for the first iteration, which may a useful optimization in practice).
    Hence, to test if a literal `l` from PEL is in fact split-relevant, we need to check whether there is a clause `c` in the setup plus `d` such that `|c| <= k+1` or `|c\d| <= k` holds.
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

