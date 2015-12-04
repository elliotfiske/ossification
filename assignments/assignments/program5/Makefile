CC	= gcc
CFLAGS = -Wall -g -fpic

intel-all: minls minget

all: minls minget
	
minls: minls.c
	$(CC) $(CFLAGS) minls.c -o minls

minget: minls.c
	$(CC) $(CFLAGS) minls.c -o minget

 clean:
	rm -f minget
	rm -f minls
