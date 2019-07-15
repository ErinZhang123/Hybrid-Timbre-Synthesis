# Makefile to keep Spectral Envelope (Transfer Function) Synthesis Programs
# up to date.
#	Authors: Andrew Horner/James W Beauchamp 1993
#       Copyright 1993, University of Illinois at Urbana-Champaign.
#       All rights reserved.
#
#    Latest edit: jwb 04/01/18

EXFILES =  tfsynth.fa0
OBJS = anread.o byteorder.o header.o sndhdr.o wavhdr.o 
INCLUDES = monan.h macro.h header.h byteorder.h sndhdr.h wavhdr.h
CCC = cc -m32 -c
CCL = cc -m32 -o

all: $(EXFILES)

clean: 
	rm -f $(EXFILES) $(OBJS)
tfsynth.fa0: tfsynth.fa0.o $(OBJS) $(INCLUDES) 
	$(CCL) tfsynth.fa0 tfsynth.fa0.o $(OBJS) -lm
tfsynth.fa0.o: tfsynth.fa0.c $(INCLUDES)
	$(CCC) tfsynth.fa0.c
anread.o: anread.c macro.h header.h byteorder.h
	$(CCC) anread.c
byteorder.o: byteorder.c byteorder.h
	$(CCC) byteorder.c
header.o: header.c header.h
	$(CCC) header.c
sndhdr.o: sndhdr.c sndhdr.h
	$(CCC) sndhdr.c
wavhdr.o: wavhdr.c wavhdr.h
	$(CCC) wavhdr.c


