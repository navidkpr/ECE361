all: server client

server: server.c
	gcc -g -o server.o server.c

client: client.c
	gcc -g -o client.o client.c -lm

testPrep: server.o
	cp server.o ../fileHub

.PHONY: testPrep