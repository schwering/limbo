% vim:filetype=prolog:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
%
% Run this file with the following command:
%   $ eclipse-clp -f examples/kitchen.pl
% or with regression
%   $ eclipse-clp -f examples/kitchen.pl -- regression

% We just need that for the operator declarations:
:- op(820, fx, ~).    % Negation
:- op(840, xfy, ^).   % Conjunction
:- op(850, xfy, v).   % Disjunction
:- op(870, xfy, ->).  % Implication
:- op(880, xfy, <->). % Equivalence
:- op(890, xfy, =>).  % Belief conditional

% Load the ESBL interface:
:- load('../eclipse-clp/libEclipseESBL.so').
:- external(kcontext/2, p_kcontext).
:- external(bcontext/3, p_bcontext).
:- external(register_pred/3, p_register_pred).
:- external(register_name/4, p_register_name).
:- external(enable_regression/1, p_enable_regression).
:- external(disable_regression/1, p_disable_regression).
:- external(is_regression/1, p_is_regression).
:- external(guarantee_consistency/2, p_guarantee_consistency).
:- external(add_sensing_result/4, p_add_sensing_result).
:- external(inconsistent/2, p_inconsistent).
:- external(entails/2, p_entails).

cmdarg(X) :- argv(all, Xs), member(X, Xs).

measure(Call) :-
    cputime(T0),
    call(Call),
    cputime(T1),
    T is T1 - T0,
    write('OK: '), writeln(T).

% For an explanation of the lines marked with ``XXX higher k than original'',
% see the message of commit c90cdaf. Briefly said, the problem is that (p v q)
% in ESL does not entail ([t]p v [t]q) because one may need to split multiple
% variables to trigger unit propagation with the SSAs.
% The only actually relevant line is marked with ``XXX XXX XXX''; in all other
% cases we achieve the same result for k = 1, too.
%
% With regression, the intended results obtain for k = 1 already.

:- kcontext(ctx, 'kitchen').
:-  ( (cmdarg("reg") ; cmdarg("regress") ; cmdarg("regression")) ->
        write('Enabling regression ... '), enable_regression(ctx)
    ;
        write('Disabling regression ... '), disable_regression(ctx)
    ),
    write('OK'), nl.

:-  measure((
    guarantee_consistency(ctx, 2), % XXX higher k than original
    entails(ctx, know(1, has_type(o1, boxA) v ~has_type(o1, boxA))),
    \+ entails(ctx, know(1, has_type(o0, boxA))),
    \+ entails(ctx, know(1, exists(X, type, has_type(o0, X)))),
    \+ entails(ctx, know(1, exists(X, type, ~has_type(o0, X)))),
    \+ entails(ctx, know(1, forall(X, type, has_type(o1, X)))),
    \+ entails(ctx, know(1, forall(X, type, ~has_type(o1, X)))),
    \+ entails(ctx, know(2, has_type(o1, boxA) v has_type(o1, boxB) v has_type(o1, boxC))), % XXX higher k than original
    \+ entails(ctx, know(2, has_type(o1, boxA))), % XXX higher k than original
    \+ entails(ctx, know(2, has_type(o1, boxB))), % XXX higher k than original
    \+ entails(ctx, know(2, has_type(o1, boxC))), % XXX higher k than original
    true)),

    measure((
    A1 = o0_has_type_boxA,
    add_sensing_result(ctx, [], A1, true),
    entails(ctx, A1 : know(1, has_type(o0, boxA))),
    entails(ctx, A1 : know(1, exists(X, type, has_type(o0, X)))),
    \+ entails(ctx, A1 : know(1, exists(X, type, ~has_type(o0, X)))),
    \+ entails(ctx, A1 : know(1, forall(X, type, has_type(o1, X)))),
    \+ entails(ctx, A1 : know(1, forall(X, type, ~has_type(o1, X)))),
    \+ entails(ctx, A1 : know(2, (has_type(o1, boxA) v has_type(o1, boxB) v has_type(o1, boxC)))), % XXX higher k than original
    \+ entails(ctx, A1 : know(2, has_type(o1, boxA))), % XXX higher k than original
    \+ entails(ctx, A1 : know(2, has_type(o1, boxB))), % XXX higher k than original
    \+ entails(ctx, A1 : know(2, has_type(o1, boxC))), % XXX higher k than original
    true)),
 
    measure((
    A2 = o1_is_box,
    add_sensing_result(ctx, [A1], A2, true),
    entails(ctx, A1 : A2 : know(1, has_type(o0, boxA))),
    entails(ctx, A1 : A2 : know(1, exists(X, type, has_type(o0, X)))),
    \+ entails(ctx, A1 : A2 : know(1, exists(X, type, ~has_type(o0, X)))),
    \+ entails(ctx, A1 : A2 : know(1, forall(X, type, has_type(o1, X)))),
    \+ entails(ctx, A1 : A2 : know(1, forall(X, type, ~has_type(o1, X)))),
    entails(ctx, A1 : A2 : know(2, (has_type(o1, boxA) v has_type(o1, boxB) v has_type(o1, boxC)))), % XXX XXX XXX higher k than original
    \+ entails(ctx, A1 : A2 : know(2, has_type(o1, boxA))), % XXX higher k than original
    \+ entails(ctx, A1 : A2 : know(2, has_type(o1, boxB))), % XXX higher k than original
    \+ entails(ctx, A1 : A2 : know(2, has_type(o1, boxC))), % XXX higher k than original
    true)),

    writeln('very good').

:- exit(0).

