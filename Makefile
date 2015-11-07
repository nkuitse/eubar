include config.mk

all: build/eubstat build/eubar

install: all
	for f in build/eubstat build/eubar script/*; do install $$f $(PREFIX)/bin/; done
	rsync -av lib/Archive $(PERL5LIB)

build/eubstat: src/eubstat.c src/eub.c src/eub.h
	gcc -g -o $@ src/eubstat.c src/eub.c -lb2

build/eubar: src/eubar.c src/eub.c src/eub.h
	gcc -g -o $@ src/eubar.c src/eub.c -lb2

.PHONY: all install
