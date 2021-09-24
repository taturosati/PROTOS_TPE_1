.PHONY=clean all
COMPILER=gcc
CFLAGS = -Wall -fsanitize=address
CFILES = src/*.c
OUT = server

all: server
clean:	
	rm -f *.o server


server:      
	$(COMPILER) $(CFLAGS) -I./include -o ${OUT} $(CFILES)

