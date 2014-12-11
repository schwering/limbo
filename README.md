# LELA

This is an implementation of a Limited Epistemic Logic with Actions based on a decidable fragment of the epistemic situation calulculus [1, 2].
Roughly, it bans existential quantifiers from the knowledge base, and limits the reasoning capabilities through a parameter *k* from the natural numbers, which allows the user to trade off runtime and completeness.
The logic allows to reason about (possibly incomplete) knowledge [1] and (possibly incomplete and/or incorrect) beliefs [3].
New knowledge or beliefs are produced by sensing actions.
Reasoning about actions is carried out by unit propagation [2], by regression [1, 4], or by progression [3].


## Installation

LELA is a library written in C++.
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


## References

1. Lakemeyer and Levesque: A semantic characterization of a useful fragment of the situation calculus with knowledge. In *Artificial Intelligence*.
2. Lakemeyer and Levesque: Decidable Reasoning in a Fragment of the Epistemic Situation Calculus. In *Proc. KR-2014*.
3. Liu and Lakemeyer: On First-Order Definability and Computability of Progression for Local-Effect Actions and Beyond. In *Proc. IJCAI-2009*.
3. Schwering and Lakemeyer: A Semantic Account of Iterated Belief Revision in the Situation Calculus. In *Proc. ECAI-2014*.
4. Schwering and Lakemeyer: Projection in the Epistemic Situation Calculus with Belief Conditionals. In *Proc. AAAI-2015*.

