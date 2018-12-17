CFLAGS += -O2 -Wall -Werror -std=gnu11 -g
CC=mpicc
LIBS=-lfftw3f -lfftw3 -lm -lvdifio

all: main.c mode1.c spectra.c ../JSON_parser.c ../jsonReader.c ../socket.c
	@echo [Compiling] Mode1
	@$(CC) -o m1 $^ $(CFLAGS) $(LDFLAGS) $(LIBS)
	@cp m1 ../../m1

.PHONY: clean
clean:
	rm -rf *.o m1 m1.exe *~ *.core core
