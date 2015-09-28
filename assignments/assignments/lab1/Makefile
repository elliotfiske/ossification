CC = gcc
LD = gcc
CFLAGS = -Wall -g -pedantic
LDFLAGS = -g

pipeit: main.o
	$(LD) $(LDFLAGS) -o pipeit main.o

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

clean:
	rm -f main.o