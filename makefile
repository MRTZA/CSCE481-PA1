# makefile

all: client server

server: crs.c
	g++ -g -w -Wall -O1 -std=c++11 -o server crs.c
	
client: crc.c
	g++ -g -w -Wall -O1 -std=c++11 -o client crc.c

clean:
	rm -rf *.o fifo* server client
