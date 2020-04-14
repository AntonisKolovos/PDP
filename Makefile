CC=mpicc
CFLAGS= -lm
DEPS = pool.h parameters.h squirrel-functions.h ran2.h clock.h grid.h squirrel.h
OBJ = pool.o main.o squirrel-functions.o ran2.o clock.o squirrel.o grid.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

simulation: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o  main

