CC=gcc
CPPC=g++
LD=g++
CFLAGS=-O2 -Wall -Wextra -fPIC -pipe
CPPFLAGS=$(CFLAGS)
LDFLAGS=-shared -lecap -lsqlite3


all: ecap_adapter_filter.so


ecap_adapter_filter.so: adapter_filter.o Debug.o filter.o map.o
	$(LD) -o $@ $^ $(LDFLAGS)

adapter_filter.o: adapter_filter.cpp Debug.h filter.h Makefile
	$(CPPC) -o $@ $< -c $(CPPFLAGS)

Debug.o: Debug.cpp Debug.h Makefile
	$(CPPC) -o $@ $< -c $(CPPFLAGS)

filter.o: filter.c filter.h map.h Makefile
	$(CC) -o $@ $< -c $(CFLAGS)

map.o: map.cpp map.h Makefile
	$(CPPC) -o $@ $< -c $(CPPFLAGS)


make_test_db.o: make_test_db.c
	$(CC) -o $@ $< -c -O2 -Wall -Wextra -pipe

make_test_db: make_test_db.o
	$(LD) -o $@ $^ -lsqlite3


clean:
	rm -fr *.o
