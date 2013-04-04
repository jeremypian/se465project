CC=g++
CC_FLAGS= -O3

pipair: pipair.o
	$(CC) pipair.o -o pipair

pipair.o: pipair.cc
	$(CC) $(CC_FLAGS) -c pipair.cc

.PHONY: clean

clean:
	rm -f pipair.o pipair
