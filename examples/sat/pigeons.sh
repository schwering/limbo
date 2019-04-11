#echo "Limbo / propositional"
#for i in $(seq 1 9); do f=pigeon/cnf-$i; echo -n "$f: " && cat $f | ./sat | grep -i satisfiable; done
echo "Limbo / propositional functionalized"
for i in $(seq 1 9); do f=pigeon/cnf-$i; echo -n "$f: " && cat $f | ./functionalize | ./sat | grep -i satisfiable; done
echo "Limbo / functional"
for i in $(seq 1 11); do f=pigeon/fcnf-$i; echo -n "$f: " && cat $f | ./sat | grep -i satisfiable; done
echo "sat-cdcl / propositional"
for i in $(seq 1 11); do f=pigeon/cnf-$i; echo -n "$f: " && ../../../2017-08-COMP4418/SAT/sat-cdcl $f | grep -i satisfiable; done
echo "MiniSAT / propositional"
for i in $(seq 1 11); do f=pigeon/cnf-$i; echo -n "$f: " && cat $f | ../../../Spielplatz/minisat/minisat | grep -i satisfiable; done

