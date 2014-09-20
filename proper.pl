% vim:filetype=prolog:textwidth=80:shiftwidth=4:softtabstop=4:expandtab

:- use_module(library(apply)).

:- op(820, fx, ~).    /* Negation */
:- op(840, xfy, ^).   /* Conjunction */
:- op(850, xfy, v).   /* Disjunction */
:- op(870, xfy, ->).  /* Implication */
:- op(880, xfy, <->). /* Equivalence */
:- op(890, xfy, =>).  /* Belief conditional */

vsort(V, Sort) :- put_attr(V, sort, Sort).

:- dynamic(box/1).
:- dynamic(static/1).
:- dynamic(belief/1).


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

sortify([], []).
sortify([V|Vs], [sort(V, Sort)|E]) :- get_attr(V, sort, Sort), !, sortify(Vs, E).
sortify([_|Vs], E) :- sortify(Vs, E).

sortify(E, C, E2) :-
    term_variables((E, C), Vars),
    sortify(Vars, E1),
    append(E1, E, E2).

cnf_to_ecnf([], []).
cnf_to_ecnf([C|Cs], ECs1) :- ewffy(C, E1, EC), sortify(E1, EC, E), ( (member((X1=Y1), E), member(~(X2=Y2), E), X1 == X2, Y1 == Y2) -> ECs1 = ECs ; ECs1 = [(E->EC)|ECs] ), cnf_to_ecnf(Cs, ECs).


names([], _, []).
names([V|Vs], I, [N=V|Ns]) :- get_attr(V, sort, action), !, atom_concat('A', I, N), I1 is I + 1, names(Vs, I1, Ns).
names([V|Vs], I, [N=V|Ns]) :- atom_concat('X', I, N), I1 is I + 1, names(Vs, I1, Ns).

print(ECnf) :-
    term_variables(ECnf, Vars),
    names(Vars, 0, Names),
    write_term(ECnf, [variable_names(Names), nl(true)]),
    nl.

print_all :-
    (   box(Alpha1), cnf(Alpha1, Cnf1), cnf_to_ecnf(Cnf1, ECnf1), Alpha = box(Alpha1), ECnf = (ECnf1)
    ;   static(Alpha1), cnf(Alpha1, Cnf1), cnf_to_ecnf(Cnf1, ECnf1), Alpha = static(Alpha1), ECnf = (ECnf1)
    ),
    nl,
    print(Alpha),
    writeln('---------------------'),
    maplist(print, ECnf),
    writeln('---------------------').

compile_ewff([], 'TRUE').
compile_ewff([(X=Y)|E], 'AND'('EQ'(X, Y), E1)) :- compile_ewff(E, E1).
compile_ewff([~(X=Y)|E], 'AND'('NEQ'(X, Y), E1)) :- compile_ewff(E, E1).
compile_ewff([sort(X, Sort)|E], 'AND'('SORT'(X, Sort), E1)) :- compile_ewff(E, E1).

literal_z_f_a(~L, As, F, Ts) :- !, literal_z_f_a(L, As, F, Ts).
literal_z_f_a(A:L, [A|As], F, Ts) :- !, literal_z_f_a(L, As, F, Ts).
literal_z_f_a(L, [], F, Ts) :- L =..[F|Ts].

compile_literal(L, L_C) :-
    ( L  = (~_), L_C = 'N'(Actions, Symbol, Args)
    ; L \= (~_), L_C = 'P'(Actions, Symbol, Args)
    ),
    literal_z_f_a(L, ActionList, Symbol, ArgList),
    ( ActionList = [] -> Actions =..['Z', ''] ; Actions =..['Z'|ActionList] ),
    ( ArgList    = [] -> Args    =..['A', ''] ; Args    =..['A'|ArgList]    ).

compile_literals([], []).
compile_literals([L|Ls], [L_C|Ls_C]) :-
    compile_literal(L, L_C),
    compile_literals(Ls, Ls_C).

compile_clause(C, C_C) :-
    compile_literals(C, Ls_C),
    ( Ls_C = [] -> C_C =..['C', ''] ; C_C =..['C' | Ls_C] ).

compile_box(E, Alpha, S) :-
    term_variables((E, Alpha), Vars),
    names(Vars, 0, Names),
    compile_ewff(E, E_C),
    compile_clause(Alpha, Alpha_C),
    with_output_to(atom(S), write_term('box_univ_clauses_append'('dynamic_bat', 'box_univ_clause_init'(E_C, Alpha_C)), [variable_names(Names)])).

compile_static(E, Alpha, S) :-
    term_variables((E, Alpha), Vars),
    names(Vars, 0, Names),
    compile_ewff(E, E_C),
    compile_clause(Alpha, Alpha_C),
    with_output_to(atom(S), write_term('univ_clauses_append'('static_bat', 'univ_clause_init'(E_C, Alpha_C)), [variable_names(Names)])).

compile_belief(E, NegPhi, Psi, S) :-
    term_variables((E, NegPhi, Psi), Vars),
    names(Vars, 0, Names),
    compile_ewff(E, E_C),
    compile_clause(NegPhi, NegPhi_C),
    compile_clause(Psi, Psi_C),
    with_output_to(atom(S), write_term('belief_conds_append'('belief_conds', 'belief_cond_init'(E_C, NegPhi_C, Psi_C)), [variable_names(Names)])).

compile(S) :-
    box(Alpha),
    cnf(Alpha, Cnf),
    member(Clause, Cnf),
    ewffy(Clause, E1, C),
    sortify(E1, C, E),
    compile_box(E, C, S).
compile(S) :-
    static(Alpha),
    cnf(Alpha, Cnf),
    member(Clause, Cnf),
    ewffy(Clause, E1, C),
    sortify(E1, C, E),
    compile_static(E, C, S).
compile(S) :-
    belief(Phi => Psi),
    cnf(~Phi, NegPhiCnf),
    cnf(Psi, PsiCnf),
    member(NegPhiClause, NegPhiCnf),
    member(PsiClause, PsiCnf),
    ewffy(NegPhiClause, E11, C1),
    ewffy(PsiClause, E21, C2),
    sortify(E11, C1, E1),
    sortify(E21, C2, E2),
    append(E1, E2, E),
    compile_belief(E, C1, C2, S).

compile_all :-
    compile(S),
    write(S),
    write(';'),
    nl,
    fail.
compile_all.

