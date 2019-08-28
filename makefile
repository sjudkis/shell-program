C = gcc -g -Wall

USERNAME = judkiss
PROJECT = p3
ZIPFILE = judkiss_smallsh.zip

PROG = smallsh
SRC = smallsh.c utilities.c builtIn.c


smallsh:
	${C} -o ${PROG} ${SRC}
	

zip:
	zip -D ${ZIPFILE} ${SRC} makefile README

clean:
	rm ${PROG}


check:
	valgrind --leak-check=yes --track-origins=yes --show-reachable=yes -v ./$(PROG)
