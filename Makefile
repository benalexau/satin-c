default: all

all: satin

clean:
	rm -f satin satin-regex m*.out p*.out

satin: src/satin.c
	gcc -O3 -Wall -ansi -pedantic src/satin.c -lm -lpthread -o satin

satin-regex: src/satin.c
	gcc -O3 -Wall -ansi -pedantic src/satin-regex.c -lm -lpthread -o satin-regex

bench: satin
	./satin -concurrent

bench-regex: satin-regex
	./satin-regex -concurrent