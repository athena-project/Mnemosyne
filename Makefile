CC=g++
CFLAGS= -W -Wall -ansi -pedantic -std=c++11 -o3 -lssl -lcrypto  -lboost_serialization -lupscaledb
LDFLAGS=
EXEC= 
SRC= $(wildcard *.cpp)
OBJ= $(SRC:.c=.o)

btree: src/btree.o
	$(CC) -o $@ $^ $(LDFLAGS)
	
src/btree.o: 
	@$(CC) -o $@ -cpp $< $(CFLAGS)
#all: $(EXEC)

##hello: $(OBJ)
	##@$(CC) -o $@ $^ $(LDFLAGS)

#main.o: hello.h

%.o: %.cpp
	@$(CC) -o $@ -cpp $< $(CFLAGS)

#.PHONY: clean mrproper

clean:
	@rm -rf *.o

mrproper: clean
	@rm -rf $(EXEC)
