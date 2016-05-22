CC = gcc

LOCK_MODE = -D__MSD_PTHREAD_LOCK_MODE__

CFLAGS  = $(LOCK_MODE) -fPIC -Wall -pipe -DDEBUG -g -D_GNU_SOURCE -D_REENTRANT
LIB 	= -ldl -lrt -rdynamic
INC     = -I../../inc 
OO	= http.o chunked.o
TARGETS = http.so

all: $(TARGETS)

$(TARGETS): $(OO)
	$(CC) $(CFLAGS) $(OO) -shared -o $@ $(LIBDIR) $(LIB)

install:
	install $(TARGETS) ../../bin/

.c.o:
	$(CC) $(CFLAGS) $< -c -o $@ $(INC)

clean:
	rm -f *.o
	rm -f $(TARGETS)
