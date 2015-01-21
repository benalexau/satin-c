LASER = default
CFLAGS = -O3 -Wall -std=c99 -pedantic -D${LASER}

default: all

all: satin

clean:
	rm -f satin m*.out p*.out

satin:
	@echo 'Building target: $@'
	gcc ${CFLAGS} src/satin.c -lm -lpthread -o satin

bench: satin
	@echo Executing ...
	./satin
