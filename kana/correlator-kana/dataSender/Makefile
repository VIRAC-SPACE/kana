CFLAGS += -O2 -Wall -Werror -std=gnu11 -g
CC=mpicc
LIBS=-lm -lvdifio

all: main.c ../JSON_parser.c ../jsonReader.c ../socket.c
	@echo [Compiling] DataSender
	@$(CC) -o ds $^ $(CFLAGS) $(LDFLAGS) $(LIBS)
	@cp ds ../../ds

.PHONY: clean
clean:
	rm -rf *.o ds ds.exe *~ *.core core