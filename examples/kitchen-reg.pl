% vim:filetype=prolog:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
%
% Run this file with the following command:
%   $ eclipse-clp -f examples/kitchen-reg.pl
%
% It is a copy of kitchen.pl with which uses entailsreg/3 instead of entails/3,
% i.e., regresses the query first.
% (As a side effect, property 3 comes out true.)

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
:- external(guarantee_consistency/2, p_guarantee_consistency).
:- external(add_sensing_result/4, p_add_sensing_result).
:- external(inconsistent/2, p_inconsistent).
:- external(entails/3, p_entails).
:- external(entailsreg/3, p_entailsreg).

measure(Call) :-
    cputime(T0),
    call(Call),
    cputime(T1),
    T is T1 - T0,
    write('OK: '), writeln(T).

%:- measure(Ctx, context_entailsreg(Ctx, 1, p v ~p)).
%:- measure(Ctx, \+ context_entailsreg(Ctx, 1, p v q)).
%:- measure(Ctx, context_entailsreg(Ctx, 1, p v q v ~(p ^ q))).
%:- measure(Ctx, context_entailsreg(Ctx, 1, p v ~(p ^ q) v q)).
%:- measure(Ctx, context_entailsreg(Ctx, 1, p(n) v ~p(n))). % triggers new grounding
%:- measure(Ctx, context_entailsreg(Ctx, 1, p(1) v ~p(1))).
%:- measure(Ctx, context_entailsreg(Ctx, 1, forall(X, p(X) v ~p(X)))).
%:- writeln(ok).


:-  profile((
    kcontext(ctx, 'kitchen'),
    guarantee_consistency(ctx, 2), % higher k than original
    entailsreg(ctx, has_type(o1, boxA) v ~has_type(o1, boxA), 1),
    \+ entailsreg(ctx, has_type(o0, boxA), 1),
    \+ entailsreg(ctx, exists(X, type, has_type(o0, X)), 1),
    \+ entailsreg(ctx, exists(X, type, ~has_type(o0, X)), 1),
    \+ entailsreg(ctx, forall(X, type, has_type(o1, X)), 1),
    \+ entailsreg(ctx, forall(X, type, ~has_type(o1, X)), 1),
    \+ entailsreg(ctx, has_type(o1, boxA) v has_type(o1, boxB) v has_type(o1, boxC), 2), % higher k than original
    \+ entailsreg(ctx, has_type(o1, boxA), 2), % higher k than original
    \+ entailsreg(ctx, has_type(o1, boxB), 2), % higher k than original
    \+ entailsreg(ctx, has_type(o1, boxC), 2), % higher k than original
    true)),

    profile((
    A1 = o0_has_type_boxA,
    add_sensing_result(ctx, [], A1, true),
    entailsreg(ctx, A1 : has_type(o0, boxA), 1),
    entailsreg(ctx, A1 : exists(X, type, has_type(o0, X)), 1),
    \+ entailsreg(ctx, A1 : exists(X, type, ~has_type(o0, X)), 1),
    \+ entailsreg(ctx, A1 : forall(X, type, has_type(o1, X)), 1),
    \+ entailsreg(ctx, A1 : forall(X, type, ~has_type(o1, X)), 1),
    \+ entailsreg(ctx, A1 : (has_type(o1, boxA) v has_type(o1, boxB) v has_type(o1, boxC)), 2), % higher k than original
    \+ entailsreg(ctx, A1 : has_type(o1, boxA), 2), % higher k than original
    \+ entailsreg(ctx, A1 : has_type(o1, boxB), 2), % higher k than original
    \+ entailsreg(ctx, A1 : has_type(o1, boxC), 2), % higher k than original
    true)),
 
    profile((
    A2 = o1_is_box,
    add_sensing_result(ctx, [A1], A2, true),
    entailsreg(ctx, A1 : A2 : has_type(o0, boxA), 1),
    entailsreg(ctx, A1 : A2 : exists(X, type, has_type(o0, X)), 1),
    \+ entailsreg(ctx, A1 : A2 : exists(X, type, ~has_type(o0, X)), 1),
    \+ entailsreg(ctx, A1 : A2 : forall(X, type, has_type(o1, X)), 1),
    \+ entailsreg(ctx, A1 : A2 : forall(X, type, ~has_type(o1, X)), 1),
    entailsreg(ctx, A1 : A2 : (has_type(o1, boxA) v has_type(o1, boxB) v has_type(o1, boxC)), 2), % higher k than original
    \+ entailsreg(ctx, A1 : A2 : has_type(o1, boxA), 2), % higher k than original
    \+ entailsreg(ctx, A1 : A2 : has_type(o1, boxB), 2), % higher k than original
    \+ entailsreg(ctx, A1 : A2 : has_type(o1, boxC), 2), % higher k than original
    true)),

    writeln('very good').


%:-  kcontext(ctx, 'kitchen'),
%    guarantee_consistency(ctx, 2),
%    A = o1_is_box,
%    add_sensing_result(ctx, [], A, true),
%    %entailsreg(ctx, A : (has_type(o1, boxA) v has_type(o1, boxB) v has_type(o1, boxC)), 1),
%    entailsreg(ctx, A : (has_type(o1, boxA) v has_type(o1, boxB) v has_type(o1, boxC)), 2),
%    writeln('very good').

:- exit(0).

