SHELL:=/bin/bash

ACCESS_ENABLE:=1
ACCESS_CHKSUM:=1

prog:=$(notdir $(PWD))
objs:=$(sort $(addsuffix .o,$(basename $(wildcard *.c) $(wildcard *.go))))

MYSQLCFLAGS:=$(shell mysql_config --cflags)
MYSQLLFLAGS:=$(shell mysql_config --libs)

GCRYPTCFLAGS:=$(shell libgcrypt-config --cflags)
GCRYPTLFLAGS:=$(shell libgcrypt-config --libs)

%.o: %.c
	gcc -g -O2 -Wall $(MYSQLCFLAGS) $(GCRYPTCFLAGS) -c $<

$(prog): $(objs)
	gcc -g -O2 -Wall \
	    $(MYSQLLFLAGS) $(GCRYPTLFLAGS) \
	    -larchive \
	    -o $@ $(objs)

.PHONY: clean
clean:
	rm -f *.o *.dep *.cmd tcd.err.* tcd.0 $(prog) options.[ch]

include GNUmakefile.genoptions
sinclude *.dep
