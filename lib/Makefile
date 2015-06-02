CC=gcc
LIBDIR=$(shell pwd)
CFLAGS=--std=c11 -Wall -fPIC -ggdb
LDFLAGS=
UNAME = $(shell uname -s)

LIBRARIES=helpers bufio

ifeq ($(UNAME), Darwin)
	LDFLAGS = -install_name $(LIBDIR)/lib_LIBNAME.so
endif

.PHONY: all
all: $(foreach lib, $(LIBRARIES), lib$(lib).so )

define LIB_template

lib$(1).so: $(1).o
	$(CC) -o $$@ $$(subst _LIBNAME,$(1),$(LDFLAGS)) -shared $$<

$(1).o: $(1).c
	$(CC) -o $$@ -c $(CFLAGS) $(1).c

endef

$(foreach lib, $(LIBRARIES), $(eval $(call LIB_template,$(lib))))

.PHONY: clean
clean:
	$(foreach lib, $(LIBRARIES), rm -f lib$(lib).so $(lib).o;)
