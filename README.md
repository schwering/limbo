# Limited Epistemic Logic in Action

LELA (**L**imited **E**pistemic **L**ogic in **A**ction; the acronym is
tentative :-)) is a C++ library that facilitates *decidable reasoning in
first-order knowledge bases*. It aims to implement the theory of limited
reasoning presented in [1,2,3,4], which achieves decidability by putting
a limit on the maximum allowed reasoning effort.

## Features

The logical language features functions and equality, first-order
quantification, standard names, and sorts. (Predicates are not built-in but can
be simulated with no overhead using boolean functions.) Knowledge bases are
restricted to be in clausal form with only universally quantified variables.
(Existentially quantified variables in the knowledge base can be simulated with
functions.) Queries are not subject to any syntactic restriction.

The library provides procedures to check whether a query is entailed by or
consistent with a knowledge base. These procedures are *sound* but *incomplete*
with respect to classical logic. That is, whenever the procedure says the
knowledge base entails (is consistent with) a query, then this answer is
correct, but the converse direction does not hold. Completeness needs to be
sacrificed to achieve decidability.
The entailment and consistency procedures are parameterised with a natural
number that limits the maximum allowed reasoning effort, which is measured in
the number of case splits.

For the theoretical background, see the papers linked below, especially [1,2].

## Examples

For examples of usage, check the [solver unit test](tests/solver.cc), which
includes examples from [1,2], or the [minesweeper
agent](examples/minesweeper/mw.cc).

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

