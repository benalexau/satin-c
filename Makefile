LASER = default
CFLAGS = -O3 -Wall -std=c99 -pedantic -D${LASER}

default: all

all: satin

clean:
	rm -f satin m*.out p*.out

satin:
	@echo 'Building target: $@'
	gcc -o satin src/satin.c ${CFLAGS} -lm -lpthread 

bench: satin
	@echo Executing ...
	./satin
