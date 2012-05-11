CC= gcc
INC = -I ./ -I../inc
TELNET_CFLAGS = $(CFLAGS) -c  
LIB = -lpthread

.c.o:
	$(CC)  $(TELNET_CFLAGS) $(INC)  $(DEBUG_FLAGS)  $<

all: demo
static shared: all

demo: telnet_a
	$(CC) -g  -Wall -o $@ demo.c -L ./ -ltelnet $(LIB)

telnet_a: telnet_server.o telnet_io.o telnet_work.o telnet_term.o telnet_slc.o telnet_state.o \
          telnet_subneg.o telnet_cmd.o
	ar -r libtelnet.a telnet_server.o telnet_io.o telnet_work.o telnet_term.o telnet_slc.o \
          telnet_state.o telnet_subneg.o telnet_cmd.o
	rm -f *.o
	
clean:
	rm -f *.o *~ *.log demo libtelnet.a ./dist/*
	rm -f *.txt
