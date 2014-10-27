% vim:filetype=prolog:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
%
% Sadly, this is quite hacky, because we only have standard names (= constants)
% and cannot use higher-ary functions. Hence, we need to encode pickup(X) as
% pickup_X and cannot use Prolog unification.

sort_name(action, pickup_o1).
sort_name(action, pickup_o2).
sort_name(action, pickup_o3).
sort_name(action, pickup_o4).
sort_name(action, pickup_o5).
sort_name(action, drop_o1).
sort_name(action, drop_o2).
sort_name(action, drop_o3).
sort_name(action, drop_o4).
sort_name(action, drop_o5).
sort_name(action, sense_present_o1).
sort_name(action, sense_present_o2).
sort_name(action, sense_present_o3).
sort_name(action, sense_present_o4).
sort_name(action, sense_present_o5).
sort_name(action, sense_box1_o1).
sort_name(action, sense_box1_o2).
sort_name(action, sense_box1_o3).
sort_name(action, sense_box1_o4).
sort_name(action, sense_box1_o5).
sort_name(action, sense_box2_o1).
sort_name(action, sense_box2_o2).
sort_name(action, sense_box2_o3).
sort_name(action, sense_box2_o4).
sort_name(action, sense_box2_o5).

sort_name(object, o1).
sort_name(object, o2).
sort_name(object, o3).
sort_name(object, o4).
sort_name(object, o5).

sort_name(type, box1).
sort_name(type, box2).

%box(
%    'POSS'(A) <-> A = sense_table ^ robot_at_table v
%                  A = sense_counter ^ pos(counter) v
%                  A = sense_o1 v
%                  A = sense_o2 v
%                  A = sense_o3 v
%                  A = drop_o1 ^ (robot_at_table <-> object_at_table(o1)) v
%                  A = drop_o2 ^ (robot_at_table <-> object_at_table(o2)) v
%                  A = drop_o3 ^ (robot_at_table <-> object_at_table(o3)) v
%                  A = pickup_o1 ^ (robot_at_table <-> object_at_table(o1)) v
%                  A = pickup_o2 ^ (robot_at_table <-> object_at_table(o2)) v
%                  A = pickup_o3 ^ (robot_at_table <-> object_at_table(o3))
%) :- put_sort(A, action).

% Saves several thousand clauses:
box(A = sense_present_o1 v A = sense_present_o2 v A = sense_present_o3 v A = sense_present_o4 v A = sense_present_o5 -> ('POSS'(A) <-> true)) :- put_sort(A, action).
box(A = sense_box1_o1 v A = sense_box2_o1 v A = pickup_o1 v A = drop_o1 -> ('POSS'(A) <-> (robot_at_table -> object_at_table(o1)))) :- put_sort(A, action).
box(A = sense_box1_o2 v A = sense_box2_o2 v A = pickup_o2 v A = drop_o2 -> ('POSS'(A) <-> (robot_at_table -> object_at_table(o2)))) :- put_sort(A, action).
box(A = sense_box1_o3 v A = sense_box2_o3 v A = pickup_o3 v A = drop_o3 -> ('POSS'(A) <-> (robot_at_table -> object_at_table(o3)))) :- put_sort(A, action).
box(A = sense_box1_o4 v A = sense_box2_o4 v A = pickup_o4 v A = drop_o4 -> ('POSS'(A) <-> (robot_at_table -> object_at_table(o4)))) :- put_sort(A, action).
box(A = sense_box1_o5 v A = sense_box2_o5 v A = pickup_o5 v A = drop_o5 -> ('POSS'(A) <-> (robot_at_table -> object_at_table(o5)))) :- put_sort(A, action).

% Saves several thousand clauses:
box(A = sense_present_o1 -> ('SF'(A) <-> (robot_at_table -> object_at_table(o1)))) :- put_sort(A, action).
box(A = sense_present_o2 -> ('SF'(A) <-> (robot_at_table -> object_at_table(o2)))) :- put_sort(A, action).
box(A = sense_present_o3 -> ('SF'(A) <-> (robot_at_table -> object_at_table(o3)))) :- put_sort(A, action).
box(A = sense_present_o4 -> ('SF'(A) <-> (robot_at_table -> object_at_table(o4)))) :- put_sort(A, action).
box(A = sense_present_o5 -> ('SF'(A) <-> (robot_at_table -> object_at_table(o5)))) :- put_sort(A, action).
box(A = sense_box1_o1    -> ('SF'(A) <-> type(o1,box1))) :- put_sort(A, action).
box(A = sense_box1_o2    -> ('SF'(A) <-> type(o2,box1))) :- put_sort(A, action).
box(A = sense_box1_o3    -> ('SF'(A) <-> type(o3,box1))) :- put_sort(A, action).
box(A = sense_box1_o4    -> ('SF'(A) <-> type(o4,box1))) :- put_sort(A, action).
box(A = sense_box1_o5    -> ('SF'(A) <-> type(o5,box1))) :- put_sort(A, action).
box(A = sense_box2_o1    -> ('SF'(A) <-> type(o1,box2))) :- put_sort(A, action).
box(A = sense_box2_o2    -> ('SF'(A) <-> type(o2,box2))) :- put_sort(A, action).
box(A = sense_box2_o3    -> ('SF'(A) <-> type(o3,box2))) :- put_sort(A, action).
box(A = sense_box2_o4    -> ('SF'(A) <-> type(o4,box2))) :- put_sort(A, action).
box(A = sense_box2_o5    -> ('SF'(A) <-> type(o5,box2))) :- put_sort(A, action).

box(A:type(O,T) <-> type(O,T)) :- put_sort(A, action), put_sort(O, object), put_sort(T, type).

box(A:robot_at_table <-> A = goto_table v
                         A \= goto_table ^ robot_at_table) :- put_sort(A, action).

box(A:object_at_table(O) <-> (A = drop_o1 ^ O = o1 v
                              A = drop_o2 ^ O = o2 v
                              A = drop_o3 ^ O = o3 v
                              A = drop_o4 ^ O = o4 v
                              A = drop_o5 ^ O = o5) ^ robot_at_table v
                             (A \= pickup_o1 ^ O = o1 v
                              A \= pickup_o2 ^ O = o2 v
                              A \= pickup_o3 ^ O = o3 v
                              A \= pickup_o4 ^ O = o4 v
                              A \= pickup_o5 ^ O = o5) ^ object_at_table(O)
) :- put_sort(A, action),
     put_sort(O, object).

box(A:holding(O) <-> A = pickup_o1 ^ O = o1 v
                     A = pickup_o2 ^ O = o2 v
                     A = pickup_o3 ^ O = o3 v
                     A = pickup_o4 ^ O = o4 v
                     A = pickup_o5 ^ O = o5 v
                     A \= drop_o1 ^ O = o1 ^ holding(O) v
                     A \= drop_o2 ^ O = o2 ^ holding(O) v
                     A \= drop_o3 ^ O = o3 ^ holding(O) v
                     A \= drop_o4 ^ O = o4 ^ holding(O) v
                     A \= drop_o5 ^ O = o5 ^ holding(O)
) :- put_sort(A, action),
     put_sort(O, object).

% Costs about 300 clauses:
%box(A = drop_o1 -> (A:object_at_table(O) <-> O = o1 ^ robot_at_table v object_at_table(O))) :- put_sort(A, action), put_sort(O, object), put_sort(L, location).
%box(A = drop_o2 -> (A:object_at_table(O) <-> O = o2 ^ robot_at_table v object_at_table(O))) :- put_sort(A, action), put_sort(O, object), put_sort(L, location).
%box(A = drop_o3 -> (A:object_at_table(O) <-> O = o3 ^ robot_at_table v object_at_table(O))) :- put_sort(A, action), put_sort(O, object), put_sort(L, location).
%box(A = pickup_o1 -> (A:object_at_table(O) <-> O \= o1 ^ object_at_table(O))) :- put_sort(A, action), put_sort(O, object), put_sort(L, location).
%box(A = pickup_o2 -> (A:object_at_table(O) <-> O \= o2 ^ object_at_table(O))) :- put_sort(A, action), put_sort(O, object), put_sort(L, location).
%box(A = pickup_o3 -> (A:object_at_table(O) <-> O \= o3 ^ object_at_table(O))) :- put_sort(A, action), put_sort(O, object), put_sort(L, location).
%box(A \= drop_o1 ^ A \= drop_o2 ^ A \= drop_o3 ^ A \= pickup_o1 ^ A \= pickup_o2 ^ A \= pickup_o3 -> (A:object_at_table(O) <-> object_at_table(O))) :- put_sort(A, action), put_sort(O, object), put_sort(L, location).

static(robot_at_table).

belief(_ => _) :- fail.

