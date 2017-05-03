CC=gcc
CPPC=g++
LD=g++
CFLAGS=-O2 -Wall -Wextra -fPIC -pipe
CPPFLAGS=$(CFLAGS)
LDFLAGS=-shared -lecap


all: ecap_adapter_filter.so


ecap_adapter_filter.so: adapter_filter.o filter.o
	$(LD) -o $@ $^ $(LDFLAGS)

adapter_filter.o: adapter_filter.cpp Makefile
	$(CPPC) -o $@ $< -c $(CPPFLAGS)

filter.o: filter.c filter.h Makefile
	$(CC) -o $@ $< -c $(CFLAGS)

clean:
	rm -fr *.o
