CC 	= gcc

CFLAGS = -Wall -I .

CFLAGS_DEBUG  = -g $(CFLAGS)

LD 	= gcc

LDFLAGS  = -Wall -g -L.

PROGS	= snakes nums hungry

SNAKEOBJS  = snakemain.o 

HUNGRYOBJS = hungrymain.o 

NUMOBJS    = numbersmain.o

OBJS	= $(SNAKEOBJS) $(HUNGRYOBJS) $(NUMOBJS) lwp_main.o 

EXTRACLEAN = core $(PROGS)

# Bread 'n Butta ---------------------#
all: liblwp.so

liblwp.so: lwp_main.o
	$(CC) $(CFLAGS) -shared -fpic lwp_main.o -o liblwp.so

lwp_main.o:
	$(CC) $(CFLAGS) -c -g -fpic lwp_main.c magic64.S

lib_exec: lwp_main.c magic64.S
	gcc -fPIC lwp_main.c magic64.S -g -O0 -o lib_exec
# End Bread 'n Butta -----------------#


# Cleanliness.next() == Godliness
allclean: clean
	@rm -f $(EXTRACLEAN)

clean:	
	rm -f $(OBJS) *~ TAGS
	rm -f liblwp.so

# Generates test executables (snake game + number printer)
test: $(PROGS)

snakes: snakemain.o liblwp.so libsnakes.a
	$(LD) $(LDFLAGS) -o snakes snakemain.o -L. -lncurses -lsnakes -llwp

hungry: hungrymain.o liblwp.so libsnakes.a
	$(LD) $(LDFLAGS) -o hungry hungrymain.o -L. -lncurses -lsnakes -llwp

nums: numbersmain.o liblwp.so 
	$(LD) $(LDFLAGS) -o nums numbersmain.o -L. -llwp

hungrymain.o: lwp.h snakes.h

snakemain.o: lwp.h snakes.h

numbermain.o: lwp.h

# This is the end, my friend