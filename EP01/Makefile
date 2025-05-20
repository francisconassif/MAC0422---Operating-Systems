CC      := gcc
EPFLAG  := -pthread
RDFLAG  := -lreadline

all: ep1 uspsh

ep1: ep1.c ep1.h
	$(CC) $(EPFLAG) -o $@ ep1.c

uspsh: uspsh.c uspsh.h
	$(CC) -o $@ uspsh.c $(RDFLAG)

clean:
	rm -f ep1 uspsh
