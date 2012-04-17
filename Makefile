# Makefile for Program #2, modified from the example Makefile provided by 
# Dr. Bellardo for 453

PLAT = $(shell uname -i)
LWP-EX = liblwp-$(PLAT).a
SN-EX = libsnakes-$(PLAT).a
LWP=liblwp.a

all: $(LWP)

examples: $(LWP-EX) lwp.h smartalloc.h nums-ex snakes-ex hungry-ex nums snakes hungry

snakes: snakemain.c snakes.h $(LWP) $(SN)
	gcc -Wall -Werror -L. -g -o $@ $< $(LWP) $(SN-EX) -lncurses

snakes-ex: snakemain.c snakes.h $(LWP-EX) $(SN-EX)
	gcc -Wall -Werror -L. -g -o $@ $< $(LWP-EX) $(SN-EX) -lncurses

hungry: hungrymain.c snakes.h $(LWP) $(SN-EX)
	gcc -Wall -Werror -L. -g -o $@ $< $(LWP) $(SN-EX) -lncurses

hungry-ex: hungrymain.c snakes.h $(LWP-EX) $(SN-EX)
	gcc -Wall -Werror -L. -g -o $@ $< $(LWP-EX) $(SN-EX) -lncurses

nums: numbersmain.c lwp.h $(LWP)
	gcc -Wall -Werror -L. -ggdb -g -o $@ $< $(LWP)

nums-ex: numbersmain.c lwp.h $(LWP-EX)
	gcc -Wall -Werror -L. -g -o $@ $< $(LWP-EX)

$(LWP): lwp.o smartalloc.o
	ar r $@ lwp.o smartalloc.o
	ranlib $@

lwp.o: lwp.c lwp.h
	gcc -Wall -Werror -L. -g -c -o $@ $< 

smartalloc.o: smartalloc.c
	gcc -Wall -Werror -g -c -o $@ $< 

clean:
	-rm -f nums snakes hungry $(LWP) lwp.o liblwp.o nums-ex snakes-ex hungry-ex
	-rm -f lwp.s smartalloc.o

handin:
	handin bellardo p2 Makefile lwp.c lwp.h smartalloc.c smartalloc.h
	-handin bellardo p2 README

