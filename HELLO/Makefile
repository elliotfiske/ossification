hello: hello.o
	gcc -g -o hello hello.o

hello.o:
	gcc -Wall -ansi -pedantic -g   -c -o hello.o hello.c

test: hello
	@echo "Testing hello..."
	@./hello
	@echo "done."




clean:
	rm -f hello.o *~ TAGS
