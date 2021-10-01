.PHONY=clean all
COMPILER=gcc
CFLAGS = -Wall -fsanitize=address
CFILES = src/*.c src/parser/*.c src/utils/*.c
OUT = server

all: server
clean:	
	rm -f *.o server


server:      
	$(COMPILER) $(CFLAGS) -I./include -o ${OUT} $(CFILES)

