CFLAGS += -O2 -Wall -Werror -std=gnu11
CC=mpicc
LIBS=-lfftw3 -lm

all: main.c mode2.c ../JSON_parser.c ../jsonReader.c
	@echo [Compiling] Mode2
	@$(CC) -o m2 $^ $(CFLAGS) $(LDFLAGS) $(LIBS)
	@cp m2 ../../m2

.PHONY: clean
clean: 
	rm -rf *.o m2 m2.exe *~ *.core core
