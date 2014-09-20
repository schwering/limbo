box('POSS'(A) <-> true) :- vsort(A, action).

box('SF'(A) <-> A = 'SL' ^ 'L1' ^ 'R1' v
		A = 'SL' ^ 'L2' ^ ~'R1' v
		A = 'LV' v
		A = 'SR1' ^ 'R1') :- vsort(A, action).
box(A:'R1' <-> ~'R1' ^ A = 'LV' v 'R1' ^ A \= 'LV') :- vsort(A, action).
box(A:'L1' <-> 'L1') :- vsort(A, action).
box(A:'L2' <-> 'L2') :- vsort(A, action).

%static('L1').
%static('L2').
%static(~'R1').

belief(true => ~'L1' ^ 'R1').
belief('L1' => 'R1').
belief(~'R1' => ~'L2').

