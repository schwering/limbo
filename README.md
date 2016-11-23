LELA (Limited Epistemic Logic in Action; the acronym is tentative :-)) is a C++
library that aims to implement the first-order reasoning services described in
the papers [1,2] and perhaps also [3,4].

The language is sorted, first-order, with functions and equality. (Predicates
can be simulated with boolean functions.) Knowledge bases are restricted to be
in clausal form and only universally quantified variables (existentially
quantified variables can be simulated with functions). A sound and a complete
procedure is provided to evaluate (syntactically unrestricted) queries in a
knowledge base. These procedures are paramterised with a natural number that
specifies the maximum allowed reasoning effort, which is measured in the number
of case splits.

For examples of usage, check the [solver unit test](tests/solver.cc), which
includes examples from [1,2], or the [minesweeper
agent](examples/minesweeper.cc).

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

