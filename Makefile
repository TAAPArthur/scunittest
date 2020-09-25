pkgname := scutest

CFLAGS ?= -Og -g

lib$(pkgname).a: tester.o
	ar rcs $@ $^

install: lib$(pkgname).a tester.h
	install -m 0744 -Dt "$(DESTDIR)/usr/lib/" $<
	install -m 0744 -Dt "$(DESTDIR)/usr/include/$(pkgname)/" tester.h

uninstall:
	rm -f "$(DESTDIR)/usr/lib/lib$(pkgname).a"
	rm -f "$(DESTDIR)/usr/include/$(pkgname)/tester.h"

unit_test: tester.c tester.h sample-test.c
	$(CC) -g *.c -o $@

test: unit_test
	./unit_test

clean:
	rm -f *.o *.a unit_test
.PHONY: test install uninstall clean

