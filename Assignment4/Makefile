all: server client

server: server.c
	gcc -o server.o server.c

client: client.c
	gcc -o client.o client.c -lm

testPrep: server.o
	cp server.o ../fileHub

.PHONY: testPrep