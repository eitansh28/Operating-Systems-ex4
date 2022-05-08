all: threadServer client

threadServer: lib.a threadServer.o 
	gcc -Wall -g lib.a threadServer.o -o threadServer -lpthread

client: client.o
	gcc -Wall -g client.o -o client

lib.a: implementMemory.o
	ar -rcs lib.a implementMemory.o

client.o: client.cpp 
	gcc -Wall -g -c client.cpp  

threadServer.o: threadServer.cpp implementMemory.hpp 
	g++ -Wall -g -c threadServer.cpp 

implementMemory.o: implementMemory.cpp 
	gcc -Wall -g -c implementMemory.cpp

.PHONY: all clean

clean :
	rm -f *.o *.a client threadServer