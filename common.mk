CC=gcc
LIBDIR=../lib
CFLAGS=-I$(LIBDIR) -ggdb
UNAME=$(shell uname -s)
LDFLAGS=-L$(LIBDIR) $(foreach lib, $(LIBS),-l$(lib) )

ifeq ($(UNAME), Linux)
	LDFLAGS += -Wl,-rpath,$(LIBDIR)
endif

.PHONY: all
all: $(PROGRAM)

define PROG_TEMPLATE

$(1): $(1).o libs
	$(CC) -o $$@ $$< $(LDFLAGS)

endef

$(foreach prog, $(PROGRAM), $(eval $(call PROG_TEMPLATE,$(prog))))

%.o: %.c
	$(CC) -o $@ -c $(CFLAGS) $<

.PHONY: libs
libs:
	$(MAKE) -C $(LIBDIR)

.PHONY: clean
clean:
	rm -f $(PROGRAM) $(PROGRAM).o
	$(MAKE) -C $(LIBDIR) clean
