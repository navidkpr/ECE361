all: server deliver testPrep

server: server.c
	gcc -o server.o server.c

deliver: deliver.c
	gcc -o deliver.o deliver.c -lm

testPrep: server.o
	cp server.o ../fileHub

.PHONY: testPrep