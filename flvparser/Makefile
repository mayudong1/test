flvparser : main.o io.o flv.o
	gcc -o flvparser main.o io.o flv.o

main.o : main.c
	gcc -c main.c

io.o : io.c io.h
	gcc -c io.c

flv.o : flv.c flv.h
	gcc -c flv.c