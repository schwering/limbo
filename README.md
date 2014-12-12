# LELA

This is an implementation of a Limited Epistemic Logic with Actions based on a decidable fragment of the epistemic situation calulculus [1, 2].
Roughly, it bans existential quantifiers from the knowledge base, and limits the reasoning capabilities through a parameter *k* from the natural numbers, which allows the user to trade off runtime and completeness.
The logic allows to reason about (possibly incomplete) knowledge [1] and (possibly incomplete and/or incorrect) beliefs [3].
New knowledge or beliefs are produced by sensing actions.
Reasoning about actions is carried out by unit propagation [2], by regression [1, 4], or by progression [3].


## Installation

LELA is a C++ library.
It has no dependencies besides a C++11 compiler.

For convenience, there are (two separate) Prolog interfaces:

1. Basic action theories can be written in Prolog (c.f. the files `bats/*.pl`).
   Such a Prolog file is then compiled to a C++ class which issues the appropriate calls to the LELA C++ library.
   For that to work, a [SWI Prolog](http://www.swi-prolog.org) interpreter must be installed.
2. Queries can be evaluated from Prolog (c.f. the files `examples/*.pl`).
   More precisely, the LELA C++ library can be accessed from [ECLiPSe-CLP](http://www.eclipseclp.org).
   For that to work, the ECLiPSe-CLP header files must be installed to some subdirectory `eclipse-clp` in the search path, e.g., `/usr/include/eclipse-clp/eclipse.h`

Additionally there are Prolog interfaces to write basic action theories and to evaluate queries.
The first one requires an SWI Prolog interpreter.

The source of the LELA C++ library is located in `src/`, the test files in `tests/`, the basic action theories in `bats/`, and the example queries in `examples/`.
To compile everything and run the tests, run the following commands:
```
cmake .
make
make test
```

## Examples

For examples, check out the files `bats/*.pl` and `examples/*.pl`.

* The running example from [2] is implemented in `bats/kr2014.pl` and `examples/kr2014.pl`.
* The running example from [4] is implemented in `bats/ecai2014.pl` and `examples/ecai2014.pl`.

After having compiled everything as described above, you can run the examples using the ECLiPSe-CLP interpreter like `eclipse -f examples/kr2014.pl`.


## References

1. Lakemeyer and Levesque: A semantic characterization of a useful fragment of the situation calculus with knowledge. In *Artificial Intelligence*.
2. Lakemeyer and Levesque: Decidable Reasoning in a Fragment of the Epistemic Situation Calculus. In *Proc. KR-2014*.
3. Liu and Lakemeyer: On First-Order Definability and Computability of Progression for Local-Effect Actions and Beyond. In *Proc. IJCAI-2009*.
4. Schwering and Lakemeyer: A Semantic Account of Iterated Belief Revision in the Situation Calculus. In *Proc. ECAI-2014*.
5. Schwering and Lakemeyer: Projection in the Epistemic Situation Calculus with Belief Conditionals. In *Proc. AAAI-2015*.

