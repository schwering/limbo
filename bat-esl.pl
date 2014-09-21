% Running example from:
% Lakemeyer, Levesque: Decidable Reasoning in a Fragment of the Epistemic Situation Calculus, KR-2014

box('POSS'(A) <-> A = forward ^ ~ d0 v A = sonar ^ true) :- put_sort(A, action).

box('SF'(A) <-> A = forward ^ true v A = sonar ^ (d0 v d1)) :- put_sort(A, action).

box(A:d0 <-> A = forward ^ d1 v d0) :-
    put_sort(A, action).
box(A:DI <-> A = forward ^ DI1 v A \= forward ^ DI) :-
    between(1, 3, I),
    I1 is I + 1,
    atom_concat('d', I, DI),
    atom_concat('d', I1, DI1),
    put_sort(A, action).

static(~d0).
static(~d1).
static('d2' v 'd3').

belief(_ => _) :- fail.

