LASER = default
CFLAGS = -O3 -Wall -ansi -pedantic -D${LASER}

default: all

all: satin

clean:
	rm -f satin m*.out p*.out

satin: src/satin.c
	@echo 'Building target: $@'
	gcc ${CFLAGS} src/satin.c -lm -lpthread -o satin
	@echo 'Finished building target: $@'

bench: satin
	./satin -concurrent
