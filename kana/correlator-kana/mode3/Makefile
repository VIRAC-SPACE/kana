CFLAGS += -O2 -Wall -Werror -std=gnu11
CC=mpicc
LIBS=-lfftw3f -lm -lfftw3

all: main.c mode3.c ../JSON_parser.c ../jsonReader.c
	@echo [Compiling] Mode3
	@$(CC) -o m3 $^ $(CFLAGS) $(LDFLAGS) $(LIBS)
	@cp m3 ../../m3

.PHONY: clean
clean: 
	rm -rf *.o m3 m3.exe *~ *.core core
