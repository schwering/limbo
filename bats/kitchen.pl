% vim:filetype=prolog:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
%
% Sadly, this is quite hacky, because we only have standard names (= constants)
% and cannot use higher-ary functions. Hence, we need to encode pickup(X) as
% pickup_X and cannot use Prolog unification.

sense_is_object(O, A) :- sort_name(object, O), atom_concat(O, '_is_object', A).
sense_is_box(O, A) :- sort_name(object, O), atom_concat(O, '_is_box', A).
sense_has_type(O, T, A) :- sort_name(object, O), sort_name(type, T), atom_concat(O, '_has_type_', A1), atom_concat(A1, T, A).

sort_name(object, o0).
sort_name(object, o1).
%sort_name(object, o2).
%sort_name(object, o3).
%sort_name(object, o4).
%sort_name(object, o5).
%sort_name(object, o6).
%sort_name(object, o7).

sort_name(box, boxA).
sort_name(box, boxB).
sort_name(box, boxC).

sort_name(type, B) :- sort_name(box, B).

sort_name(action, A) :- sense_is_object(_, A).
sort_name(action, A) :- sense_is_box(_, A).
sort_name(action, A) :- sense_has_type(_, _, A).

is_one_of(_, [], ~true).
is_one_of(V, [T|Ts], V = T v Phi) :- is_one_of(V, Ts, Phi).

type_disj(_, [], ~true).
type_disj(O, [T|Ts], has_type(O,T) v Phi) :- type_disj(O, Ts, Phi).

box('SF'(A) <-> is_object(O)) :- sense_is_object(O, A).
box('SF'(A) <-> TypeDisj) :- sense_is_box(O, A), findall(Box, sort_name(box, Box), Boxes), type_disj(O, Boxes, TypeDisj).
box('SF'(A) <-> has_type(O, T)) :- sense_has_type(O, T, A).

box(A:is_object(O) <-> is_object(O)) :- put_sort(A, action), put_sort(O, object).
box(A:has_type(O,T) <-> has_type(O,T)) :- put_sort(A, action), put_sort(O, object), put_sort(T, type).

static(_) :- fail.

belief(_ => _) :- fail.

