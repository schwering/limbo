F=sudokus
if [ "$1" != "" ]
then
    F=$1
fi
echo "Limbo / functional"
for f in $(ls -a1 $F | grep '^fcnf'); do cat $F/$f | ./sat -e=0 -c=1 | grep -i satisfiable; done | ./sum.sh
echo "Limbo / propositional"
for f in $(ls -a1 $F | grep '^cnf'); do cat $F/$f | ./sat -e=0 -c=1 | grep -i satisfiable; done | ./sum.sh
#echo "sat-cdcl / propositional"
#for f in $(ls -a1 $F | grep '\.cnf'); do ../../../2017-08-COMP4418/SAT/sat-cdcl $F/$f | grep -i satisfiable; done | ./sum.sh
#echo "MiniSAT / propositional"
#for f in $(ls -a1 $F | grep '\.cnf'); do cat $F/$f | ../../../Spielplatz/minisat/minisat | grep -i satisfiable; done | ./sum.sh

