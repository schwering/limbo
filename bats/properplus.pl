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

declarations(E, Cs, VarNames, StdNames, PredNames) :-
    term_variables((E, Cs), Vars),
    variable_names(Vars, 0, Names),
    extract_variable_names(Names, VarNames),
    extract_standard_names(E, StdNames),
    ( Cs = (C1, C2) ->
        extract_predicate_names(C1, PredNames1),
        extract_predicate_names(C2, PredNames2),
        append(PredNames1, PredNames2, PredNames)
    ;
        C = Cs,
        extract_predicate_names(C, PredNames)
    ).

compile_vars(_, [], '').
compile_vars(Names, [V|Vs], T) :-
    with_output_to(atom(VV), write_term(V, [variable_names(Names)])),
    compile_vars(Names, Vs, Ts),
    is_sort(V, Sort),
    with_output_to(atom(T), format('const Variable ~w = tf_.CreateVariable(~w); ~w', [VV,Sort,Ts])).


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

compile_box(E, Alpha, Code) :-
    term_variables((E, Alpha), Vars),
    variable_names(Vars, 0, Names),
    compile_vars(Names, Vars, Vars_C),
    compile_ewff(Names, E, E_C),
    compile_clause(Names, Alpha, Alpha_C),
    with_output_to(atom(Alpha_C1), write_term(Alpha_C, [variable_names(Names)])),
    with_output_to(atom(Code), format('{ ~wconst std::pair<bool, Ewff> p = ~w; assert(p.first); const SimpleClause c = ~w; setup_.AddClause(Clause(true,p.second,c)); }', [Vars_C, E_C, Alpha_C1])).

compile_static(E, Alpha, Code) :-
    term_variables((E, Alpha), Vars),
    variable_names(Vars, 0, Names),
    compile_vars(Names, Vars, Vars_C),
    compile_ewff(Names, E, E_C),
    compile_clause(Names, Alpha, Alpha_C),
    with_output_to(atom(Alpha_C1), write_term(Alpha_C, [variable_names(Names)])),
    with_output_to(atom(Code), format('{ ~wconst std::pair<bool, Ewff> p = ~w; assert(p.first); const SimpleClause c = ~w; setup_.AddClause(Clause(false,p.second,c)); }', [Vars_C, E_C, Alpha_C1])).

compile_belief(E, NegPhi, Psi, Code) :-
    term_variables((E, NegPhi, Psi), Vars),
    variable_names(Vars, 0, Names),
    compile_ewff(Names, E, E_C),
    compile_clause(Names, NegPhi, NegPhi_C),
    compile_clause(Names, Psi, Psi_C),
    with_output_to(atom(Code), write_term('belief_conds_append'('belief_conds', 'belief_cond_init'(E_C, NegPhi_C, Psi_C)), [variable_names(Names)])).

compile(VarNames, StdNames, PredNames, Code) :-
    box(Alpha),
    cnf(Alpha, Cnf),
    member(Clause, Cnf),
    ewffy(Clause, E1, C1),
    ewff_sat(E1),
    dewffy(E1, C1, E, C),
    declarations(E, C, VarNames, StdNames, PredNames),
    compile_box(E, C, Code).
compile(VarNames, StdNames, PredNames, Code) :-
    static(Alpha),
    cnf(Alpha, Cnf),
    member(Clause, Cnf),
    ewffy(Clause, E1, C1),
    ewff_sat(E1),
    dewffy(E1, C1, E, C),
    declarations(E, C, VarNames, StdNames, PredNames),
    compile_static(E, C, Code).
compile(VarNames, StdNames, PredNames, Code) :-
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
    declarations(E, (C1, C2), VarNames, StdNames, PredNames),
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

declare_standard_names(_, [], 0).
declare_standard_names(Stream, [N|Ns], MaxStdName) :-
    length(Ns, I),
    I1 is I + 1,
    sort_name(Sort, N),
    format(Stream, '  const StdName ~w = tf_.CreateStdName(~w, ~w);~n', [N, I1, Sort]),
    declare_standard_names(Stream, Ns, MaxStdName1),
    MaxStdName is max(I1, MaxStdName1).

declare_and_define_max_stdname(Stream, MaxStdName) :-
    format(Stream, '  Term::Id max_std_name() const override { return ~w; }~n', [MaxStdName]).

declare_and_define_max_pred(Stream, MaxPred) :-
    format(Stream, '  Atom::PredId max_pred() const override { return ~w; }~n', [MaxPred]).

declare_predicate_name_declarations(_, [], 1).
declare_predicate_name_declarations(Stream, [P|Ps], MaxPred) :-
    length(Ps, I),
    ( P = 'SF' -> I1 = 'Atom::SF' ; I1 = I ),
    format(Stream, '  static constexpr Atom::PredId ~w = ~w;~n', [P, I1]),
    declare_predicate_name_declarations(Stream, Ps, MaxPred1),
    MaxPred is max(I, MaxPred1).

define_predicate_name_declarations(_, _, [], 1).
define_predicate_name_declarations(Stream, Class, [P|Ps], MaxPred) :-
    length(Ps, I),
    format(Stream, '  constexpr Atom::PredId ~w::~w;~n', [Class, P]),
    define_predicate_name_declarations(Stream, Class, Ps, MaxPred1),
    MaxPred is max(I, MaxPred1).

declare_variable_name_declarations(_, []).
declare_variable_name_declarations(Stream, [V|Vs]) :-
    length(Vs, I),
    I1 is -1 * (I + 1),
    format(Stream, 'static const var_t ~w = ~w;~n', [V, I1]),
    declare_variable_name_declarations(Stream, Vs).

define_functions(Stream, StdNames, PredNames) :-
    format(Stream, '  void InitNameToStringMap() override {~n', []),
    format(Stream, '    std::map<StdName, const char*>& map = name_to_string_;~n', []),
    maplist(print_serialization(Stream), StdNames),
    format(Stream, '  }~n', []),
    format(Stream, '~n', []),
    format(Stream, '  void InitPredToStringMap() override {~n', []),
    format(Stream, '    std::map<Atom::PredId, const char*>& map = pred_to_string_;~n', []),
    maplist(print_serialization(Stream), PredNames),
    format(Stream, '  }~n', []),
    format(Stream, '~n', []),
    format(Stream, '  void InitStringToNameMap() override {~n', []),
    format(Stream, '    std::map<std::string, StdName>& map = string_to_name_;~n', []),
    maplist(print_deserialization(Stream), StdNames),
    format(Stream, '  }~n', []),
    format(Stream, '~n', []),
    format(Stream, '  void InitStringToPredMap() override {~n', []),
    format(Stream, '    std::map<std::string, Atom::PredId>& map = string_to_pred_;~n', []),
    maplist(print_deserialization(Stream), PredNames),
    format(Stream, '  }~n', []).

define_clauses(_, []).
%define_clauses(Stream, [C]) :- !, format(Stream, '  ~w;~n', [C]).
define_clauses(Stream, [C|Cs]) :- format(Stream, '  ~w;~n', [C]), define_clauses(Stream, Cs).

print_serialization(Stream, Val) :-
    format(Stream, '    map[~w] = "~w";~n', [Val, Val]).

print_deserialization(Stream, Val) :-
    format(Stream, '    map["~w"] = ~w;~n', [Val, Val]).

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
    findall(r(VarNames, StdNames, PredNames, Code), compile(VarNames, StdNames, PredNames, Code), All),
    maplist(arg(1), All, VarNames1),
    maplist(arg(2), All, StdNames1),
    maplist(arg(3), All, PredNames1),
    maplist(arg(4), All, Code),
    flatten(VarNames1, VarNames2),
    flatten(StdNames1, StdNames2),
    flatten(PredNames1, PredNames2),
    sort(VarNames2, VarNames),
    sort(StdNames2, StdNames),
    sort(PredNames2, PredNames),
    findall(SortName, sort_name(SortName, _), SortNames1),
    sort(SortNames1, SortNames),
    % header
    format(HeaderStream, '#ifndef BATS_~w_H_~n', [Class]),
    format(HeaderStream, '~n', []),
    format(HeaderStream, '#include "bat.h"~n', []),
    format(HeaderStream, '~n', []),
    format(HeaderStream, 'namespace bats {~n', []),
    format(HeaderStream, '~n', []),
    format(HeaderStream, 'class ~w : public BAT {~n', [Class]),
    format(HeaderStream, ' public:~n', []),
    declare_sorts(HeaderStream, SortNames),
    format(HeaderStream, '~n', []),
    declare_standard_names(HeaderStream, StdNames, MaxStdName),
    format(HeaderStream, '~n', []),
    declare_predicate_name_declarations(HeaderStream, PredNames, MaxPred),
    format(HeaderStream, '~n', []),
    declare_and_define_max_stdname(HeaderStream, MaxStdName),
    declare_and_define_max_pred(HeaderStream, MaxPred),
    format(HeaderStream, '~n', []),
    format(HeaderStream, '  void InitSetup() override;~n', []),
    format(HeaderStream, '~n', []),
    format(HeaderStream, ' protected:~n', []),
    define_functions(HeaderStream, StdNames, PredNames),
    format(HeaderStream, '};~n', []),
    format(HeaderStream, '~n', []),
    format(HeaderStream, '}  // namespace bats~n', []),
    format(HeaderStream, '~n', []),
    format(HeaderStream, '#endif  // BATS_~w_H_~n', [Class]),
    % body
    format(BodyStream, '#include "~w"~n', [Header]),
    format(BodyStream, '~n', []),
    format(BodyStream, 'namespace bats {~n', []),
    format(BodyStream, '~n', []),
    define_sorts(BodyStream, Class, SortNames),
    format(BodyStream, '~n', []),
    define_predicate_name_declarations(BodyStream, Class, PredNames, MaxPred),
    format(BodyStream, '~n', []),
    format(BodyStream, 'void ~w::InitSetup() {~n', [Class]),
    define_clauses(BodyStream, Code),
    format(BodyStream, '  setup_.UpdateHPlus(tf_);~n', []),
    format(BodyStream, '}~n', []),
    format(BodyStream, '~n', []),
    format(BodyStream, '}  // namespace bats~n', []),
    format(HeaderStream, '~n', []),
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

compile_all(BAT) :-
    upcase_atom(BAT, Class),
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

