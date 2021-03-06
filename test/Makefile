# use normal make for this Makefile
#
# Makefile for building user programs to run on top of Nachos
#
# Several things to be aware of:
#
#    Nachos assumes that the location of the program startup routine (the
# 	location the kernel jumps to when the program initially starts up)
#       is at location 0.  This means: start.o must be the first .o passed 
# 	to ld, in order for the routine "Start" to be loaded at location 0
#

# if you are cross-compiling, you need to point to the right executables
# and change the flags to ld and the build procedure for as

GCCDIR = ../../../gnu-decstation-ultrix/decstation-ultrix/2.95.3/

LDFLAGS = -T script -N
ASFLAGS = -mips2
CPPFLAGS = $(INCDIR)

# if you aren't cross-compiling:
# GCCDIR =
# LDFLAGS = -N -T 0
# ASFLAGS =
# CPPFLAGS = -P $(INCDIR)

PATH = $(GCCDIR):/lib:/usr/bin:/bin

CC = $(GCCDIR)gcc -B../../../gnu-decstation-ultrix/
AS = $(GCCDIR)as
LD = $(GCCDIR)ld

CPP = gcc -E
INCDIR =-I../userprog -I../threads -I /usr/include
CFLAGS = -G 0 -c $(INCDIR)

all: halt shell matmult sort tlb_test multi_proc1 multi_proc2 vm_test \
	fileSysCallTest progSysCallTest execSysCallTest 

start.o: start.s ../userprog/syscall.h
	$(CPP) $(CPPFLAGS) start.c > strt.s
	$(AS) $(ASFLAGS) -o start.o strt.s
	rm strt.s

halt.o: halt.c
	$(CC) $(CFLAGS) -c halt.c
halt: halt.o start.o
	$(LD) $(LDFLAGS) start.o halt.o -o halt.coff
	../bin/coff2noff halt.coff halt

shell.o: shell.c
	$(CC) $(CFLAGS) -c shell.c
shell: shell.o start.o
	$(LD) $(LDFLAGS) start.o shell.o -o shell.coff
	../bin/coff2noff shell.coff shell

sort.o: sort.c
	$(CC) $(CFLAGS) -c sort.c
sort: sort.o start.o
	$(LD) $(LDFLAGS) start.o sort.o -o sort.coff
	../bin/coff2noff sort.coff sort

matmult.o: matmult.c
	$(CC) $(CFLAGS) -c matmult.c
matmult: matmult.o start.o
	$(LD) $(LDFLAGS) start.o matmult.o -o matmult.coff
	../bin/coff2noff matmult.coff matmult

tlb_test.o: tlb_test.c
	$(CC) $(CFLAGS) -c tlb_test.c
tlb_test: tlb_test.o start.o
	$(LD) $(LDFLAGS) start.o tlb_test.o -o tlb_test.coff
	../bin/coff2noff tlb_test.coff tlb_test
	
multi_proc1.o: multi_proc1.c
	$(CC) $(CFLAGS) -c multi_proc1.c
multi_proc1: multi_proc1.o start.o
	$(LD) $(LDFLAGS) start.o multi_proc1.o -o multi_proc1.coff
	../bin/coff2noff multi_proc1.coff multi_proc1
	
multi_proc2.o: multi_proc2.c
	$(CC) $(CFLAGS) -c multi_proc2.c
multi_proc2: multi_proc2.o start.o
	$(LD) $(LDFLAGS) start.o multi_proc2.o -o multi_proc2.coff
	../bin/coff2noff multi_proc2.coff multi_proc2

vm_test.o: vm_test.c
	$(CC) $(CFLAGS) -c vm_test.c
vm_test: vm_test.o start.o
	$(LD) $(LDFLAGS) start.o vm_test.o -o vm_test.coff
	../bin/coff2noff vm_test.coff vm_test
	
fileSysCallTest.o: fileSysCallTest.c
	$(CC) $(CFLAGS) -c fileSysCallTest.c
fileSysCallTest: fileSysCallTest.o start.o
	$(LD) $(LDFLAGS) start.o fileSysCallTest.o -o fileSysCallTest.coff
	../bin/coff2noff fileSysCallTest.coff fileSysCallTest

progSysCallTest.o: progSysCallTest.c
	$(CC) $(CFLAGS) -c progSysCallTest.c
progSysCallTest: progSysCallTest.o start.o
	$(LD) $(LDFLAGS) start.o progSysCallTest.o -o progSysCallTest.coff
	../bin/coff2noff progSysCallTest.coff progSysCallTest

execSysCallTest.o: execSysCallTest.c
	$(CC) $(CFLAGS) -c execSysCallTest.c
execSysCallTest: execSysCallTest.o start.o
	$(LD) $(LDFLAGS) start.o execSysCallTest.o -o execSysCallTest.coff
	../bin/coff2noff execSysCallTest.coff execSysCallTest