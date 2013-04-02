SHELL:=/bin/bash

ACCESS_ENABLE:=1
ACCESS_CHKSUM:=1

prog:=$(notdir $(PWD))
objs:=$(sort $(addsuffix .o,$(basename $(wildcard *.c) $(wildcard *.go))))

%.o: %.c
	gcc -D_GNU_SOURCE -g -Wall -Wno-unused -c $<

$(prog): $(objs)
	gcc -g -Wall -o $@ $(objs) -lgdbm -lpthread

.PHONY: clean
clean:
	rm -f *.o *.dep *.cmd $(prog) options.[ch]

include GNUmakefile.genoptions
sinclude *.dep
