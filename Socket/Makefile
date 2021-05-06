OBJECTS = $(SOURCES:.c=.o)


.PHONY: all $(OBJECTS)
all: UDP.o TCP.o server.o lib.o client.o log.o
	$(CC) -shared UDP.o lib.c log.c -o lib_UDP.so
	$(CC) -shared TCP.o lib.c log.c -o lib_TCP.so
	$(CC) server.o lib.c log.c -o server -ldl 
	$(CC) client.o lib.c log.c -o client -ldl

UDP.o: UDP.c
	$(CC) -c UDP.c -o UDP.o -g -O0 -fPIC

TCP.o: TCP.c
	$(CC) -c TCP.c -o TCP.o -g -O0 -fPIC

log.o: log.c
	$(CC) log.c -c -o log.o

server.o: server.c
	$(CC) server.c -c -o server.o -g -O0 -ldl

client.o: client.c
	$(CC) client.c -c -o client.o -g -O0 -ldl

.PHONY: clean
clean:
	-rm *.o
	-rm *.so
	-rm server
	-rm client
