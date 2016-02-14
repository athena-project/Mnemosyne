CC=g++
CFLAGS= -W -Wall -ansi -pedantic -std=c++11 -o3 -lssl -lcrypto  -lboost_serialization -lupscaledb
LDFLAGS=
EXEC=
SRC= $(wildcard *.c)
OBJ= $(SRC:.c=.o)

all: $(EXEC)

#hello: $(OBJ)
	#@$(CC) -o $@ $^ $(LDFLAGS)

main.o: hello.h

%.o: %.c
	@$(CC) -o $@ -c $< $(CFLAGS)

.PHONY: clean mrproper

clean:
	@rm -rf *.o

mrproper: clean
	@rm -rf $(EXEC)
