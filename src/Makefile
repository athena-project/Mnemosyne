CXX=g++
#CXXFLAGS= -W -Wall -ansi -pedantic 
CXXFLAGS= -std=c++11 -g

LDFLAGS= -lssl -lcrypto  -lboost_serialization  -lboost_filesystem -lboost_system
LDFLAGS+= -pthread

EXEC= test55
SRC= $(wildcard */*.cpp)
SRC+= $(wildcard *.cpp)
OBJ= $(SRC:.cpp=.o)

all: $(EXEC)

test55: $(OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

main.o: hello.h

%.o: %.cpp 
	$(CXX) -c -o $@  $< $(CXXFLAGS)

.PHONY: clean mrproper

clean:
	rm -rf */*.o
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)
