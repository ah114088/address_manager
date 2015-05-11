ALL = post_example main
all: $(ALL)

clean:
	rm -f *.o $(ALL)

post_example: post_example.c
	gcc -o post_example post_example.c -lmicrohttpd

main: main.c
	gcc -o main main.c -lmicrohttpd
