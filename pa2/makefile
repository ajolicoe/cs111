all: threads

threads.o: threads.c
	gcc -c threads.c

threads: threads.o
	gcc -o threads threads.o

clean:
	rm -r threads.o

spotless: clean
	rm threads
