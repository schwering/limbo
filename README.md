# A Reasoning System for First-Order Limited Belief

Limbo is a **reasoning system** for modelling and querying an agent's
knowledge. Limbo features a **highly expressive** modelling language
(with functions, equality, quantification, introspective modal
operators), but  maintains **attractive computational** properties.

Great expressivity usually brings along great computational complexity.
Limbo avoids the computational cliff using the *theory of limited
belief*, which offers a principled way to control the computational
effort and preserves decidability and, in the propositional case, even
tractability.

A quick example of the modelling language: the statement "I know
Sally's father is rich, but I don't know who he is" can be encoded
by the following Limbo formula:
**K**<sub>1</sub> exists *x* (fatherOf(Sally) = *x* ^ rich(*x*) = T ^ **M**<sub>1</sub> fatherOf(Sally) /= *x*).
The operators **K**<sub>1</sub> and **M**<sub>1</sub> refer to what
the agent knows or considers possible, respectively, at *belief level*
1; the belief level is the key concept that controls the computational
effort.

Limbo is a **C++ library**. Started as an academic proof of concept,
it'll hopefully develop into a practical reasoning system in the longer
run. The name Limbo derives from **lim**ited **b**elief with an
Australian '-**o**' suffix.

Where to go from here?

* You could check out the [web
  demos](http://www.cse.unsw.edu.au/~cschwering/limbo/) to see limited
  belief in action.
* You could check out the code and [compile](#installation) it and run
  Limbo on your computer.
* You could have a look at one of the [papers](#references) on the
  theory behind Limbo.
* You could also send [me](http://www.cse.unsw.edu.au/~cschwering/) an email:
  c.schwering@unsw.edu.au.

## Features

Limbo provides a logical language to represent and reason about (possibly
incomplete) knowledge and belief.

This logical language is a first-order modal dialect: it features functions and
equality, standard names to designate distinct objects, first-order variables
and quantification, sorts, and modal operators for knowledge and conditional
belief. (Predicates are not built-in but can be simulated without overhead
using boolean functions.)

An agent's knowledge and beliefs are encoded in this language. This knowledge
base is subject to a syntactic restriction: it must be in clausal form, and all
variables must be universally quantified; however, as existentially quantified
variables can be simulated through Skolemization, this is no effective
restriction. An example knowledge base might assume a birthday scenario and
stipulate that that a certain gift box is known to contain an (unknown) gift.

Reasoning in such knowledge bases is done with queries expressed in this
language. For example, we can say that we believe that something is in the box
and that we have no idea what it is. A decision procedure evaluates such
queries in a way that is *sound* but *incomplete* with respect to classical
logic. That is, if the procedure says that the knowledge base entails the
query, then this correct, but conversely the procedure may miss some true
queries. Completeness is sacrificed for decidability, which means that the
procedure actually terminates. (Soundness, completeness, decidability in
first-order logic is one of those "pick any two" scenarios.)

How much *effort* (and time) is spent on evaluating a query is controlled
through a parameter that specifies how many case splits the reasoner may
investigate. Every modal operator is decorated with such an effort parameter.
This effort parameter and its limiting effect on the reasoning capabilities is
the key ingredient to achieve decidability without restricting the syntactical
expressivity of queries. This sets this theory apart from decidable syntactical
subclasses of first-order logic and from description logics.

For more theoretical background see the papers linked below.

## Examples

* This [web demo](http://www.cse.unsw.edu.au/~cschwering/limbo/tui/) allows to
  define and query a knowledge base through a simple text interface.
  You can also use it from the command line; the the code is
  [here](examples/tui/).
* There's are
  [Minesweeper](http://www.cse.unsw.edu.au/~cschwering/limbo/minesweeper/) and
  [Sudoku](http://www.cse.unsw.edu.au/~cschwering/limbo/sudoku/) web demos. You
  can use them it from the command line as well; the the code is
  [here](examples/minesweeper/) and [here](examples/sudoku/).
* The main part of the C++ API is [kb.h](src/limbo/kb.h),
  [formula.h](src/limbo/formula.h), [clause.h](src/limbo/clause.h),
  [literal.h](src/limbo/literal.h), [term.h](src/limbo/term.h), in this
  order. Optional C++ operator overloadings are provided by
  [format/cpp/syntax.h](src/limbo/format/cpp/syntax.h) to make creating
  formulas a little more convenient.

## Future work

Interesting KR concepts to be added include:

* Progression for actions.
* Sensing and/or belief revision.
* Multiple agents.

To improve the performance, we could investigate:

* Clause learning, including its theoretical implications.
* Backjumping.
* More efficient grounding.

## Installation

The library is header-only and has no third-party dependencies. It suffices to
have `src/limbo` in the include path.

To compile and run the tests and demos, execute the following:

```shell
$ git clone https://github.com/schwering/limbo.git
$ cd limbo
$ git submodule init
$ git submodule update
$ cmake .
$ make
$ make test
```

## References

The first paper is the most recent one and describes the theory behind Limbo.
I'm working on a technical report that includes new development since (and fixes a bunch of bugs in the extended version).
Many of the ideas were introduced in the earlier papers linked below.

1.  A Reasoning System for a First-Order Logic of Limited Belief. <br>
   C. Schwering. <br>
   In *Proc. IJCAI*, 2017. <br>
   [pdf](http://www.cse.unsw.edu.au/~cschwering/ijcai-2017.pdf),
   [proofs](https://arxiv.org/abs/1705.01817),
   [slides](http://www.cse.unsw.edu.au/~cschwering/ijcai-2017-slides.pdf)
2. Limbo: A Reasoning System for Limited Belief <br>
   C. Schwering. <br>
   In *Proc. IJCAI*, 2017. <br>
   [pdf](http://www.cse.unsw.edu.au/~cschwering/ijcai-2017-demo.pdf)
3. Reasoning in the Situation Calculus with Limited Belief <br>
   C. Schwering. <br>
   Commonsense, 2017. <br>
   [pdf](http://www.cse.unsw.edu.au/~cschwering/commonsense-2017.pdf)
4. Decidable Reasoning in a Logic of Limited Belief with Function Symbols. <br>
   G. Lakemeyer and H. Levesque. <br>
   In *Proc. KR*, 2016. <br>
   [pdf](https://kbsg.rwth-aachen.de/sites/kbsg/files/LakemeyerLevesque2016.pdf)
5. Decidable Reasoning in a First-Order Logic of Limited Conditional Belief. <br>
   C. Schwering and G. Lakemeyer. <br>
   In *Proc. ECAI*, 2016. <br>
   [pdf](http://www.cse.unsw.edu.au/~cschwering/ecai-2016.pdf),
   [slides](http://www.cse.unsw.edu.au/~cschwering/ecai-2016-slides.pdf)
6. Decidable Reasoning in a Fragment of the Epistemic Situation Calculus. <br>
   G. Lakemeyer and H. Levesque. <br>
   In *Proc. KR*, 2014. <br>
   [pdf](https://pdfs.semanticscholar.org/8ac9/a2955895cd391ec2b62d8210ee8206979f4a.pdf)
7. Decidable Reasoning in a Logic of Limited Belief with Introspection and Unknown Individuals. <br>
   G. Lakemeyer and H. Levesque. <br>
   In *Proc. IJCAI*, 2013. <br>
   [pdf](https://pdfs.semanticscholar.org/387c/951016c68aaf8ce36bb87e5ea4d1ef42405d.pdf)

