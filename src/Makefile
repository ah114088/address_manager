CC = gcc
LD = gcc

ALL = main

CFLAGS = -Wall -Werror -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -O1

all: $(ALL)

clean:
	rm -f *.o $(ALL)

MAIN_OBJ = main.o member.o formation.o file.o user.o
main: $(MAIN_OBJ)
	@$(LD) -o $@ $(MAIN_OBJ) -lmicrohttpd
	@echo " [LD]        $@"

%.o: %.c
	@echo " [CC]        $@"
	@$(CC) $(CFLAGS) -c $<
