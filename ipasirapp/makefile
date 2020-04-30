#--------------------------------------------------------------------------#
# There is usually no need to change something here unless you want to force
# a specific compiler or specific compile flags.
#--------------------------------------------------------------------------#

CC	    =	g++ -std=c++11
CFLAGS	=	-Wall -DNDEBUG -O3

DEPS	=	../build/libcandylib.a
LIBS	=	-L../build/ -lcandylib -lz

all: enumerate

clean:
	rm -f enumerate *.o

#--------------------------------------------------------------------------#
# Link
#--------------------------------------------------------------------------#

enumerate: enumerate.o $(DEPS)
	$(CC) -o $@ enumerate.o $(LIBS)

#--------------------------------------------------------------------------#
# Compile
#--------------------------------------------------------------------------#

enumerate.o: enumerate.cpp ipasir.h makefile
	$(CC) $(CFLAGS) -c enumerate.cpp
