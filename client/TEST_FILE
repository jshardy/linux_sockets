CC = gcc

CLFAGS = -g -Wall -Wshadow -Wunreachable-code -Wredundant-decls \
		-Wmissing-declarations -Wold-style-definition -Wmissing-prototypes \
        -Wdeclaration-after-statement -pthread
DEPS =
PROG = socket_server socket_client

all: $(PROG)

socket_server.o: server/socket_server.c $(DEPS)
	$(CC) $(CLFAGS) -c server/$<

socket_server: server/socket_server.o
	$(CC) $(CLFAGS) -o server/$@ $^

socket_client.o: client/socket_client.c $(DEPS)
	$(CC) $(CLFAGS) -c client/$<

socket_client: client/socket_client.o
	$(CC) $(CLFAGS) -o client/$@ $^

compress: clean
	tar -czvf lab9.tar.gz *

clean:
	rm -rf client/socket_client server/socket_server
	rm server/*.o server/*.dSYM client/*.o client/*.dSYM
