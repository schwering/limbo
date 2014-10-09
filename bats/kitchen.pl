% vim:filetype=prolog:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
%
% Sadly, this is quite hacky, because we only have standard names (= constants)
% and cannot use higher-ary functions. Hence, we need to encode pickup(X) as
% pickup_X and cannot use Prolog unification.

sort_name(action, pickup_o1).
sort_name(action, pickup_o2).
sort_name(action, pickup_o3).
sort_name(action, drop_o1).
sort_name(action, drop_o2).
sort_name(action, drop_o3).
sort_name(action, sense_present_o1).
sort_name(action, sense_present_o2).
sort_name(action, sense_present_o3).
sort_name(action, sense_cup_o1).
sort_name(action, sense_cup_o2).
sort_name(action, sense_cup_o3).
sort_name(action, sense_mug_o1).
sort_name(action, sense_mug_o2).
sort_name(action, sense_mug_o3).

sort_name(object, o1).
sort_name(object, o2).
sort_name(object, o3).

sort_name(location, counter).
sort_name(location, table).

sort_name(type, cup).
sort_name(type, mug).

%box(
%    'POSS'(A) <-> A = sense_table ^ pos(table) v
%                  A = sense_counter ^ pos(counter) v
%                  A = sense_o1 v
%                  A = sense_o2 v
%                  A = sense_o3 v
%                  A = drop_o1 ^ (pos(L) <-> at(o1,L)) v
%                  A = drop_o2 ^ (pos(L) <-> at(o2,L)) v
%                  A = drop_o3 ^ (pos(L) <-> at(o3,L)) v
%                  A = pickup_o1 ^ (pos(L) <-> at(o1,L)) v
%                  A = pickup_o2 ^ (pos(L) <-> at(o2,L)) v
%                  A = pickup_o3 ^ (pos(L) <-> at(o3,L))
%) :- put_sort(A, action),
%     put_sort(L, location).

% Saves several thousand clauses:
box(A = sense_present_o1                                              -> ('POSS'(A) <-> true)) :- put_sort(A, action).
box(A = sense_present_o2                                              -> ('POSS'(A) <-> true)) :- put_sort(A, action).
box(A = sense_present_o3                                              -> ('POSS'(A) <-> true)) :- put_sort(A, action).
box(A = sense_cup_o1 v A = sense_mug_o1 v A = pickup_o1 v A = drop_o1 -> ('POSS'(A) <-> (pos(L) -> at(o1,L)))) :- put_sort(A, action), put_sort(L, location).
box(A = sense_cup_o2 v A = sense_mug_o2 v A = pickup_o2 v A = drop_o2 -> ('POSS'(A) <-> (pos(L) -> at(o2,L)))) :- put_sort(A, action), put_sort(L, location).
box(A = sense_cup_o3 v A = sense_mug_o3 v A = pickup_o3 v A = drop_o3 -> ('POSS'(A) <-> (pos(L) -> at(o3,L)))) :- put_sort(A, action), put_sort(L, location).

% Saves several thousand clauses:
box(A = sense_present_o1 -> ('SF'(A) <-> (pos(L) -> at(o1,L)))) :- put_sort(A, action), put_sort(L, location).
box(A = sense_present_o2 -> ('SF'(A) <-> (pos(L) -> at(o2,L)))) :- put_sort(A, action), put_sort(L, location).
box(A = sense_present_o3 -> ('SF'(A) <-> (pos(L) -> at(o3,L)))) :- put_sort(A, action), put_sort(L, location).
box(A = sense_cup_o1     -> ('SF'(A) <-> type(o1,cup))) :- put_sort(A, action).
box(A = sense_cup_o2     -> ('SF'(A) <-> type(o2,cup))) :- put_sort(A, action).
box(A = sense_cup_o3     -> ('SF'(A) <-> type(o3,cup))) :- put_sort(A, action).
box(A = sense_mug_o1     -> ('SF'(A) <-> type(o1,mug))) :- put_sort(A, action).
box(A = sense_mug_o2     -> ('SF'(A) <-> type(o2,mug))) :- put_sort(A, action).
box(A = sense_mug_o3     -> ('SF'(A) <-> type(o3,mug))) :- put_sort(A, action).

box(A:type(O,T) <-> type(O,T)) :- put_sort(A, action), put_sort(O, object), put_sort(T, type).

box(A:loc(L) <-> A = goto_table ^ L = table v
                 A = goto_counter ^ L = counter v
                 A \= goto_table ^ A \= goto_counter ^ pos(L)) :- put_sort(A, action), put_sort(L, location).

% Doesn't save any clauses:
%box(A = goto_table -> (A:loc(L) <-> L = table)) :- put_sort(A, action), put_sort(L, location).
%box(A = goto_counter -> (A:loc(L) <-> L = counter)) :- put_sort(A, action), put_sort(L, location).
%box(A \= goto_table ^ A \= goto_counter -> (A:loc(L) <-> pos(L))) :- put_sort(A, action), put_sort(L, location).

box(A:at(O,L) <-> (A = drop_o1 ^ O = o1 v
                   A = drop_o2 ^ O = o2 v
                   A = drop_o3 ^ O = o3) ^ pos(L) v
                  (A \= pickup_o1 ^ O = o1 v
                   A \= pickup_o2 ^ O = o2 v
                   A \= pickup_o3 ^ O = o3) ^ at(O,L)
) :- put_sort(A, action),
     put_sort(O, object),
     put_sort(L, location).

box(A:holding(O) <-> A = pickup_o1 ^ O = o1 v
                     A = pickup_o2 ^ O = o2 v
                     A = pickup_o3 ^ O = o3 v
                     A \= drop_o1 ^ O = o1 ^ holding(O) v
                     A \= drop_o2 ^ O = o2 ^ holding(O) v
                     A \= drop_o3 ^ O = o3 ^ holding(O)
) :- put_sort(A, action),
     put_sort(O, object).

% Costs about 300 clauses:
%box(A = drop_o1 -> (A:at(O,L) <-> O = o1 ^ pos(L) v at(O,L))) :- put_sort(A, action), put_sort(O, object), put_sort(L, location).
%box(A = drop_o2 -> (A:at(O,L) <-> O = o2 ^ pos(L) v at(O,L))) :- put_sort(A, action), put_sort(O, object), put_sort(L, location).
%box(A = drop_o3 -> (A:at(O,L) <-> O = o3 ^ pos(L) v at(O,L))) :- put_sort(A, action), put_sort(O, object), put_sort(L, location).
%box(A = pickup_o1 -> (A:at(O,L) <-> O \= o1 ^ at(O,L))) :- put_sort(A, action), put_sort(O, object), put_sort(L, location).
%box(A = pickup_o2 -> (A:at(O,L) <-> O \= o2 ^ at(O,L))) :- put_sort(A, action), put_sort(O, object), put_sort(L, location).
%box(A = pickup_o3 -> (A:at(O,L) <-> O \= o3 ^ at(O,L))) :- put_sort(A, action), put_sort(O, object), put_sort(L, location).
%box(A \= drop_o1 ^ A \= drop_o2 ^ A \= drop_o3 ^ A \= pickup_o1 ^ A \= pickup_o2 ^ A \= pickup_o3 -> (A:at(O,L) <-> at(O,L))) :- put_sort(A, action), put_sort(O, object), put_sort(L, location).

static(pos(table)).

belief(_ => _) :- fail.

