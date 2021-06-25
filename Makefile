CURR_DIR = $(shell pwd)
CC = gcc
CFLAGS = -O3 -g -w
DNS_SOURCE = $(CURR_DIR)/DNS/dnsClient.c
DNS_TARGET = $(CURR_DIR)/DNS/DNS


all:
	$(CC) $(DNS_SOURCE) $(CFLAGS) -o $(DNS_TARGET)

clean:
	rm $(DNS_TARGET)
