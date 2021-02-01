# This line is needed on some machines. 
MAKE=make

# Set the root directory for mudlib/driver
MUDROOT = /mud

# Set MUDLIB to the directory which contains the mud data. Was formerly
# defined in config.h !
MUDLIB = $(MUDROOT)/lib

# Set MUDBIN to the directory where you want to install the executables.
MUDBIN = $(MUDROOT)/bin

# Specify the interface source files on the ISRC line, and the corresponding
# object files on the IOBJ line.
# NB. interface.c should be edited accordingly.
ISRC= clib/efun.c clib/stdobject.c clib/gl_language.c
IOBJ= clib/efun.o clib/stdobject.o clib/gl_language.o

# List of objects that might not exist in your standard library
# Select from: ualarm.[co], drand48.[co], strerror.[co], strpbrk.[co],
#              strtod.[co], snprintf.[co], vprintf.[co]
MISSING_SRC=drand48.c snprintf.c
MISSING_OBJ=drand48.o snprintf.o

# Chose one of these mallocs:
# This is the standard malloc for the CD gamedriver. It is quite fast
# and has a very small memory overhead.
MALLOC=bibopmalloc

# This is the old standard mud malloc. It's a bit faster that bibopmalloc
# but uses more memory.
#MALLOC=smalloc

# A variant of smalloc for debug purposes
#MALLOC=debugmalloc
#DEBUG_MALLOC=-DDEBUG_MALLOC

# Gnu malloc, fastest but uses most memory.
#MALLOC=gmalloc

# Use the standard malloc on your system:
#MALLOC=sysmalloc

# Another debug allocator
#MALLOC=dmalloc

# Define what random number generator to use.
# If no one is specified, a guaranteed bad one will be used.
#
RAND=DRAND48

# Select a parser generator among the following:
# Bison from the FSF. Produces good code. Is available for anonymous ftp.
YACC=bison -y

# The Berkely yacc. Produces the best code. Is available for anonymous ftp.
#YACC=byacc

# Standard yacc. Should be available on any Unix system worth its salt.
# WARNING: This will probably not work on all systems!
#YACC=yacc

# If you don't have 'strchr', then add next flag to CFLAGS.
# -Dstrchr=index -Dstrrchr=rindex
#
PROFIL=
#PROFIL=-p -DMARK
#PROFIL=-pg
#PROFIL=-pg -non_shared

#Enable warnings from the compiler, if wanted.
WARN=-Wall -Wshadow -Wstrict-prototypes

# Enable run time debugging. It will use more time and space.
# When the flag is changed, be sure to recompile everything.
# Simply comment out this line if not wanted.
# If you change anything in the source, you are strongly encouraged to have
# DEBUG defined.
# If you will not change anything, you are still strongly to have it defined,
# as long as the game is not bug free.
DEBUG=
#DEBUG=-DDEBUG

# System dependent flags
# Linux
#  Some versions might require -lcrypt added to SYS_LIBS
SYS_CFLAGS=-pipe -D__USE_BSD_SIGNAL
SYS_OPT=-g -fomit-frame-pointer
SYS_LIBS=-lcrypt
CC=gcc
ULIBS=
LINTFILTER=sedf.sun

CFLAGS=  $(SYS_CFLAGS) $(SYS_OPT) -DWORD_WRAP -D$(RAND) -DMALLOC_$(MALLOC) \
 $(WARN) $(DEBUG) $(DEBUG_MALLOC)
LINTFLAGS=	$(CFLAGS)

#
# Add extra libraries here.
#
LIBS= -lm $(SYS_LIBS)
MFLAGS = "MUDBIN=$(MUDBIN)" "MUDLIB=$(MUDLIB)"

driver: DUMMY
	$(MAKE) -f Makefile.distrib "CC=$(CC)" "CFLAGS=$(CFLAGS) $(PROFIL)" "YACC=$(YACC)" "LIBS=$(LIBS)" "ISRC=$(ISRC)" "IOBJ=$(IOBJ)" "MALLOC=$(MALLOC)" "MUDLIB=$(MUDLIB)" "MUDBIN=$(MUDBIN)" "MISSING_SRC=$(MISSING_SRC)" "MISSING_OBJ=$(MISSING_OBJ)" $@


install.utils:
	(cd util;\
	$(MAKE) $(MFLAGS) "CC=$(CC)" "CFLAGS=$(CFLAGS)" "LIBS=$(ULIBS)" install)

utils:	
	(cd util;\
	$(MAKE) $(MFLAGS) "CC=$(CC)" "CFLAGS=$(CFLAGS)" "LIBS=$(ULIBS)")

install:
	-$(MAKE) -f Makefile.distrib "MUDBIN=$(MUDBIN)" $@

tags:	TAGS

clean depend lint TAGS:
	-$(MAKE) -f Makefile.distrib "CC=$(CC)" "CFLAGS=$(CFLAGS)" "LINTFLAGS=$(LINTFLAGS)" "LINTFILTER=$(LINTFILTER)" "YACC=$(YACC)" "LIBS=$(LIBS)" "ISRC=$(ISRC)" "MALLOC=$(MALLOC)" "MUDLIB=$(MUDLIB)" "MISSING_SRC=$(MISSING_SRC)" $@

check:
	-$(MAKE) -f Makefile.distrib $@

DUMMY:
