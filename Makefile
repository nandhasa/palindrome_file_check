GCC=gcc
CFLAGS=-g
LIB:=$(shell pkg-config --libs --cflags libcurl)
LIB+=-lpthread
#DEBUG=-DDEBUG
SOURCES=file_request.c
SOURCES+=file_parse.c
SOURCES+=file_palindrome_check.c

OBJECTS=$(SOURCES:.c=.o)

EXECUTABLE=palindrome_check

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(GCC) $(CFLAGS) $(OBJECTS) -o $@ $(LIB)

.c.o: $<
	$(GCC) $(CFLAGS) $(LIB) $(DEBUG) -c $< 

clean:
	rm -rf *.o rewind.test*
