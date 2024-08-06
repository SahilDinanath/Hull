BIN=witsshell
CC=gcc

CODEDIRS=.
CFILES= $(foreach D, $(CODEDIRS), $(wildcard $(D)/*.c))

all:
	$(CC) $(CFILES) -o $(BIN)

clean:
	rm -rf $(BIN)

run:
	./witsshell

test-all:
	./test-witsshell.sh

test:
	./tester/run-tests.sh
