OBJECTS = $(SOURCES:.c=.o)

.PHONY: all $(OBJECTS)
all: UDP.o TCP.o server.o lib.o client.o
	gcc -shared UDP.o lib.c -o lib_UDP.so
	gcc -shared TCP.o lib.c -o lib_TCP.so
	gcc server.o lib.c -o server -ldl 
	gcc client.o lib.c -o client -ldl

UDP.o: UDP.c
	gcc -c UDP.c -o UDP.o -g -O0 -fPIC

TCP.o: TCP.c
	gcc -c TCP.c -o TCP.o -g -O0 -fPIC

server.o: server.c
	gcc server.c -c -o server.o -g -O0 -ldl

client.o: client.c
	gcc client.c -c -o client.o -g -O0 -ldl

.PHONY: clean
clean:
	-rm *.o
	-rm *.so
	-rm server
	-rm client
