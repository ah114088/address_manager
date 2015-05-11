CC = gcc
LD = gcc

ALL = post_example main

CFLAGS = -Wall -Werror

all: $(ALL)

clean:
	rm -f *.o $(ALL)

post_example: post_example.o
	@$(LD) -o $@ post_example.o -lmicrohttpd
	@echo " [LD]        $@"

main: main.o member.o
	@$(LD) -o $@ main.o member.o -lmicrohttpd
	@echo " [LD]        $@"

%.o: %.c
	@echo " [CC]        $@"
	@$(CC) $(CFLAGS) -c $<
