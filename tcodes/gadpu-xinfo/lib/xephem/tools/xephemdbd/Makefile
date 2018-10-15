# Makefile for xephemdbd.
# run this after successfully making XEphem and its libs.
# Only tested on Linux.

CC = gcc

XED = ../..
LAS = $(XED)/../../libastro
IPH = $(XED)/../../libip

CLDFLAGS =-g
CFLAGS = $(CLDFLAGS) -Wall -ffast-math -I$(XED) -I$(LAS) -I$(IPH)
LDFLAGS = $(CLDFLAGS) -L$(LAS)
LIBS = -lastro -lm

OBJS =	xephemdbd.o 		\
	db.o			\
	fsfetch.o		\
	request.o		\
	stubs.o			\
	$(XED)/gsc.o		\
	$(XED)/xe2.o

xephemdbd: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

xephemdbd_static: $(OBJS)
	$(CC) -static $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

clobber:
	touch x.o
	rm -f *.o xephemdbd


# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/12/05 06:27:38 $ $Revision: 1.5 $ $Name:  $
