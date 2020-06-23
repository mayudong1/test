flvparser : main.o io.o
	gcc -o flvparser main.o io.o

main.o : main.c amf.h flv.h
	gcc -c main.c

io.o : io.c io.h
	gcc -c io.c