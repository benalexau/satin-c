default: all

all: satin

clean:
	rm -f satin m*.out p*.out

satin: src/satin.c
	gcc -O3 src/satin.c -lm -lpthread -o satin

bench: satin
	./satin -concurrent

