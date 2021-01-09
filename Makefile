HEAD = cachesim.h
OBJ  = cachesim.o
SRC  = cachesim.c

cachesim: ${OBJ}
	gcc -o cachesim ${OBJ}
	strip cachesim
	sleep 1
	touch ${SRC}

cachesim.o: cachesim.c ${HEAD}
	gcc -c -O cachesim.c
