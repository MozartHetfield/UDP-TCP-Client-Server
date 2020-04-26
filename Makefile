BINARY=server subscriber
SOURCES=sun_lib.cpp
SERVER_SOURCE=server.cpp
SUBSCRIBER_SOURCE=subscriber.cpp
OBJECTS=$(SOURCES:.cpp=.o)
SERVER_OBJECT=$(SERVER_SOURCE:.cpp=.o)
SUBSCRIBER_OBJECT=$(SUBSCRIBER_SOURCE:.cpp=.o)
LIBPATHS=.
LDFLAGS=
CFLAGS=-c -g -static -Wall
CC=g++

all: $(SOURCES) $(BINARY)

server: $(OBJECTS) $(SERVER_OBJECT)
	$(CC) $(LIBFLAGS) $(OBJECTS) $(SERVER_OBJECT) $(LDFLAGS) -o $@

subscriber: $(OBJECTS) $(SUBSCRIBER_OBJECT)
	$(CC) $(LIBFLAGS) $(OBJECTS) $(SUBSCRIBER_OBJECT) $(LDFLAGS) -o $@

distclean: clean
	rm -f $(BINARY)

clean:
	rm -f *.o $(BINARY)