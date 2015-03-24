all: myshell

myshell: lex.yy.o myshell.o
	gcc -o myshell lex.yy.o myshell.o -lfl

lex.yy.c: lex.c
	flex lex.c

lex.yy.o: lex.yy.c
	gcc -c lex.yy.c

myshell.o: myshell.c
	gcc -c myshell.c

clean:
	rm -r myshell.o lex.yy.o lex.yy.c

spotless: clean
	rm -r myshell
