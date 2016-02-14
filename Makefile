include config.mk

all: build/eubstat build/eubar build/eubotar

install: all
	for f in build/eubstat build/eubar build/eubotar script/*; do install $$f $(PREFIX)/bin/; done
	rsync -av lib/Archive $(PERL5LIB)
	rsync -av script/ $(PREFIX)/bin/

build/eubstat: src/eubstat.c src/eub.c src/eub.h
	mkdir -p build
	gcc -g -o $@ src/eubstat.c src/eub.c -lb2

build/eubar: src/eubar.c src/eub.c src/eub.h
	mkdir -p build
	gcc -g -o $@ src/eubar.c src/eub.c -lb2

build/eubotar: src/eubotar.c src/eub.c src/eub.h
	mkdir -p build
	gcc -g -o $@ src/eubotar.c src/eub.c -lb2 -ltar

dist: $(NAME)-$(VERSION).tar.gz

$(NAME)-$(VERSION).tar.gz: $(NAME)-$(VERSION)
	tar -czf $@ $<
	rm -Rf $<

manifest: MANIFEST

MANIFEST: MANIFEST.SKIP
	find * | egrep -v -f $< > $@

$(NAME)-$(VERSION): MANIFEST
	[ -d $@ ] || mkdir $@
	cpio -p -m $@ < $<

clean:
	rm -Rf $(NAME)-$(VERSION)* MANIFEST

.PHONY: all install dist clean manifest
