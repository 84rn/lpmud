all:	hname

hname:	hname.c
	$(CC) $(CFLAGS) hname.c $(LIBS) -o hname

install: hname
	cp hname $(BINDIR)/hname
	sed -e "s;MUD_LIB;$(MUD_LIB);" \
		-e "s;BINDIR;$(BINDIR);" restart_mud.sh > $(BINDIR)/restart_mud
	-chmod 775 $(BINDIR)/restart_mud

clean:
	-rm -f *.o hname core
