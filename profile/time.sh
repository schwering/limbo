gcc -std=gnu99 -O3 -Wall -Isrc/ -Itests/ src/*.c profile/profile.c -o profile/profile && /usr/bin/time profile/profile
