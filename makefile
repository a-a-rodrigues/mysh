CC = gcc
CFLAGS = -std=c99 -Wall 
OBJECTS = mysh.o arraylist.o

.PHONY: all clean

all: mysh

mysh: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o mysh

mysh.o: mysh.c arraylist.h
	$(CC) $(CFLAGS) -c mysh.c

arraylist.o: arraylist.c arraylist.h
	$(CC) $(CFLAGS) -c arraylist.c

clean:
	rm -f *.o mysh
