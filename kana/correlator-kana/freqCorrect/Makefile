CFLAGS += -O2 -Wall -Werror -std=gnu11
CC=mpicc
LIBS=-lfftw3f -lfftw3 -lm

all: main.c hzCorrect.c ../JSON_parser.c ../jsonReader.c
	@echo [Compiling] FreqCorrect
	@$(CC) -o fc $^ $(CFLAGS) $(LDFLAGS) $(LIBS)
	@cp fc ../../fc

.PHONY: clean
clean: 
	rm -rf *.o fc fc.exe *~ *.core core
