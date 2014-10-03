% vim:filetype=prolog:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
%
% Run this file with the following command:
%   $ eclipse-clp -f examples/ecai2014.pl

% We just need that for the operator declarations:
:- ['../bats/proper-plus.pl'].

% Load the BAT shared library:
:- load('../bats/libBAT-ecai2014.so').

% Load the ESBL interface:
:- load('../eclipse-clp/libEclipseESBL.so').
:- external(kcontext/1, p_kcontext).
:- external(bcontext/2, p_bcontext).
:- external(store_context/2, p_store_context).
:- external(retrieve_context/2, p_retrieve_context).
:- external(context_exec/3, p_context_exec).
:- external(context_entails/3, p_context_entails).

% Create a context and store it in an extra-logical variable:
:- bcontext(2, Ctx), store_context(myctx, Ctx).

% Now test the properties (matches the ECAI properties; Property 4 is left out;
% also matches the ecai2014.c example):
:- retrieve_context(myctx, Ctx), write('Testing property 1 ... '),   context_entails(Ctx, 2, ~'L1'),          write('OK'), nl.
:- retrieve_context(myctx, Ctx), write('Testing property 2a ... '),  \+ context_entails(Ctx, 2, 'L1' ^ 'R1'), write('OK'), nl.
:- retrieve_context(myctx, Ctx), write('Adding \'SL\'/true ... '),   context_exec(Ctx, 'SL', true),           write('OK'), nl.
:- retrieve_context(myctx, Ctx), write('Testing property 2b ... '),  context_entails(Ctx, 2, 'L1' ^ 'R1'),    write('OK'), nl.
:- retrieve_context(myctx, Ctx), write('Testing property 3a ... '),  \+ context_entails(Ctx, 2, ~'R1'),       write('OK'), nl.
:- retrieve_context(myctx, Ctx), write('Adding \'SR1\'/false ... '), context_exec(Ctx, 'SR1', false),         write('OK'), nl.
:- retrieve_context(myctx, Ctx), write('Testing property 3b ... '),  context_entails(Ctx, 2, ~'R1'),          write('OK'), nl.
:- retrieve_context(myctx, Ctx), write('Testing property 5a ... '),  \+ context_entails(Ctx, 2, 'L1'),        write('OK'), nl.
:- retrieve_context(myctx, Ctx), write('Testing property 5a ... '),  \+ context_entails(Ctx, 2, ~'L1'),       write('OK'), nl.
:- retrieve_context(myctx, Ctx), write('Testing property 6a ... '),  \+ context_entails(Ctx, 2, 'R1'),        write('OK'), nl.
:- retrieve_context(myctx, Ctx), write('Adding \'LV\'/true ... '),   context_exec(Ctx, 'LV', true),           write('OK'), nl.
:- retrieve_context(myctx, Ctx), write('Testing property 6b ... '),  context_entails(Ctx, 2, 'R1'),           write('OK'), nl.
:- retrieve_context(myctx, Ctx), write('Adding \'SL\'/true ... '),   context_exec(Ctx, 'SL', true),           write('OK'), nl.
:- retrieve_context(myctx, Ctx), write('Testing property 7 ... '),   context_entails(Ctx, 2, 'R1'),           write('OK'), nl.

:- exit(0).

