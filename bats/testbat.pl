% vim:filetype=prolog:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
%
% schwering@kbsg.rwth-aachen.de

sort_name(action, n).

box('SF'(A) <-> p) :- put_sort(A, action).

box(A:p <-> p) :- put_sort(A, action).

