CC=mpicc
CFLAGS= -lm
DEPS = pool.h actors.h squirrel-functions.h ran2.h
OBJ = pool.o test.o actors.o squirrel-functions.o ran2.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

test: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o  test

