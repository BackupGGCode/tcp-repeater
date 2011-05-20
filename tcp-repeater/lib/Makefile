#Makefile

CC=gcc
CFLAGS=-Wall -g

target = error.o \
         readn.o  writen.o readline.o \
         wrapsock.o wrapstdio.o wrappthread.o wraplib.o

all:$(target)
$(target):%.o:%.c
			$(CC) -c $(CFLAGS) $< -o $@
clean:
	rm -rf *.o
 
#end
