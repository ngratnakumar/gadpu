#
# Make file for making LUTE programs
#
STATIC_DIR=./lib64/
CC = gcc -Wall -g -I. -D_FILE_OFFSET_BITS=64 -DHAVE_READLINE=1 -DARCH_64BIT -UDEBUG_MODE -DARCH_64BIT_64BIT
FC = gfortran -g -I. -D_FILE_OFFSET_BITS=64 -DHAVE_READLINE=1
LINK = gcc -g
LIBS = -lm -L./lib64/ -L. -lopt
SLALIB=-lsla -L./lib64/

ALL=aveself ltahdr ltaclean ltasel  xinfo relta diffstop ltacomb ltaedit ltabcal ltatime ltaint ltamerge
STATIC_DIR=./lib64/
all: $(ALL)

static: CC=gcc -Wall -g -I. -I$(STATIC_DIR)  -D_FILE_OFFSET_BITS=64 -DHAVE_READLINE=1
static: LIBS=$(STATIC_DIR)/libopt.a -lm
static: SLALIB=$(STATIC_DIR)/libsla.a
static:$(ALL)

static: CC=gcc -Wall -pg -I. -I/astro/Linux/include  -D_FILE_OFFSET_BITS=64 -DHAVE_READLINE=1
static: LIBS=$(STATIC_DIR)/libopt.a -lm
static: SLALIB=$(STATIC_DIR)/libsla.a
static:$(ALL)


AOBJ= aveself.o corrsub.o ltasub.o sort.o
aveself: $(AOBJ) 
	$(CC) $(AOBJ) -o $@ $(LIBS)
LBOBJ= ltabcal.o ltasub.o sort.o corrsub.o
ltabcal: $(LBOBJ) 
	$(CC) $(LBOBJ) -o $@ $(LIBS)
LCALOBJ= ltacal.o corrsub.o ltasub.o sort.o hdrsubs.o solsub.o select.o
ltacal: $(LCALOBJ) 
	$(CC) $(LCALOBJ) -o $@ $(LIBS)
LCKOBJ= ltacheck.o corrsub.o ltasub.o sort.o select.o solsub.o
ltacheck: $(LCKOBJ) 
	$(CC) $(LCKOBJ) -o $@ $(LIBS)
LCLOBJ= ltaclean.o corrsub.o ltasub.o hdrsubs.o sort.o
ltaclean: $(LCLOBJ) 
	$(CC) $(LCLOBJ) -o $@ $(LIBS)
LCMOBJ= ltacomb.o corrsub.o ltasub.o hdrsubs.o sort.o
ltacomb:$(LCMOBJ) 
	$(CC) $(LCMOBJ) -o $@ $(LIBS)
LEOBJ= ltaedit.o corrsub.o ltasub.o hdrsubs.o sort.o
ltaedit: $(LEOBJ) 
	$(CC) $(LEOBJ) -o $@ $(LIBS)
LHOBJ= ltahdr.o corrsub.o ltasub.o slasub.o sort.o
ltahdr: $(LHOBJ) 
	$(CC) $(LHOBJ) -o $@ $(LIBS)
LMOBJ= ltamerge.o corrsub.o ltasub.o sort.o hdrsubs.o
ltamerge: $(LMOBJ) 
	$(CC) $(LMOBJ) -o $@ $(LIBS)
LPOBJ= ltaprint.o corrsub.o ltasub.o sort.o slasub.o
ltaprint: $(LPOBJ) 
	$(CC) $(LPOBJ) -o $@ $(LIBS)
LTOBJ= ltatime.o corrsub.o ltasub.o sort.o
ltatime: $(LTOBJ) 
	$(CC) $(LTOBJ) -o $@ $(LIBS)
LSOBJ= ltasel.o corrsub.o ltasub.o hdrsubs.o sort.o
ltasel: $(LSOBJ) 
	$(CC) $(LSOBJ) -o $@ $(LIBS)
XOBJ= xinfo.o corrsub.o ltasub.o sort.o
xinfo: $(XOBJ) 
	$(FC) $(XOBJ) -o $@ $(SLALIB) $(LIBS)
RLOBJ= relta.o corrsub.o ltasub.o hdrsubs.o sort.o
relta: $(RLOBJ) 
	$(CC) $(RLOBJ) -o $@ $(LIBS)
DSOBJ= diffstop.o corrsub.o ltasub.o astro_cal.o hdrsubs.o sort.o
diffstop: $(DSOBJ) 
	$(CC) $(DSOBJ) -o $@ $(LIBS)
SDOBJ= sdspc.o corrsub.o ltasub.o sort.o 
sdspc: $(SDOBJ) 
	$(FC) $(SDOBJ) -o $@ -L/analysis/chengalu//pgplot -L/usr/X11R6/lib -lcpgplot -lpgplot -lX11 $(LIBS)
LSLOBJ= ltasol.o corrsub.o ltasub.o sort.o hdrsubs.o solsub.o select.o
ltasol: $(LSLOBJ) 
	$(CC) $(LSLOBJ) -o $@ $(LIBS)
LTIOBJ= ltaint.o corrsub.o ltasub.o sort.o
ltaint: $(LTIOBJ) 
	$(CC) $(LTIOBJ) -o $@ $(LIBS)

clean:
	/bin/rm *.o $(ALL)
#	/bin/mv $(ALL) /astro/Linux/bin/.

install:
	/bin/mv $(ALL) ./bin/

