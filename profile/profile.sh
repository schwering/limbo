gcc -std=gnu99 -O3 -Wall -pg src/*.c profile.c -o profile && ./profile && gprof profile | less
