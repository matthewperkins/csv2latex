PROGNAME=csv2latex
PREFIX=/usr
BINDIR=$(PREFIX)/bin
MANDIR=$(PREFIX)/share/man/man1
CFLAGS=-Wall -Wextra -pedantic -Os
GZIP=gzip --to-stdout
all:
	${CC} $(CFLAGS) $(PROGNAME).c -o $(PROGNAME)
	$(GZIP) $(PROGNAME).1 > $(PROGNAME).1.gz
install:
	strip $(PROGNAME).exe
	install $(PROGNAME) ${DESTDIR}$(BINDIR)/$(PROGNAME)
	install $(PROGNAME).1.gz ${DESTDIR}$(MANDIR)/$(PROGNAME).1.gz
uninstall:
	$(RM) ${DESTDIR}$(BINDIR)/$(PROGNAME)
	$(RM) ${DESTDIR}$(MANDIR)/$(PROGNAME).1.gz
clean:
	$(RM) $(PROGNAME)
	$(RM) $(PROGNAME).1.gz
# vim:set ts=8 sw=8 noexpandtab et:
