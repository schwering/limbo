% vim:filetype=prolog:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
%
% Run this file with the following command:
%   $ eclipse-clp -f examples/ecai2014.pl
% or with regression
%   $ eclipse-clp -f examples/ecai2014.pl -- regression

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

% Initialize the context.
:- bcontext(ctx, 'ecai2014', 2).
:-  ( (cmdarg("reg") ; cmdarg("regress") ; cmdarg("regression")) ->
        write('Enabling regression ... '), enable_regression(ctx)
    ;
        write('Disabling regression ... '), disable_regression(ctx)
    ),
    write('OK'), nl.

% Now test the properties (matches the ECAI properties; Property 4 is left out;
% also matches the ecai2014.c example):
:- write('Testing property 1 ... '),   measure((entails(ctx, believe(2, ~'L1')))).
:- write('Testing property 2a ... '),  measure((\+ entails(ctx, believe(2, 'L1' ^ 'R1')))).
:- write('Adding \'SL\'/true ... '),   measure((add_sensing_result(ctx, [], 'SL', true))).
:- write('Testing property 2b ... '),  measure((entails(ctx, 'SL' : believe(2, 'L1' ^ 'R1')))).
:- write('Testing property 3a ... '),  measure((\+ entails(ctx, 'SL' : believe(2, ~'R1')))).
:- write('Adding \'SR1\'/false ... '), measure((add_sensing_result(ctx, ['SL'], 'SR1', false))).
:- write('Testing property 3b ... '),  measure((entails(ctx, 'SL' : 'SR1' : believe(2, ~'R1')))).
:- write('Testing property 5a ... '),  measure((\+ entails(ctx, 'SL' : 'SR1' : believe(2, 'L1')))).
:- write('Testing property 5a ... '),  measure((\+ entails(ctx, 'SL' : 'SR1' : believe(2, ~'L1')))).
:- write('Testing property 6a ... '),  measure((\+ entails(ctx, 'SL' : 'SR1' : believe(2, 'R1')))).
:- write('Adding \'LV\'/true ... '),   measure((add_sensing_result(ctx, ['SL', 'SR1'], 'LV', true))).
:- write('Testing property 6b ... '),  measure((entails(ctx, 'SL' : 'SR1' : 'LV' : believe(2, 'R1')))).
:- write('Adding \'SL\'/true ... '),   measure((add_sensing_result(ctx, ['SL', 'SR1', 'LV'], 'SL', true))).
:- write('Testing property 7 ... '),   measure((entails(ctx, 'SL' : 'SR1' : 'LV' : 'SL' : believe(2, 'R1')))).

:- exit(0).

