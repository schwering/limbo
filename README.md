# Limited Epistemic Logic in Action

LELA (**L**imited **E**pistemic **L**ogic in **A**ction) is a C++ library for
*decidable reasoning in first-order knowledge bases* based on the theory of
*limited belief* from [1,2,3,4]. [Here is a
demo](http://www.cse.unsw.edu.au/~cschwering/demo/textinterface/).

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

* This [web demo](http://www.cse.unsw.edu.au/~cschwering/demo/textinterface/)
  allows to define and query a knowledge base through a simple text interface.
  You can also use it from the command line; the the code is
  [here](examples/textinterface/).
* To see how the C++ API works, have a look at [this](tests/solver.cc) and
  [that](tests/modal.cc) unit tests, which contain test cases that implement
  examples from [1,2]. To reduce boiler plate code, they use a [higher-level
  API](src/lela/format/cpp/syntax.h), which overloads some C++ operators.
* There's also [Minesweeper web
  demo](http://www.cse.unsw.edu.au/~cschwering/demo/minesweeper/). You can use
  it from the command line as well; the the code is
  [here](examples/minesweeper/).

## Status

* The first-oder logic of knowledge with functions and equality and the sound
  consistency from [1] is implemented. A difference to [1] is that the point
  of splitting is "deterministic", whereas [1] allows splitting at any stage
  during the proof.
* The complete but unsound entailment check (or, equivalently, sound
  consistency check) from [2] is implemented as well. Conditional belief from
  [2] still needs to be implemented, which should be easy. The belief and
  knowledge modalities should be made explicit then.
* Actions from [3] are not yet implemented. The plan is to keep actions and
  projection separate from the core reasoner and implement them as a
  preprocessing step.
* Nested beliefs from [4] are not yet implemented.
* If time permits, I'll rewrite the whole system in Rust. First results (cf.
  'rust' branch) indicate it's faster. And it's actually fun!

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

