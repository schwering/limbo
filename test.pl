:- ['bats/proper-plus.pl'].
:- load('bats/libBAT-KR2014.so'), writeln('libBAT-KR2014 geladen').
:- load('eclipse-clp/libEclipseESBL.so'), writeln('libEclipseESBL geladen').
:- external(kcontext/1, p_kcontext), writeln('kcontext geladen').
:- external(bcontext/2, p_bcontext), writeln('bcontext geladen').
:- external(executed/3, p_executed), writeln('executed geladen').
:- external(holds/3, p_holds).

%:- kcontext(Ctx), writeln(Ctx).
:- kcontext(Ctx), writeln(Ctx), setval(ctx, Ctx).
%:- getval(ctx, Ctx), is_handle(Ctx), writeln(Ctx).
:-
	kcontext(Ctx),
	%getval(ctx, Ctx),
	writeln(Ctx),
	writeln(holds),
	holds(Ctx, 0, ~d0 ^ ~d1), writeln('Property 0 OK'),
	holds(Ctx, 0, ~(d0 v d1)), writeln('Property 1 OK'),
	holds(Ctx, 1, forward : (d1 v d2)), writeln('Property 2 OK'),
	\+ holds(Ctx, 0, forward : (d1 v d2)), writeln('Property 3 OK'),
	executed(Ctx, forward, true),
	executed(Ctx, sonar, true),
	holds(Ctx, 1, d0 v d1), writeln('Property 4 OK'),
	\+ holds(Ctx, 1, d0), writeln('Property 5 OK'),
	holds(Ctx, 1, d1), writeln('Property 6 OK'),
	holds(Ctx, 1, sonar : (d0 v d1)), writeln('Property 7 OK'),
	holds(Ctx, 1, sonar : sonar : (d0 v d1)), writeln('Property 8 OK'),
	holds(Ctx, 1, forward : sonar : (d0 v d1)), writeln('Property 9 OK'),
	holds(Ctx, 1, forward : forward : d0), writeln('Property 10 OK').

