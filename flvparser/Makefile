CFLAG=-std=c99

flvparser : main.o io.o flv.o ts.o
	gcc -o flvparser main.o io.o flv.o ts.o

main.o : main.c
	gcc -c main.c ${CFLAG}

io.o : io.c io.h
	gcc -c io.c ${CFLAG}

flv.o : flv.c flv.h sps.h
	gcc -c flv.c ${CFLAG}

ts.o : ts.c ts.h
	gcc -c ts.c $(CFLAG)