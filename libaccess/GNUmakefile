# anything other than /bin/sh
SHELL:=/bin/bash

ACCESS_ENABLE:=1
ACCESS_CHKSUM:=1

objs:=$(patsubst %.c,%.o,$(wildcard *.c))

%.o:  %.c ; gcc -fPIC $(ARCH) -g -O2 -Wall -c $<
%.so: %.o ; gcc $(ARCH) -g -O2 -shared -ldl -o $@ $(objs)
%.a: %.o ; gcc $(ARCH) -g -O2 -static -ldl -o $@ $(objs)

$(notdir $(PWD)).so: $(objs)
$(notdir $(PWD)).a: $(objs)

.PHONY: clean
clean: ; rm -f *.o *.so *.cmd *.dep

sinclude *.dep
