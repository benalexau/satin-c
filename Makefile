default: all

all: satin satinSingleThread

clean:
	rm -f satin satinSingleThread m*.out p*.out

satin: src/satin.c
	gcc src/satin.c -lm -lpthread -o satin

satinSingleThread: src/satinSingleThread.c
	gcc src/satinSingleThread.c -lm -o satinSingleThread

bench: satin satinSingleThread
	./satin
	./satinSingleThread
