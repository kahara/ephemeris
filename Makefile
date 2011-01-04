all:		ephemeris

clean:
		rm -f ephemeris.o ephemeris

install:	ephemeris
		cp ephemeris /var/www/earthorbit.info/development/ephemeris.cgi

ephemeris.o:	ephemeris.c
		gcc -g -c ephemeris.c -o ephemeris.o

ephemeris:	ephemeris.o
		gcc -g ephemeris.o -o ephemeris -lnova
