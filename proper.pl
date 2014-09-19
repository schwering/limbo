% vim:filetype=prolog:textwidth=80:shiftwidth=4:softtabstop=4:expandtab

:- use_module(library(apply)).

:- op(820, fx, ~).    /* Negation */
:- op(840, xfy, ^).   /* Conjunction */
:- op(850, xfy, v).   /* Disjunction */
:- op(870, xfy, ->).  /* Implication */
:- op(880, xfy, <->). /* Equivalence */
:- op(890, xfy, =>).  /* Belief conditional */

box(poss(A) <-> A = forward ^ ~ d0 v A = sonar ^ true).

box(sf(A) <-> A = forward ^ true v A = sonar ^ (d0 v d1)).

box(A:d0 <-> A = forward ^ d1 v d0).
box(A:DI <-> A = forward ^ DI1 v A \= forward ^ DI) :- between(1, 3, I), I1 is I + 1, atom_concat('d', I, DI), atom_concat('d', I1, DI1).

init(~d0).
init(~d1).
init(d2 v d3).

cartesian([], _, []).
cartesian([A|N], L, M) :- pair(A, L, M1), cartesian(N, L, M2), append(M1, M2, M).

pair(_, [], []).
pair(A, [B|L], [AB|N]) :- append(A, B, AB), pair(A, L, N).

cnf(_ => _, _) :- !, throw('cnf/2: unexpected =>').
cnf(A1 <-> A2, B) :- !, cnf((A1 -> A2) ^ (A2 -> A1), B).
cnf(A1 -> A2, B) :- !, cnf(~A1 v A2, B).
cnf(A1 ^ A2, B) :- !, cnf(A1, B1), cnf(A2, B2), append(B1, B2, B).
cnf(A1 v A2, B) :- !, cnf(A1, B1), cnf(A2, B2), cartesian(B1, B2, B).
cnf(~(A1 ^ A2), B) :- !, cnf(~A1 v ~A2, B).
cnf(~(A1 v A2), B) :- !, cnf(~A1 ^ ~A2, B).
cnf(~(~A), B) :- !, cnf(A, B).
cnf(~true, [[]]) :- !.
cnf(~(X\=Y), [[X=Y]]) :- !.
cnf(~A, [[~A]]) :- !.
cnf(true, []) :- !.
cnf((X\=Y), [[~(X=Y)]]) :- !.
cnf(A, [[A]]).

var_list([], [], []).
var_list([V|Args], [V|NewArgs], E) :- var(V), !, var_list(Args, NewArgs, E).
var_list([T|Args], [V|NewArgs], [V=T|E]) :- var_list(Args, NewArgs, E).

varify(T1:(~T2), ~(T1:T3), E) :- !, varify(T2, T3, E).
varify(T1:T2, T1:T3, E) :- !, varify(T2, T3, E).
varify(~A, ~A1, E) :- !, varify(A, A1, E).
varify(A, A1, E) :- A =..[F|Args], var_list(Args, NewArgs, E), A1 =..[F|NewArgs].

ewffy([], [], []).
ewffy([(X=Y)|As], [~(X=Y)|Es], Ls) :- !, ewffy(As, Es, Ls).
ewffy([~(X=Y)|As], [(X=Y)|Es], Ls) :- !, ewffy(As, Es, Ls).
ewffy([(X\=Y)|As], [(X=Y)|Es], Ls) :- !, ewffy(As, Es, Ls).
ewffy([~(X\=Y)|As], [~(X=Y)|Es], Ls) :- !, ewffy(As, Es, Ls).
ewffy([A|As], Es, [L|Ls]) :- varify(A, L, Es1), ewffy(As, Es2, Ls), append(Es1, Es2, Es).

cnf_to_ecnf([], []).
cnf_to_ecnf([C|Cs], ECs1) :- ewffy(C, E, EC), ( (member((X1=Y1), E), member(~(X2=Y2), E), X1 == X2, Y1 == Y1) -> ECs1 = ECs ; ECs1 = [(E->EC)|ECs] ), cnf_to_ecnf(Cs, ECs).

run :-
    make,
    box(Alpha),
    nl,
    writeln(Alpha),
    cnf(Alpha,
    Cnf),
    cnf_to_ecnf(Cnf, ECnf),
    writeln('---------------------'),
    maplist(writeln, ECnf),
    writeln('---------------------').

