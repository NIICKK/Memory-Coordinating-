CC=gcc
CFLAGS=-Wall -I. -pthread -lvirt

all: Memory_coordinator

Memory_coordinator: Memory_coordinator.c
	$(CC) -o Memory_coordinator Memory_coordinator.c $(CFLAGS)
