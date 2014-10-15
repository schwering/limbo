% vim:filetype=prolog:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
%
% Run this file with the following command:
%   $ eclipse-clp -f examples/kr2014.pl

% We just need that for the operator declarations:
:- ['../bats/proper-plus.pl'].

% Load the BAT shared library:
:- load('../bats/libBAT-kr2014.so').

% Load the ESBL interface:
:- load('../eclipse-clp/libEclipseESBL.so').
:- external(kcontext/1, p_kcontext).
:- external(bcontext/2, p_bcontext).
:- external(store_context/2, p_store_context).
:- external(retrieve_context/2, p_retrieve_context).
:- external(context_exec/3, p_context_exec).
:- external(context_entails/3, p_context_entails).

% Create a context and store it in an extra-logical variable:
:- kcontext(Ctx), store_context(myctx, Ctx).

% Now test the properties (some are taken from the KR-2014 paper, some are
% additional tests; they also match the kr2014.c example):
:- retrieve_context(myctx, Ctx), write('Testing property 0 ... '),          context_entails(Ctx, 0, ~d0 ^ ~d1),                   write('OK'), nl.
:- retrieve_context(myctx, Ctx), write('Testing property 1 ... '),          context_entails(Ctx, 0, ~(d0 v d1)),                  write('OK'), nl.
:- retrieve_context(myctx, Ctx), write('Testing property 2 ... '),          context_entails(Ctx, 1, forward : (d1 v d2)),         write('OK'), nl.
:- retrieve_context(myctx, Ctx), write('Testing property 3 ... '),          \+ context_entails(Ctx, 0, forward : (d1 v d2)),      write('OK'), nl.
:- retrieve_context(myctx, Ctx), write('Adding forward/true ... '),         context_exec(Ctx, forward, true),                    write('OK'), nl.
:- retrieve_context(myctx, Ctx), write('Adding sonar/true ... '),           context_exec(Ctx, sonar, true),                       write('OK'), nl.
:- retrieve_context(myctx, Ctx), write('Testing property 4 ... '),          context_entails(Ctx, 1, d0 v d1),                     write('OK'), nl.
:- retrieve_context(myctx, Ctx), write('Testing property 5 ... '),          \+ context_entails(Ctx, 1, d0),                       write('OK'), nl.
:- retrieve_context(myctx, Ctx), write('Testing property 6 ... '),          context_entails(Ctx, 1, d1),                          write('OK'), nl.
:- retrieve_context(myctx, Ctx), write('Testing property 7 ... '),          context_entails(Ctx, 1, sonar : (d0 v d1)),           write('OK'), nl.
:- retrieve_context(myctx, Ctx), write('Testing property 8 ... '),          context_entails(Ctx, 1, sonar : sonar : (d0 v d1)),   write('OK'), nl.
:- retrieve_context(myctx, Ctx), write('Testing property 9 ... '),          context_entails(Ctx, 1, forward : sonar : (d0 v d1)), write('OK'), nl.
:- retrieve_context(myctx, Ctx), write('Testing property 10 ... '),         context_entails(Ctx, 1, forward : forward : d0),      write('OK'), nl.
:- retrieve_context(myctx, Ctx), write('Testing a valid formula ... '),     context_entails(Ctx, 1, p v ~p),                      write('OK'), nl.
:- retrieve_context(myctx, Ctx), write('Testing a non-valid formula ... '), \+ context_entails(Ctx, 1, p v ~q),                   write('OK'), nl.

:- exit(0).

