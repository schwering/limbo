% vim:filetype=prolog:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
%
% schwering@kbsg.rwth-aachen.de

:- ['proper-plus.pl'].

sublist([], _).
sublist([X|XS], [X|XSS]) :- sublist(XS, XSS).
sublist([X|XS], [_|XSS]) :- sublist([X|XS], XSS).

nat_point((X,Y)) :- between(0,9,X), between(0,9,Y).

nat_neighbor((X,Y), (X1,Y1)) :- X1 is X - 1, Y1 is Y + 1, X1 >= 0, Y1 >= 0.
nat_neighbor((X,Y), (X1,Y1)) :- X1 is X,     Y1 is Y + 1, X1 >= 0, Y1 >= 0.
nat_neighbor((X,Y), (X1,Y1)) :- X1 is X + 1, Y1 is Y + 1, X1 >= 0, Y1 >= 0.
nat_neighbor((X,Y), (X1,Y1)) :- X1 is X + 1, Y1 is Y,     X1 >= 0, Y1 >= 0.
nat_neighbor((X,Y), (X1,Y1)) :- X1 is X + 1, Y1 is Y - 1, X1 >= 0, Y1 >= 0.
nat_neighbor((X,Y), (X1,Y1)) :- X1 is X,     Y1 is Y - 1, X1 >= 0, Y1 >= 0.
nat_neighbor((X,Y), (X1,Y1)) :- X1 is X - 1, Y1 is Y - 1, X1 >= 0, Y1 >= 0.
nat_neighbor((X,Y), (X1,Y1)) :- X1 is X - 1, Y1 is Y,     X1 >= 0, Y1 >= 0.

nat_bat((XN,YN), (X,Y)) :- nat_bat('x', XN, X), nat_bat('y', YN, Y).
nat_bat(Prefix, CN, C) :- between(0, 9, CN), atom_concat(Prefix, CN, C).

point((X,Y)) :- nat_bat((XN,YN), (X,Y)), nat_point((XN,YN)).
neighbor((X,Y), (X1,Y1)) :- nat_bat((XN,YN), (X,Y)), nat_neighbor((XN,YN), (XN1,YN1)), nat_bat((XN1,YN1), (X1,Y1)).

neighbors((X,Y), Ps) :- findall((X1,Y1), neighbor((X,Y), (X1,Y1)), Ps).

n_neighbors(N, P, Ps) :- length(Ps, N), neighbors(P, Ps1), sublist(Ps, Ps1).

action_name(explore(X,Y), A) :- between(0, 9, XN), between(0, 9, YN), nat_bat((XN,YN), (X,Y)), atom_concat('explore_', X, A1), atom_concat(A1, '_', A2), atom_concat(A2, Y, A).
action_name(sense_less(N,X,Y), A) :- between(1, 9, N), between(0, 9, XN), between(0, 9, YN), nat_bat((XN,YN), (X,Y)), atom_concat('sense_less_', N, A1), atom_concat(A1, '_', A2), atom_concat(A2, X, A3), atom_concat(A3, '_', A4), atom_concat(A4, Y, A).

%sort_name(action, A) :- action_name(_, A).
%sort_name(x, X) :- nat_bat('x', _, X).
%sort_name(y, Y) :- nat_bat('y', _, Y).

no_mine_at([], true).
no_mine_at([(X,Y)|Ps], ~mine(X,Y) ^ Phi) :- no_mine_at(Ps, Phi).

box(A:mine(X,Y) <-> mine(X,Y)) :- action_name(_, A), point((X,Y)).
box('SF'(A) <-> Phi) :- action_name(sense_less(N,X,Y), A), n_neighbors(N, (X,Y), Ps), no_mine_at(Ps, Phi).
box('SF'(A) <-> mine(X, Y)) :- action_name(explore(X,Y), A).

static(_) :- fail.

belief(_ => _) :- fail.

