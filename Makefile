pkgname := scutest
HEADER :=$(pkgname).h

CFLAGS ?= -std=c99

all: $(HEADER) lib$(pkgname).a

lib$(pkgname).a: tester.o
	ar rcs $@ $^

install-lib: lib$(pkgname).a
	install -m 0744 -Dt "$(DESTDIR)/usr/lib/" $^

install-header: $(HEADER)
	install -m 0744 -Dt "$(DESTDIR)/usr/include/$(pkgname)/" $^

install: install-lib install-header

uninstall:
	rm -f "$(DESTDIR)/usr/lib/lib$(pkgname).a"
	rm -f "$(DESTDIR)/usr/include/$(pkgname)/$(HEADER)"

$(HEADER): tester.h tester.c
	cat $^ > $@

unit_test: $(HEADER) sample-test.c
	$(CC) $(CFLAGS) -g sample-test.c -o $@

test: unit_test
	./unit_test

clean:
	rm -f *.o *.a unit_test $(HEADER)

.PHONY: test install uninstall clean
