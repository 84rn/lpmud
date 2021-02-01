all:	hname

hname:	hname.c
	$(CC) $(CFLAGS) hname.c $(LIBS) -o hname

install: hname
	cp hname $(MUDBIN)/hname
	sed -e "s;MUDLIB;$(MUDLIB);" \
		-e "s;MUDBIN;$(MUDBIN);" restart_mud.sh > $(MUDBIN)/restart_mud
	-chmod 775 $(MUDBIN)/restart_mud

clean:
	-rm -f *.o hname core
