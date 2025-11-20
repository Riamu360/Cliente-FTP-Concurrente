TARGET = ZunigaL-clienteFTP
CC = gcc
CFLAGS = -w
SOURCES = ZunigaL-clienteFTP.c connectsock.c connectTCP.c errexit.c

all:
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)
