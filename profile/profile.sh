gcc -std=gnu99 -O3 -Wall -pg -Isrc/ -Itests/ src/*.c profile/profile.c -o profile/profile && profile/profile && gprof profile/profile | less
