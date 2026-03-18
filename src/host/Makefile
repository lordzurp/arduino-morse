CC      = x86_64-w64-mingw32-gcc
WINDRES = x86_64-w64-mingw32-windres
TARGET  = ../../arduino-morse.exe

SRCS = main.c serial.c
RES  = resource.o

CFLAGS  = -Wall -Wextra -O2 -mwindows -I.
LDFLAGS = -mwindows
LIBS    = -lcomctl32 -ladvapi32

.PHONY: all clean

all: $(TARGET)

$(RES): resource.rc resource.h
	$(WINDRES) $< -o $@

$(TARGET): $(SRCS) $(RES)
	$(CC) $(CFLAGS) $(SRCS) $(RES) -o $@ $(LDFLAGS) $(LIBS)

clean:
	rm -f $(RES) $(TARGET)
