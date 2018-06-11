F=sudoku
if [ "$1" != "" ]
then
    F=$1
fi
echo "Limbo / functional"
for f in $(ls -1 $F/fcnf-*); do cat $f | ./sat -e=0 -c=1 | grep -i satisfiable; done | ./sum.sh
echo "Limbo / propositional"
for f in $(ls -1 $F/cnf-*); do cat $f | ./sat -e=0 -c=1 | grep -i satisfiable; done | ./sum.sh
echo "MiniSAT / propositional"
for f in $(ls -1 $F/cnf-*); do cat $f | ../../../minisat/minisat | grep -i satisfiable; done | ./sum.sh

