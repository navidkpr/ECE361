all: message server client

message: message.c
	gcc -c message.c

server: server.c
	gcc -c server.c
	gcc -g message.o server.o -o server 

client: client.c
	gcc -c client.c
	gcc -g message.o client.o -lm -o client

testPrep: server.o
	cp server ../fileHub

.PHONY: testPrep