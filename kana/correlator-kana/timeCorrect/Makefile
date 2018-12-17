CFLAGS += -O2 -Wall -Werror -std=gnu11
CC=mpicc
LIBS=-lfftw3f -lfftw3 -lm

all: main.c lagCorrect.c ../JSON_parser.c ../jsonReader.c
	@echo [Compiling] TimeCorrect
	@$(CC) -o tc $^ $(CFLAGS) $(LDFLAGS) $(LIBS)
	@cp tc ../../tc

.PHONY: clean
clean: 
	rm -rf *.o tc tc.exe *~ *.core core
