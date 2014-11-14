% vim:filetype=prolog:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
%
% Run this file with the following command:
%   $ eclipse-clp -f examples/kr2014-reg.pl
%
% It is a copy of kr2014.pl with which uses entailsreg/3 instead of entails/3,
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

% Initialize the context.
:- kcontext(ctx, 'kr2014').

% Now test the properties (some are taken from the KR-2014 paper, some are
% additional tests; they also match the kr2014.c example):
:- write('Testing property 0 ... '),          entailsreg(ctx, ~d0 ^ ~d1, 0),                                     write('OK'), nl.
:- write('Testing property 1 ... '),          entailsreg(ctx, ~(d0 v d1), 0),                                    write('OK'), nl.
:- write('Testing property 2 ... '),          entailsreg(ctx, forward : (d1 v d2), 1),                           write('OK'), nl.
:- write('Testing property 3 ... '),          entailsreg(ctx, forward : (d1 v d2), 0),                        write('OK'), nl.
:- write('Adding forward/true ... '),         add_sensing_result(ctx, [], forward, true),                     write('OK'), nl.
:- write('Adding sonar/true ... '),           add_sensing_result(ctx, [forward], sonar, true),                write('OK'), nl.
:- write('Testing property 4 ... '),          entailsreg(ctx, forward : sonar : (d0 v d1), 1),                   write('OK'), nl.
:- write('Testing property 5 ... '),          \+ entailsreg(ctx, forward : sonar : d0, 1),                       write('OK'), nl.
:- write('Testing property 6 ... '),          entailsreg(ctx, forward : sonar : d1, 1),                          write('OK'), nl.
:- write('Testing property 7 ... '),          entailsreg(ctx, forward : sonar : sonar : (d0 v d1), 1),           write('OK'), nl.
:- write('Testing property 8 ... '),          entailsreg(ctx, forward : sonar : sonar : sonar : (d0 v d1), 1),   write('OK'), nl.
:- write('Testing property 9 ... '),          entailsreg(ctx, forward : sonar : forward : sonar : (d0 v d1), 1), write('OK'), nl.
:- write('Testing property 10 ... '),         entailsreg(ctx, forward : sonar : forward : forward : d0, 1),      write('OK'), nl.
:- write('Adding predicates p, q ... '),      register_pred(ctx, p, 100), register_pred(ctx, q, 101),         write('OK'), nl.
:- write('Testing a valid formula ... '),     entailsreg(ctx, (p v ~p), 1),                                      write('OK'), nl.
:- write('Testing a valid formula ... '),     entailsreg(ctx, sonar : (p v ~p), 1),                              write('OK'), nl.
:- write('Testing a non-valid formula ... '), \+ entailsreg(ctx, (p v ~q), 1),                                   write('OK'), nl.
:- write('Testing a non-valid formula ... '), \+ entailsreg(ctx, forward : sonar : (p v ~q), 1),                 write('OK'), nl.

:- exit(0).

