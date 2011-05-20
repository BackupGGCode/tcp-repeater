#Makefile

CC=gcc
CFLAGS=-Wall -g

target =server client
#         readn.o  writen.o  \
#         wrapsock.o wrapstdio.o wrappthread.o wraplib.o \
#         lb_list.o topnode.o main.o 

all:$(target)

server:error.o realserv.o wrapsock.o wrapstdio.o wrappthread.o wraplib.o lb_list.o topnode.o main.o
			$(CC) -g $(CFLAGS) $^ -o $@ -lpthread -lxml2
client:client.o
			$(CC) -g $(CFLAGS) $^ -o $@ -lpthread -lxml2
#topnode:topnode.o image.o error.o wrapstdio.o 
#			$(CC) -g $(CFLAGS) $^ -o $@ -lxml2
%.o:test/%.c
			$(CC) -c $(CFLAGS) $^ -o $@
%.o:lib/%.c
			$(CC) -c $(CFLAGS) $^ -o $@
%.o:src/%.c
			$(CC) -c $(CFLAGS) $^ -o $@
clean:
	rm -rf *.o
 
#end
