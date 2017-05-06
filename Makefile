CC=gcc
CPPC=g++
LD=g++
CFLAGS=-O2 -Wall -Wextra -fPIC -pipe
CPPFLAGS=$(CFLAGS)
LDFLAGS=-shared -lecap -lsqlite3


all: ecap_adapter_filter.so

ecap_adapter_filter.so: adapter_filter.o Debug.o filter.o uri_parser.o map.o
	$(LD) -o $@ $^ $(LDFLAGS)

adapter_filter.o: adapter_filter.cpp Debug.h filter.h Makefile
	$(CPPC) -o $@ $< -c $(CPPFLAGS)

Debug.o: Debug.cpp Debug.h Makefile
	$(CPPC) -o $@ $< -c $(CPPFLAGS)

filter.o: filter.c filter.h uri_parser.h map.h Makefile
	$(CC) -o $@ $< -c $(CFLAGS)

uri_parser.o: uri_parser.c uri_parser.h Makefile
	$(CC) -o $@ $< -c $(CFLAGS)

map.o: map.cpp map.h Makefile
	$(CPPC) -o $@ $< -c $(CPPFLAGS)



make_test_db: make_test_db.o
	gcc -o $@ $^ -lsqlite3

make_test_db.o: make_test_db.c Makefile
	gcc -o $@ $< -c -O2 -Wall -Wextra -pipe



clean:
	rm -fr *.o
