% vim:filetype=prolog:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
%
% Tool that generates a proper+ BAT represented in C code.
%
% The input BAT consists of Prolog clauses box/1, static/1, belief/1:
%
% For example, a sensing axiom can be written as
%   box('SF'(A) <-> A = sonar ^ (d1 v d0)).
% A successor state axiom can be written as
%   box(A:d0 <-> A = forward ^ d1 v d0).
%
% The operators `^', `v', `~' represent conjunction, disjunction, and negation.
% Implication and equivalence are written as `->' and `<->', and `true' can be
% used as well.
% The `box' indicates that the formula is surrounded by a Box operator in ES.
%
% The initial situation is axiomatized using `static' instead of `box':
%   static(d2 v d3).
%
% Belief conditionals as in 
%   belief(~d0 ^ ~d1 => d2 v d3).
% As in ESB, the `=>' is read counterfactually is no stand-alone logical
% operator but can only be used in connection as in belief(_ => _).
%
% Prolog variables in these Prolog clauses translate to ESBL variables. (Which
% are objects of type `var_t' in the generated C code. Their naming may differ,
% though, as they are called A, X, A1, X1, ...)
%
% Predicates and standard names (for actions or objects) need to be represented
% as Prolog terms which must be legal C identifiers as wells. For example,
% `'SF'' is the sensed fluent literal (which must be uppercase to conform with
% the ESBL constant defined in literal.h). Similarly, `forward' and `sonar'
% represent actions, and `d0', `d1', ... represent predicates. Observe that
% `d(0)' would not have worked, because it's no legal identifier in C.
%
% A BAT then consists of a bunch of Prolog clauses of the above format. Suppose
% a file `my-bat.pl' contains these clauses. Then the following command prints
% the BAT in proper+ form:
%   ?- print_all('bats/my-bat.pl').
% And this command generates the corresponding C code in the specified C header:
%   ?- compile_all('bats/my-bat.pl', 'my-generated-proper-plus-bat.h').
%
% It may improve performance to use different sorts for variables and standard
% names. For example, in the above sensed fluent axiom we can require `A' to be
% of sort `action':
%   box('SF'(A) <-> A = sonar ^ (d1 v d0)) :- put_sort(A, action).
% Then you need to adjust the disjunction in the generated C function
% `is_action': it must return true for only those standard names from the BAT
% which are of sort `action' and for those which have higher numbers than the
% names of the BAT (standard names of that kind will be used to deal with
% quantifiers).
%
% schwering@kbsg.rwth-aachen.de

:- module(properplus,
    [ op(820, fx, ~)    % Negation
    , op(840, xfy, ^)   % Conjunction
    , op(850, xfy, v)   % Disjunction
    , op(870, xfy, ->)  % Implication
    , op(880, xfy, <->) % Equivalence
    , op(890, xfy, =>)  % Belief conditional
    , put_sort/2
    , is_sort/2
    , print_all/1
    , compile_all/1
    , compile_all/4
    ]).

:- use_module(library(apply)).

% We misuse attributes to store the sort of a variable.
% That's not a good idea and incompatible with ECLiPSe-CLP (but it works with
% SWI), but I don't know any alternative right now.
put_sort(V, Sort) :- put_attr(V, properplus, Sort).
is_sort(V, Sort) :- get_attr(V, properplus, Sort).
attr_unify_hook(_, _).

:- dynamic(sort_name/2).
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
cnf(~(A1 <-> A2), B) :- !, cnf(~((A1 -> A2) ^ (A2 -> A1)), B).
cnf(~(A1 -> A2), B) :- !, cnf(A1 ^ ~A2, B).
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

replace(X, Y, T, Y) :- X == T, !.
replace(_, _, T, T) :- var(T), !.
replace(X, Y, T, T1) :- ( T = [] ; T = [_|_] ), !, maplist(replace(X, Y), T, T1).
replace(X, Y, T, T1) :- T =..[F|Ts], replace(X, Y, Ts, Ts1), T1 =..[F|Ts1].

dewffy([(X=Y)|Es], Ls, Es2, Ls2) :- var(X), !, replace(X, Y, Es, Es1), replace(X, Y, Ls, Ls1), dewffy(Es1, Ls1, Es2, Ls2).
dewffy([(X=Y)|Es], Ls, Es2, Ls2) :- var(Y), !, dewffy([(Y=X)|Es], Ls, Es2, Ls2).
dewffy([E|Es], Ls, [E|Es2], Ls2) :- dewffy(Es, Ls, Es2, Ls2).
dewffy([], Ls, [], Ls).

sortify([], []).
sortify([V|Vs], [sort(V, Sort)|E]) :- is_sort(V, Sort), !, sortify(Vs, E).
sortify([_|Vs], E) :- sortify(Vs, E).

sortify(E, C, E2) :-
    term_variables((E, C), Vars),
    sortify(Vars, E1),
    append(E1, E, E2).

ewff_sat_h([~(X=Y)|E], F) :- !, ewff_sat_h(E, [~(X=Y)|F]).
ewff_sat_h([(X=Y)|E], F) :- !, X = Y, ewff_sat_h(E, F).
ewff_sat_h([_|E], F) :- !, ewff_sat_h(E, F).
ewff_sat_h([], [~(X=Y)|E]) :- ( ground(X), ground(Y) ), !, X \== Y, ewff_sat_h([], E).
ewff_sat_h([], [~(_=_)|E]) :- !, ewff_sat_h([], E).
ewff_sat_h([], []) :- !.

ewff_sat(E) :- \+ \+ ewff_sat_h(E, []).

variable_names([], _, []).
variable_names([V|Vs], I, [N=V|Ns]) :- is_sort(V, action), !, ( I = 0 -> N = 'A' ; atom_concat('A', I, N) ), I1 is I + 1, variable_names(Vs, I1, Ns).
variable_names([V|Vs], I, [N=V|Ns]) :- ( I = 0 -> N = 'X' ; atom_concat('X', I, N) ), I1 is I + 1, variable_names(Vs, I1, Ns).

extract_variable_names([], []).
extract_variable_names([N=_|Mapping], [N|Names]) :- extract_variable_names(Mapping, Names).

extract_standard_names([], []).
extract_standard_names([sort(_,_)|Es], Names) :-
    extract_standard_names(Es, Names).
extract_standard_names([~(X=Y)|Es], Names) :-
    extract_standard_names([(X=Y)|Es], Names).
extract_standard_names([(X=Y)|Es], Names) :-
    ( ground(X), ground(Y) ->
        Names = [X,Y|Names1]
    ; ground(X) ->
        Names = [X|Names1]
    ; ground(Y) ->
        Names = [Y|Names1]
    ;
        Names = Names1
    ),
    extract_standard_names(Es, Names1).

extract_predicate_names([], []).
extract_predicate_names([~L|Ls], Ps) :- !, extract_predicate_names([L|Ls], Ps).
extract_predicate_names([_:L|Ls], Ps) :- !, extract_predicate_names([L|Ls], Ps).
extract_predicate_names([L|Ls], [P|Ps]) :- functor(L, P, _), extract_predicate_names(Ls, Ps).
extract_predicate_names((C1, C2), Ps) :- extract_predicate_names(C1, Ps1), extract_predicate_names(C2, Ps2), append(Ps1, Ps2, Ps).

compile_vars(_, [], '').
compile_vars(Names, [V|Vs], T) :-
    with_output_to(atom(VV), write_term(V, [variable_names(Names)])),
    compile_vars(Names, Vs, Ts),
    is_sort(V, Sort),
    with_output_to(atom(T), format('const Variable ~w = mutable_tf()->CreateVariable(~w); ~w', [VV,Sort,Ts])).


brace_list_h(_, [], '').
brace_list_h(Names, [T|Ts], T4) :-
    with_output_to(atom(T1), write_term(T, [variable_names(Names)])),
    ( Ts = [] -> T1 = T2 ; atom_concat(T1, ', ', T2) ),
    brace_list_h(Names, Ts, T3),
    atom_concat(T2, T3, T4).

brace_list(Names, Ls, T) :-
    brace_list_h(Names, Ls, T1),
    atom_concat('{', T1, T2),
    atom_concat(T2, '}', T).

compile_ewff2(_, [], []).
compile_ewff2(Names, [~(X=Y)|E], [T|Ts]) :-
    var(X), var(Y), !,
    with_output_to(atom(XX), write_term(X, [variable_names(Names)])),
    with_output_to(atom(YY), write_term(Y, [variable_names(Names)])),
    with_output_to(atom(T), format('{~w,~w}', [XX,YY])),
    compile_ewff2(Names, E, Ts).
compile_ewff2(Names, [_|E], Ts) :-
    compile_ewff2(Names, E, Ts).

compile_ewff1(_, [], []).
compile_ewff1(Names, [~(X=Y)|E], Ts) :-
    \+ var(X), var(Y), !,
    compile_ewff1(Names, [~(Y=X)|E], Ts).
compile_ewff1(Names, [~(X=Y)|E], [T|Ts]) :-
    var(X), \+ var(Y), !,
    with_output_to(atom(XX), write_term(X, [variable_names(Names)])),
    with_output_to(atom(YY), write_term(Y, [variable_names(Names)])),
    with_output_to(atom(T), format('{~w,~w}', [XX,YY])),
    compile_ewff1(Names, E, Ts).
compile_ewff1(Names, [_|E], Ts) :-
    compile_ewff1(Names, E, Ts).

compile_ewff(Names, E, C) :-
    compile_ewff1(Names, E, C1),
    compile_ewff2(Names, E, C2),
    brace_list(Names, C1, C3),
    brace_list(Names, C2, C4),
    with_output_to(atom(C), format('Ewff::Create(~w, ~w)', [C3, C4])).

literal_z_f_a(~L, As, F, Ts) :- !, literal_z_f_a(L, As, F, Ts).
literal_z_f_a(A:L, [A|As], F, Ts) :- !, literal_z_f_a(L, As, F, Ts).
literal_z_f_a(L, [], F, Ts) :- L =..[F|Ts].

compile_literal(Names, L, L_C) :-
    ( L  = (~_), L_C = 'Literal'(Actions, false, Symbol, Args)
    ; L \= (~_), L_C = 'Literal'(Actions, true, Symbol, Args)
    ),
    literal_z_f_a(L, ActionList, Symbol, ArgList),
    brace_list(Names, ActionList, Actions),
    brace_list(Names, ArgList, Args).

compile_literals(_, [], []).
compile_literals(Names, [L|Ls], [L_C|Ls_C]) :-
    compile_literal(Names, L, L_C),
    compile_literals(Names, Ls, Ls_C).

compile_clause(Names, C, 'SimpleClause'(C_C)) :-
    compile_literals(Names, C, Ls_C),
    brace_list(Names, Ls_C, C_C).

compile_formula(Names, Alpha, T) :-
    Alpha =..[Operator, Term1, Term2],
    member((Operator, Method), [((=), 'Eq'), ((\=), 'Neq')]), !,
    with_output_to(atom(T1), write_term(Term1, [variable_names(Names)])),
    with_output_to(atom(T2), write_term(Term2, [variable_names(Names)])),
    with_output_to(atom(T), format('Formula::~w(~w, ~w)', [Method, T1, T2])).
compile_formula(Names, Alpha, T) :-
    Alpha =..[Operator, Alpha1, Alpha2],
    member((Operator, Method), [(v, 'Or'), ((^), 'And'), ((->), 'OnlyIf'), ((<-), 'If'), ((<->), 'Iff')]), !,
    compile_formula(Names, Alpha1, T1),
    compile_formula(Names, Alpha2, T2),
    with_output_to(atom(T), format('Formula::~w(~w, ~w)', [Method, T1, T2])).
compile_formula(Names, Alpha, T) :-
    Alpha =..[Operator, Term1, Alpha2],
    member((Operator, Method), [(exists, 'Exists'), ((forall), 'Forall'), ((:), 'Act')]), !,
    with_output_to(atom(T1), write_term(Term1, [variable_names(Names)])),
    compile_formula(Names, Alpha2, T2),
    with_output_to(atom(T), format('Formula::~w(~w, ~w)', [Method, T1, T2])).
compile_formula(Names, Alpha, T) :-
    Alpha = (~Alpha1), !,
    compile_formula(Names, Alpha1, T1),
    with_output_to(atom(T), format('Formula::Neg(~w)', [T1])).
compile_formula(_, Alpha, T) :-
    Alpha = true, !,
    with_output_to(atom(T), format('Formula::True()', [])).
compile_formula(Names, Alpha, T) :-
    compile_literal(Names, Alpha, T1),
    with_output_to(atom(T), format('Formula::Lit(~w)', [T1])).

compile_box(E, Alpha, Code) :-
    term_variables((E, Alpha), Vars),
    variable_names(Vars, 0, Names),
    compile_vars(Names, Vars, Vars_C),
    compile_ewff(Names, E, E_C),
    compile_clause(Names, Alpha, Alpha_C),
    with_output_to(atom(Alpha_C1), write_term(Alpha_C, [variable_names(Names)])),
    with_output_to(atom(Code), format('{ ~wconst Maybe<Ewff> p = ~w; assert(p); const SimpleClause c = ~w; AddClause(Clause(true,p.val,c)); }', [Vars_C, E_C, Alpha_C1])).

compile_static(E, Alpha, Code) :-
    term_variables((E, Alpha), Vars),
    variable_names(Vars, 0, Names),
    compile_vars(Names, Vars, Vars_C),
    compile_ewff(Names, E, E_C),
    compile_clause(Names, Alpha, Alpha_C),
    with_output_to(atom(Alpha_C1), write_term(Alpha_C, [variable_names(Names)])),
    with_output_to(atom(Code), format('{ ~wconst Maybe<Ewff> p = ~w; assert(p); const SimpleClause c = ~w; AddClause(Clause(false,p.val,c)); }', [Vars_C, E_C, Alpha_C1])).

compile_belief(E, NegPhi, Psi, Code) :-
    term_variables((E, NegPhi, Psi), Vars),
    variable_names(Vars, 0, Names),
    compile_vars(Names, Vars, Vars_C),
    compile_ewff(Names, E, E_C),
    compile_clause(Names, NegPhi, NegPhi_C),
    compile_clause(Names, Psi, Psi_C),
    with_output_to(atom(Code), format('{ ~wconst Maybe<Ewff> p = ~w; assert(p); const SimpleClause neg_phi = ~w; const SimpleClause psi = ~w; AddBeliefConditional(Clause(p.val,neg_phi), Clause(p.val,psi), k); }', [Vars_C, E_C, NegPhi_C, Psi_C])).

compile(StdNames, PredNames, Code) :-
    box(Alpha),
    cnf(Alpha, Cnf),
    member(Clause, Cnf),
    ewffy(Clause, E1, C1),
    ewff_sat(E1),
    dewffy(E1, C1, E, C),
    extract_standard_names(E1, StdNames),
    extract_predicate_names(C, PredNames),
    compile_box(E, C, Code).
compile(StdNames, PredNames, Code) :-
    static(Alpha),
    cnf(Alpha, Cnf),
    member(Clause, Cnf),
    ewffy(Clause, E1, C1),
    ewff_sat(E1),
    dewffy(E1, C1, E, C),
    extract_standard_names(E1, StdNames),
    extract_predicate_names(C, PredNames),
    compile_static(E, C, Code).
compile(StdNames, PredNames, Code) :-
    belief(Phi => Psi),
    cnf(~Phi, NegPhiCnf),
    cnf(Psi, PsiCnf),
    member(NegPhiClause, NegPhiCnf),
    member(PsiClause, PsiCnf),
    ewffy(NegPhiClause, E11, C11),
    ewffy(PsiClause, E21, C21),
    ewff_sat(E11),
    ewff_sat(E21),
    dewffy(E11, C11, E1, C1),
    dewffy(E21, C21, E2, C2),
    append(E1, E2, E),
    extract_standard_names(E11, StdNames1),
    extract_standard_names(E21, StdNames2),
    append(StdNames1, StdNames2, StdNames),
    extract_predicate_names((C1, C2), PredNames),
    compile_belief(E, C1, C2, Code).

declare_sorts(_, []).
declare_sorts(Stream, [N|Ns]) :-
    length(Ns, I),
    I1 is I + 1,
    format(Stream, '  static constexpr Term::Sort ~w = ~w;~n', [N, I1]),
    declare_sorts(Stream, Ns).

define_sorts(_, _, []).
define_sorts(Stream, Class, [N|Ns]) :-
    format(Stream, 'constexpr Term::Sort ~w::~w;~n', [Class, N]),
    define_sorts(Stream, Class, Ns).

declare_standard_names(_, []).
declare_standard_names(Stream, [N|Ns]) :-
    format(Stream, '  const StdName ~w;~n', [N]),
    declare_standard_names(Stream, Ns).

declare_constructor(Stream, Class, 'KBat') :-
    format(Stream, '  ~w();~n', [Class]).
declare_constructor(Stream, Class, 'BBat') :-
    format(Stream, '  ~w(Setups::split_level k);~n', [Class]).

begin_constructor(Stream, Class, 'KBat') :-
    format(Stream, '~w::~w()~n', [Class,Class]).
begin_constructor(Stream, Class, 'BBat') :-
    format(Stream, '~w::~w(Setups::split_level k)~n', [Class,Class]).


init_standard_names(_, [], 0).
init_standard_names(Stream, [N|Ns], MaxStdName) :-
    length(Ns, I),
    I1 is I + 1,
    sort_name(Sort, N),
    format(Stream, '    , ~w(mutable_tf()->CreateStdName(~w, ~w))~n', [N, I1, Sort]),
    init_standard_names(Stream, Ns, MaxStdName1),
    MaxStdName is max(I1, MaxStdName1).

init_maps(Stream, StdNames, PredNames, SortNames) :-
    format(Stream, '  {~n', []),
    format(Stream, '    auto& map = name_to_string_;~n', []),
    maplist(print_serialization(Stream), StdNames),
    format(Stream, '  }~n', []),
    format(Stream, '~n', []),
    format(Stream, '  {~n', []),
    format(Stream, '    auto& map = pred_to_string_;~n', []),
    maplist(print_serialization(Stream), PredNames),
    format(Stream, '  }~n', []),
    format(Stream, '~n', []),
    format(Stream, '  {~n', []),
    format(Stream, '    auto& map = sort_to_string_;~n', []),
    maplist(print_serialization(Stream), SortNames),
    format(Stream, '  }~n', []),
    format(Stream, '~n', []),
    format(Stream, '  {~n', []),
    format(Stream, '    auto& map = string_to_name_;~n', []),
    maplist(print_deserialization(Stream), StdNames),
    format(Stream, '  }~n', []),
    format(Stream, '~n', []),
    format(Stream, '  {~n', []),
    format(Stream, '    auto& map = string_to_pred_;~n', []),
    maplist(print_deserialization(Stream), PredNames),
    format(Stream, '  }~n', []),
    format(Stream, '~n', []),
    format(Stream, '  {~n', []),
    format(Stream, '    auto& map = string_to_sort_;~n', []),
    maplist(print_deserialization(Stream), SortNames),
    format(Stream, '  }~n', []).

declare_and_define_max_stdname_function(Stream, MaxStdName) :-
    format(Stream, '  Term::Id max_std_name() const override { return ~w; }~n', [MaxStdName]).

declare_and_define_max_pred_function(Stream, MaxPred) :-
    format(Stream, '  Atom::PredId max_pred() const override { return ~w; }~n', [MaxPred]).

declare_predicate_names(_, [], N, N).
declare_predicate_names(Stream, [P|Ps], N, N2) :-
    ( P = 'SF' ->
        Id = 'Atom::SF',
        N1 is N
    ; P = 'POSS' ->
        Id = 'Atom::POSS',
        N1 is N
    ;
        Id = N,
        N1 is N + 1
    ),
    format(Stream, '  static constexpr Atom::PredId ~w = ~w;~n', [P, Id]),
    declare_predicate_names(Stream, Ps, N1, N2).

declare_predicate_names(Stream, Ps, MaxPred) :-
    declare_predicate_names(Stream, Ps, 0, MaxPred).

define_predicate_names(_, _, []).
define_predicate_names(Stream, Class, [P|Ps]) :-
    format(Stream, 'constexpr Atom::PredId ~w::~w;~n', [Class, P]),
    define_predicate_names(Stream, Class, Ps).

init_clauses(_, []).
%init_clauses(Stream, [C]) :- !, format(Stream, '  ~w;~n', [C]).
init_clauses(Stream, [C|Cs]) :- format(Stream, '  ~w;~n', [C]), init_clauses(Stream, Cs).

print_serialization(Stream, Val) :-
    format(Stream, '    map[~w] = "~w";~n', [Val, Val]).

print_deserialization(Stream, Val) :-
    format(Stream, '    map["~w"] = ~w;~n', [Val, Val]).

literal_of(_:A, Lit) :- !, literal_of(A, Lit).
literal_of(A, Lit) :- A =..[Lit|_].

define_regression_step(_, _, []).
define_regression_step(BodyStream, Class, [(Lhs,Rhs)|Boxes]) :-
    literal_of(Lhs, Lit),
    term_variables((Lhs, Rhs), Vars),
    variable_names(Vars, 0, Names),
    compile_vars(Names, Vars, VarsTerm),
    compile_literal(Names, Lhs, Lhs_C),
    compile_formula(Names, Rhs, Rhs_C),
    with_output_to(atom(Lhs_Readable), write_term(Lhs, [variable_names(Names)])),
    with_output_to(atom(Rhs_Readable), write_term(Rhs, [variable_names(Names)])),
    format(BodyStream, '  // ~w <-> ~w~n', [Lhs_Readable, Rhs_Readable]),
    format(BodyStream, '  if (_a.pred() == ~w) {~n', [Lit]),
    ( VarsTerm \= '' -> format(BodyStream, '    ~w~n', [VarsTerm]) ; true ),
    format(BodyStream, '    Atom _lhs = ~w;~n', [Lhs_C]),
    format(BodyStream, '    const Maybe<TermSeq> _prefix = _a.z().WithoutLast(_lhs.z().size());~n', []),
    format(BodyStream, '    if (_prefix) {~n', []),
    format(BodyStream, '      _lhs = _lhs.PrependActions(_prefix.val);~n', [VarsTerm]),
    format(BodyStream, '      Unifier _theta;~n', []),
    format(BodyStream, '      if (_a.Matches(_lhs, &_theta)) {~n', []),
    format(BodyStream, '        Formula::ObjPtr rhs(~w);~n', [Rhs_C]),
    format(BodyStream, '        rhs->SubstituteInPlace(_theta);~n', [Rhs_C]),
    format(BodyStream, '        rhs->PrependActions(_prefix.val);~n', [Rhs_C]),
    format(BodyStream, '        return Just(std::move(rhs));~n', []),
    format(BodyStream, '      }~n', []),
    format(BodyStream, '    }~n', []),
    format(BodyStream, '  }~n', []),
    define_regression_step(BodyStream, Class, Boxes).

compile_all(Class, Input, Header, Body) :-
    ( Header = stdout ->
        current_output(HeaderStream)
    ;
        open(Header, write, HeaderStream)
    ),
    ( Body = stdout ->
        current_output(BodyStream)
    ;
        open(Body, write, BodyStream)
    ),
    retractall(sort_name(_, _)),
    retractall(box(_)),
    retractall(static(_)),
    retractall(belief(_)),
    [Input],
    findall(r(StdNames, PredNames, Code), compile(StdNames, PredNames, Code), All),
    maplist(arg(1), All, StdNames1),
    maplist(arg(2), All, PredNames1),
    maplist(arg(3), All, Code),
    flatten(StdNames1, StdNames2),
    flatten(PredNames1, PredNames2),
    sort(StdNames2, StdNames),
    sort(PredNames2, PredNames),
    findall(SortName, sort_name(SortName, _), SortNames1),
    sort(SortNames1, SortNames),
    findall((Lhs, Rhs), box(Lhs <-> Rhs), Boxes),
    ( belief(_) -> SuperClass = 'BBat' ; SuperClass = 'KBat' ),
    % body
    format(BodyStream, '#include "~w"~n', [Header]),
    format(BodyStream, '~n', []),
    format(BodyStream, 'namespace esbl {~n', []),
    format(BodyStream, '~n', []),
    format(BodyStream, 'namespace bats {~n', []),
    format(BodyStream, '~n', []),
    begin_constructor(BodyStream, Class, SuperClass),
    format(BodyStream, '    : ~w()~n', [SuperClass]),
    init_standard_names(BodyStream, StdNames, MaxStdName),
    format(BodyStream, '{~n', []),
    init_clauses(BodyStream, Code),
    init_maps(BodyStream, StdNames, PredNames, SortNames),
    format(BodyStream, '}~n', []),
    format(BodyStream, '~n', []),
    define_sorts(BodyStream, Class, SortNames),
    format(BodyStream, '~n', []),
    define_predicate_names(BodyStream, Class, PredNames),
    format(BodyStream, '~n', []),
    format(BodyStream, 'Maybe<Formula::ObjPtr> ~w::RegressOneStep(const Atom& _a) {~n', [Class]),
    define_regression_step(BodyStream, Class, Boxes),
    format(BodyStream, '  return Nothing;~n', []),
    format(BodyStream, '}~n', []),
    format(BodyStream, '~n', []),
    format(BodyStream, '}  // namespace bats~n', []),
    format(BodyStream, '~n', []),
    format(BodyStream, '}  // namespace esbl~n', []),
    format(BodyStream, '~n', []),
    % header
    format(HeaderStream, '#ifndef BATS_~w_H_~n', [Class]),
    format(HeaderStream, '~n', []),
    format(HeaderStream, '#include <setup.h>~n', []),
    format(HeaderStream, '#include <term.h>~n', []),
    format(HeaderStream, '#include "bat.h"~n', []),
    format(HeaderStream, '~n', []),
    format(HeaderStream, 'namespace esbl {~n', []),
    format(HeaderStream, '~n', []),
    format(HeaderStream, 'namespace bats {~n', []),
    format(HeaderStream, '~n', []),
    format(HeaderStream, 'class ~w : public ~w {~n', [Class, SuperClass]),
    format(HeaderStream, ' public:~n', []),
    declare_predicate_names(HeaderStream, PredNames, MaxPred),
    format(HeaderStream, '~n', []),
    declare_sorts(HeaderStream, SortNames),
    format(HeaderStream, '~n', []),
    declare_constructor(HeaderStream, Class, SuperClass),
    format(HeaderStream, '~n', []),
    declare_standard_names(HeaderStream, StdNames),
    format(HeaderStream, '~n', []),
    declare_and_define_max_stdname_function(HeaderStream, MaxStdName),
    declare_and_define_max_pred_function(HeaderStream, MaxPred),
    format(HeaderStream, '  Maybe<Formula::ObjPtr> RegressOneStep(const Atom& _a) override;~n', []),
    format(HeaderStream, '};~n', []),
    format(HeaderStream, '~n', []),
    format(HeaderStream, '}  // namespace bats~n', []),
    format(HeaderStream, '~n', []),
    format(HeaderStream, '}  // namespace esbl~n', []),
    format(HeaderStream, '~n', []),
    format(HeaderStream, '#endif  // BATS_~w_H_~n', [Class]),
    ( Header = stdout ->
        true
    ;
        close(HeaderStream)
    ),
    ( Body = stdout ->
        true
    ;
        close(BodyStream)
    ).

first_char_uppercase(WordLC, WordUC) :-
    atom_chars(WordLC, [FirstChLow|LWordLC]),
    atom_chars(FirstLow, [FirstChLow]),
    lwrupr(FirstLow, FirstUpp),
    atom_chars(FirstUpp, [FirstChUpp]),
    atom_chars(WordUC, [FirstChUpp|LWordLC]).

lwrupr(Low, Upp) :- upcase_atom(Low, Upp).

compile_all(BAT) :-
    first_char_uppercase(BAT, Class),
    atom_concat(BAT, '.pl', Prolog),
    atom_concat(BAT, '.h', C_Header),
    atom_concat(BAT, '.cc', C_Body),
    compile_all(Class, Prolog, C_Header, C_Body).



cnf_to_ecnf([], []).
cnf_to_ecnf([C|Cs], ECs1) :- ewffy(C, E1, EC), sortify(E1, EC, E), ( (member((X1=Y1), E), member(~(X2=Y2), E), X1 == X2, Y1 == Y2) -> ECs1 = ECs ; ECs1 = [(E->EC)|ECs] ), cnf_to_ecnf(Cs, ECs).

print_ecnf(ECnf) :-
    term_variables(ECnf, Vars),
    variable_names(Vars, 0, Names),
    write_term(ECnf, [variable_names(Names), nl(true)]), nl.

print_to_term('v', [], '~true').
print_to_term(_, '^', [], 'true').
print_to_term(Names, _, [X], T) :- !, with_output_to(atom(T), write_term(X, [variable_names(Names)])).
print_to_term(Names, Op, [X|Xs], T) :- print_to_term(Names, Op, Xs, Ts), with_output_to(atom(T1), write_term(X, [variable_names(Names)])), with_output_to(atom(T), format('~w ~w ~w', [T1, Op, Ts])).

print_all(Input) :-
    retractall(sort_name(_, _)),
    retractall(box(_)),
    retractall(static(_)),
    retractall(belief(_)),
    [Input],
    (   box(Alpha),
        print_ecnf(box(Alpha)),
        cnf(Alpha, Cnf), member(Clause, Cnf),
        ewffy(Clause, E1, C1),
        ewff_sat(E1),
        dewffy(E1, C1, E, C),
        term_variables((E, C), Vars),
        variable_names(Vars, 0, Names),
        print_to_term(Names, '^', E, ETerm),
        print_to_term(Names, 'v', C, CTerm),
        Term = box(ETerm -> CTerm)
    ;   static(Alpha),
        print_ecnf(Alpha),
        cnf(Alpha, Cnf), member(Clause, Cnf),
        ewffy(Clause, E1, C1),
        ewff_sat(E1),
        dewffy(E1, C1, E, C),
        term_variables((E, C), Vars),
        variable_names(Vars, 0, Names),
        print_to_term(Names, '^', E, ETerm),
        print_to_term(Names, 'v', C, CTerm),
        Term = (ETerm -> CTerm)
    ;   belief(Phi => Psi),
        print_ecnf(Phi => Psi),
        cnf(~Phi, NegPhiCnf), cnf(Psi, PsiCnf),
        member(NegPhiClause, NegPhiCnf),
        member(PsiClause, PsiCnf),
        ewffy(NegPhiClause, E11, C11),
        ewffy(PsiClause, E21, C21),
        ewff_sat(E11),
        ewff_sat(E21),
        dewffy(E11, C11, E1, C1),
        dewffy(E21, C21, E2, C2),
        append(E1, E2, E),
        term_variables((E, Phi, Psi), Vars),
        variable_names(Vars, 0, Names),
        print_to_term(Names, '^', E, ETerm),
        print_to_term(Names, 'v', C1, CTerm1),
        print_to_term(Names, 'v', C2, CTerm2),
        Term = (ETerm -> (CTerm1 => CTerm2))
    ),
    write('  '),
    write_term(Term, [variable_names(Names), nl(true)]), nl,
    fail.
print_all(_).

