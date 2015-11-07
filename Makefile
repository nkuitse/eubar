all: build/eubstat build/eubar

build/eubstat: src/eubstat.c src/eub.c src/eub.h
	gcc -g -o $@ src/eubstat.c src/eub.c -lb2

build/eubar: src/eubar.c src/eub.c src/eub.h
	gcc -g -o $@ src/eubar.c src/eub.c -lb2

