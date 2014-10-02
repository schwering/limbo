:- ['bats/proper-plus.pl'].
:- load('bats/libBAT-KR2014.so'), writeln('libBAT-KR2014 geladen').
:- load('eclipse-clp/libEclipseESBL.so'), writeln('libEclipseESBL geladen').
:- external(kcontext/1, p_kcontext), writeln('kcontext geladen').
:- external(bcontext/2, p_bcontext), writeln('bcontext geladen').
:- external(holds/3, p_holds).

%:- kcontext(Ctx), writeln(Ctx).
:- kcontext(Ctx), writeln(Ctx), setval(ctx, Ctx).
%:- getval(ctx, Ctx), is_handle(Ctx), writeln(Ctx).
:-
	%kcontext(Ctx),
	getval(ctx, Ctx),
	writeln(Ctx),
	writeln(holds),
	%holds(Ctx, 1, d2 v d3),
	holds(Ctx, 1, forward : (d1 v d2)),
	%holds(Ctx, 1, forward : (d1 v d2 v d3)),
	%holds(Ctx, 1, forward : d0 v d1 v d0 -> 'POSS'(forward) ^ forall(y, exists(x, 'POSS'(x) ^ 'POSS'(blabla) ^ 'SF'(y) v 'POSS'(blupp) ^ 'SF'(blabla) ^ 'SF'(blupp)))),
	writeln(held).

