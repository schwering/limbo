% vim:filetype=prolog:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
%
% Run this file with the following command:
%   $ eclipse-clp -f examples/ecai2014.pl

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

% Initialize the context.
:- bcontext(ctx, 'ecai2014', 2).

% Now test the properties (matches the ECAI properties; Property 4 is left out;
% also matches the ecai2014.c example):
:- write('Testing property 1 ... '),   entails(ctx, ~'L1', 2),                                   write('OK'), nl.
:- write('Testing property 2a ... '),  \+ entails(ctx, 'L1' ^ 'R1', 2),                          write('OK'), nl.
:- write('Adding \'SL\'/true ... '),   add_sensing_result(ctx, [], 'SL', true),                  write('OK'), nl.
:- write('Testing property 2b ... '),  entails(ctx, 'SL' : ('L1' ^ 'R1'), 2),                    write('OK'), nl.
:- write('Testing property 3a ... '),  \+ entails(ctx, 'SL' : (~'R1'), 2),                       write('OK'), nl.
:- write('Adding \'SR1\'/false ... '), add_sensing_result(ctx, ['SL'], 'SR1', false),            write('OK'), nl.
:- write('Testing property 3b ... '),  entails(ctx, 'SL' : 'SR1' : (~'R1'), 2),                  write('OK'), nl.
:- write('Testing property 5a ... '),  \+ entails(ctx, 'SL' : 'SR1' : 'L1', 2),                  write('OK'), nl.
:- write('Testing property 5a ... '),  \+ entails(ctx, 'SL' : 'SR1' : (~'L1'), 2),               write('OK'), nl.
:- write('Testing property 6a ... '),  \+ entails(ctx, 'SL' : 'SR1' : 'R1', 2),                  write('OK'), nl.
:- write('Adding \'LV\'/true ... '),   add_sensing_result(ctx, ['SL', 'SR1'], 'LV', true),       write('OK'), nl.
:- write('Testing property 6b ... '),  entails(ctx, 'SL' : 'SR1' : 'LV' : 'R1', 2),              write('OK'), nl.
:- write('Adding \'SL\'/true ... '),   add_sensing_result(ctx, ['SL', 'SR1', 'LV'], 'SL', true), write('OK'), nl.
:- write('Testing property 7 ... '),   entails(ctx, 'SL' : 'SR1' : 'LV' : 'SL' : 'R1', 2),       write('OK'), nl.

:- exit(0).

