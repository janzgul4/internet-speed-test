CC = gcc
CFLAGS = -lcurl -lcjson

all:
	gcc main.c -o speedtest -lcurl -lcjson

clean:
	rm -f speedtest
