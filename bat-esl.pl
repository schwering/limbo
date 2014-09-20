box('POSS'(A) <-> A = 'FORWARD' ^ ~ 'D(0)' v A = 'SONAR' ^ true) :- put_attr(A, sort, action).

box('SF'(A) <-> A = 'FORWARD' ^ true v A = 'SONAR' ^ ('D(0)' v 'D(1)')) :- put_attr(A, sort, action).

box(A:'D(0)' <-> A = 'FORWARD' ^ 'D(1)' v 'D(0)') :-
    put_attr(A, sort, action).
box(A:DI <-> A = 'FORWARD' ^ DI1 v A \= 'FORWARD' ^ DI) :-
    between(1, 3, I),
    I1 is I + 1,
    with_output_to(atom(DI), write_term('D'(I), [])),
    with_output_to(atom(DI1), write_term('D'(I1), [])),
    put_attr(A, sort, action).

static(~'D(0)').
static(~'D(1)').
static('D(2)' v 'D(3)').

belief(_ => _) :- fail.

