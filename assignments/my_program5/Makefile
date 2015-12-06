all: minget minls

min_common.o:
	gcc -Wall -ansi -pedantic -g   -c -o min_common.o min_common.c

minls.o:
	gcc -Wall -ansi -pedantic -g   -c -o minls.o minls.c

minget.o:
	gcc -Wall -ansi -pedantic -g   -c -o minget.o minget.c

minget: minget.o min_common.o
	gcc -g -o minget minget.o min_common.o

minls: minls.o min_common.o
	gcc -g -o minls minls.o min_common.o

clean:
	rm -f *.o minget minls
