% vim:filetype=prolog:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
%
% Running example from:
% Schwering, Lakemeyer: A Semantic Account of Iterated Belief Revision in the Situation Calculus, ECAI-2014
%
% schwering@kbsg.rwth-aachen.de

sort_name(action, 'LV').
sort_name(action, 'SL').
sort_name(action, 'SR1').

box('POSS'(A) <-> true) :- put_sort(A, action).

box('SF'(A) <-> A = 'SL' ^ 'L1' ^ 'R1' v
		A = 'SL' ^ 'L2' ^ ~'R1' v
		A = 'LV' v
		A = 'SR1' ^ 'R1') :- put_sort(A, action).
box(A:'R1' <-> ~'R1' ^ A = 'LV' v 'R1' ^ A \= 'LV') :- put_sort(A, action).
box(A:'L1' <-> 'L1') :- put_sort(A, action).
box(A:'L2' <-> 'L2') :- put_sort(A, action).

% real world would be
%static('L1').
%static('L2').
%static(~'R1').

belief(true => ~'L1' ^ 'R1').
belief('L1' => 'R1').
belief(~'R1' => ~'L2').

