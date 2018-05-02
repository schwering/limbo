valgrind --tool=callgrind "$@" && mv callgrind.out.* cg$(expr $(ls -1 cg* | wc -l) + 1)

