% vim:filetype=prolog:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
%
% Run this file with the following command:
%   $ eclipse-clp -f examples/ecai2014-reg.pl
%
% It is a copy of ecai2014.pl with which uses entailsreg/3 instead of entails/3,
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

% Initialize the context.
:- bcontext(ctx, 'ecai2014', 2).

% Now test the properties (matches the ECAI properties; Property 4 is left out;
% also matches the ecai2014.c example):
:- write('Testing property 1 ... '),   measure((entailsreg(ctx, ~'L1', 2))).
:- write('Testing property 2a ... '),  measure((\+ entailsreg(ctx, 'L1' ^ 'R1', 2))).
:- write('Adding \'SL\'/true ... '),   measure((add_sensing_result(ctx, [], 'SL', true))).
:- write('Testing property 2b ... '),  measure((entailsreg(ctx, 'SL' : ('L1' ^ 'R1'), 2))).
:- write('Testing property 3a ... '),  measure((\+ entailsreg(ctx, 'SL' : (~'R1'), 2))).
:- write('Adding \'SR1\'/false ... '), measure((add_sensing_result(ctx, ['SL'], 'SR1', false))).
:- write('Testing property 3b ... '),  measure((entailsreg(ctx, 'SL' : 'SR1' : (~'R1'), 2))).
:- write('Testing property 5a ... '),  measure((\+ entailsreg(ctx, 'SL' : 'SR1' : 'L1', 2))).
:- write('Testing property 5a ... '),  measure((\+ entailsreg(ctx, 'SL' : 'SR1' : (~'L1'), 2))).
:- write('Testing property 6a ... '),  measure((\+ entailsreg(ctx, 'SL' : 'SR1' : 'R1', 2))).
:- write('Adding \'LV\'/true ... '),   measure((add_sensing_result(ctx, ['SL', 'SR1'], 'LV', true))).
:- write('Testing property 6b ... '),  measure((entailsreg(ctx, 'SL' : 'SR1' : 'LV' : 'R1', 2))).
:- write('Adding \'SL\'/true ... '),   measure((add_sensing_result(ctx, ['SL', 'SR1', 'LV'], 'SL', true))).
:- write('Testing property 7 ... '),   measure((entailsreg(ctx, 'SL' : 'SR1' : 'LV' : 'SL' : 'R1', 2))).

:- exit(0).

