include config.mk

all: build/eubstat build/eubar build/eubotar

install: all
	for f in build/eubstat build/eubar build/eubotar script/*; do install $$f $(PREFIX)/bin/; done
	rsync -av lib/Archive $(PERL5LIB)

build/eubstat: src/eubstat.c src/eub.c src/eub.h
	mkdir -p build
	gcc -g -o $@ src/eubstat.c src/eub.c -lb2

build/eubar: src/eubar.c src/eub.c src/eub.h
	mkdir -p build
	gcc -g -o $@ src/eubar.c src/eub.c -lb2

build/eubotar: src/eubotar.c src/eub.c src/eub.h
	mkdir -p build
	gcc -g -o $@ src/eubotar.c src/eub.c -lb2 -ltar

.PHONY: all install
