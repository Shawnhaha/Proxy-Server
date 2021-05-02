all: pyserver client

server: pyserver.c
	gcc pyserver.c -o pyserver

client: client.c
	gcc client.c -o client

clean:
	rm pyserver client
