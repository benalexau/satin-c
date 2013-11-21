default: all

all: satin satinSingleThread

clean:
	rm -f satin satinSingleThread m*.put p*.out

satin: satin.c
	gcc satin.c -lm -lpthread -o satin

satinSingleThread: satinSingleThread.c
	gcc satinSingleThread.c -lm -o satinSingleThread

bench: satin satinSingleThread
	./satin
	./satinSingleThread
