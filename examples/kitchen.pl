% vim:filetype=prolog:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
%
% Run this file with the following command:
%   $ eclipse-clp -f examples/kr2014.pl

% We just need that for the operator declarations:
:- ['../bats/proper-plus.pl'].

% Load the BAT shared library:
:- load('../bats/libBAT-kitchen.so').

% Load the ESBL interface:
:- load('../eclipse-clp/libEclipseESBL.so').
:- external(kcontext/1, p_kcontext).
:- external(bcontext/2, p_bcontext).
:- external(context_store/2, p_context_store).
:- external(context_retrieve/2, p_context_retrieve).
:- external(context_guarantee_consistency/2, p_context_guarantee_consistency).
:- external(context_exec/3, p_context_exec).
:- external(context_entails/3, p_context_entails).

% Create a context and store it in an extra-logical variable:
:- kcontext(Ctx), context_guarantee_consistency(Ctx, 1), context_store(myctx, Ctx).

% Now test the properties (some are taken from the KR-2014 paper, some are
% additional tests; they also match the kr2014.c example):
test_tautology :-
    context_retrieve(myctx, Ctx),
    cputime(T0),
    context_entails(Ctx, 1, p v ~p),
    \+ context_entails(Ctx, 1, p v q),
    cputime(T1),
    T is T1 - T0,
    write('OK: '), writeln(T).

:- test_tautology.
:- test_tautology.

:- exit(0).

