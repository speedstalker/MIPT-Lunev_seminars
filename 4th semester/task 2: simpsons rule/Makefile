CC              =  gcc
CFLAGS          = -Wall -g -O0 -pthread
LIBS            = -lm

SOURCES         = $(EXECUTABLE).c
OBJECTS         = $(SOURCES:.c=.o)
EXECUTABLE      = simp_int


all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

-include $(wildcard *.d)

valgrind:
	valgrind ./$(EXECUTABLE)

make_clean: all clean

clean:
	rm -f *.o *.d

clean_all:
	rm -f *.o *.d $(EXECUTABLE)



.PHONY: all clean

