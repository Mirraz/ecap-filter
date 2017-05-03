CC=gcc
CPPC=g++
LD=g++
CFLAGS=-O2 -Wall -Wextra -fPIC -pipe
CPPFLAGS=$(CFLAGS)
LDFLAGS=-shared -lecap -lsqlite3


all: ecap_adapter_filter.so


ecap_adapter_filter.so: adapter_filter.o filter.o map.o
	$(LD) -o $@ $^ $(LDFLAGS)

adapter_filter.o: adapter_filter.cpp filter.h Makefile
	$(CPPC) -o $@ $< -c $(CPPFLAGS)

filter.o: filter.c filter.h map.h Makefile
	$(CC) -o $@ $< -c $(CFLAGS)

map.o: map.cpp map.h Makefile
	$(CPPC) -o $@ $< -c $(CPPFLAGS)


clean:
	rm -fr *.o
