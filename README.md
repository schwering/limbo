# ESBL

ESBL is an implementation of a decidable fragment of the Epistemic Situation Calculus.
It is based on the logic ESL and, for the belief revision part, on the logic ESB:

* Lakemeyer and Levesque: Decidable Reasoning in a Fragment of the Epistemic Situation Calculus, KR-2014
* Schwering and Lakemeyer: A Semantic Account of Iterated Belief Revision in the Situation Calculus, ECAI-2014


## Installation

ESBL is a library written in C++.
Additionally there are Prolog interfaces to write basic action theories and to evaluate queries.

To compile everything and run the tests, run the following commands:
```
cmake .
make
make test
```

The `bats` directory contains a few example basic action theories.
Each basic action theory is written in Prolog (a `*.pl` file).
At compile time, it is translated to a proper+ basic action theory in C++ code (a `*.h` and a `*.cc` file).

The `examples` directory contains a few examples which evaluate queries wrt the basic action theories.

