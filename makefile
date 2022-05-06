all: threadServer client

threadServer: threadServer.o
	gcc -Wall -g threadServer.o -o threadServer -lpthread

client: client.o
	gcc -Wall -g client.o -o client

client.o: client.cpp 
	gcc -Wall -g -c client.cpp  

threadServer.o: threadServer.cpp 
	g++ -Wall -g -c threadServer.cpp 

# implementMemory.o: implementMemory.hpp implementMemory.cpp 
# 	gcc -Wall -g -c implementMemory.cpp

.PHONY: all clean

clean :
	rm -f *.o client threadServer