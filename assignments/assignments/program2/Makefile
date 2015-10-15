CC 	= gcc

CFLAGS = -Wall -I .

CFLAGS_DEBUG  = -g $(CFLAGS)

LD 	= gcc

LDFLAGS  = -Wall -g -L.

PROGS	= snakes nums hungry

SNAKEOBJS  = snakemain.o 

HUNGRYOBJS = hungrymain.o 

NUMOBJS    = numbersmain.o

OBJS	= $(SNAKEOBJS) $(HUNGRYOBJS) $(NUMOBJS) 

EXTRACLEAN = core $(PROGS)

# Bread 'n Butta ---------------------#
all: liblwp.so

liblwp.so: lwp_main.o
	$(CC) $(CFLAGS) -shared -fPIC -o liblwp.so lwp_main.o

lwp_main.o:
	gcc -c -fPIC lwp_main.c magic64.S
# End Bread 'n Butta -----------------#


# Cleanliness.next() == Godliness
allclean: clean
	@rm -f $(EXTRACLEAN)

clean:	
	rm -f $(OBJS) *~ TAGS

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