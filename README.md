# Limited Epistemic Logic in Action

LELA (**L**imited **E**pistemic **L**ogic in **A**ction) is a C++ library for
*decidable reasoning in first-order knowledge bases* based on the theory of
*limited belief* from [1,2,3,4]. [Here are some web
demos](http://www.cse.unsw.edu.au/~cschwering/demo/).

## Features

The library provides a logical language to encode and reason about knowledge
and belief.

The logical language features functions of different sorts, standard names that
designate distinct objects, first-order variables and quantification, equality,
and modal operators for knowledge and conditional belief. (Predicates are not
built-in but can be simulated without overhead using boolean functions.)

An agent's knowledge and beliefs are encoded in this language. This knowledge
base is subject to a syntactic restriction: it must be in clausal form, and all
variables must be universally quantified. (Existentially quantified variables
in the knowledge base can be simulated through Skolemization.) For instance,
assuming a birthday scenario and a gift box, we could say that we know the box
contains a gift.

Reasoning in such knowledge bases is done with queries expressed in this
language. For example, we can say that we believe that something is in the box
and that we have no idea what it is. A decision procedure evaluates such
queries in a way that is *sound* but *incomplete* with respect to classical
logic. That is, if the procedure says that the knowledge base entails the
query, then this correct, but conversely the procedure may miss some true
queries. Completeness is sacrificed for decidability, which means that the
procedure actually terminates. (Soundness, completeness, decidability in
first-order logic is one of those "pick any two" scenarios.)

How much *effort* (and time) is spend on evaluating a query is controlled
through a parameter that specifies how many case splits the reasoner may
investigate. Every modal operator is decorated with such an effort parameter.
This effort parameter and its limiting effect on the reasoning capabilities is
the key ingredient to achieve decidability without restricting the syntactical
expressivity of queries. This sets this theory apart from decidable syntactical
subclasses of first-order logic and from description logics.

For more theoretical background see the papers linked below.

## Examples

* This [web demo](http://www.cse.unsw.edu.au/~cschwering/demo/tui/) allows to
  define and query a knowledge base through a simple text interface.
  You can also use it from the command line; the the code is
  [here](examples/tui/).
* There's are
  [Minesweeper](http://www.cse.unsw.edu.au/~cschwering/demo/minesweeper/) and
  [Sudoku](http://www.cse.unsw.edu.au/~cschwering/demo/sudoku/) web demos. You
  can use them it from the command line as well; the the code is
  [here](examples/minesweeper/) and [here](examples/sudoku/).
* The main part of the C++ API is [kb.h](src/lela/kb.h),
  [formula.h](src/lela/formula.h), [clause.h](src/lela/clause.h),
  [literal.h](src/lela/literal.h), [term.h](src/lela/term.h), in this
  order.
  To make creating formulas a little more convenient,
  [format/cpp/syntax.h](src/lela/format/cpp/syntax.h) overloads some C++
  operators.

## Future work

* Add sitcalc-style actions: regression, progression, or simulate ESL [3] with
  preprocessing, or all of them?
* Add clause learning and/or backjumping?
* Improve grounding.
* Have a look at some other KR concepts.

## References

1. Lakemeyer and Levesque. Decidable Reasoning in a Logic of Limited Belief
   with Function Symbols. KR 2016.
   [PDF](https://kbsg.rwth-aachen.de/sites/kbsg/files/LakemeyerLevesque2016.pdf)
2. Schwering and Lakemeyer. Decidable Reasoning in a First-Order Logic of
   Limited Conditional Belief. ECAI 2016.
   [PDF](https://kbsg.rwth-aachen.de/sites/kbsg/files/SchweringLakemeyer2016.pdf)
3. Lakemeyer and Levesque. Decidable Reasoning in a Fragment of the Epistemic
   Situation Calculus. KR 2014.
   [PDF](https://pdfs.semanticscholar.org/8ac9/a2955895cd391ec2b62d8210ee8206979f4a.pdf)
4. Lakemeyer and Levesque. Decidable Reasoning in a Logic of Limited Belief
   with Introspection and Unknown Individuals. IJCAI 2013.
   [PDF](https://pdfs.semanticscholar.org/387c/951016c68aaf8ce36bb87e5ea4d1ef42405d.pdf)

