CFLAGS = -O3 -Wall -std=c17 -pedantic -static

default: bench

all: satin

clean:
	rm -f satin m*.out p*.out

satin: clean
	@echo 'Building target: $@'
	gcc -o satin src/satin.c ${CFLAGS} -lm -lpthread

bench: satin
	@echo Executing ...
	./satin

