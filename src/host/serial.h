#ifndef SERIAL_H
#define SERIAL_H

#include <windows.h>
#include <stdbool.h>

// Opens a COM port at 9600 8N1. Returns INVALID_HANDLE_VALUE on failure.
HANDLE serial_open(const char *port);

// Writes a null-terminated string to the open handle. Returns true on success.
bool serial_write(HANDLE h, const char *data);

// Closes the serial port handle.
void serial_close(HANDLE h);

// Enumerates available COM ports from the Windows registry.
// ports[][16] is caller-allocated; *count is set to the number found.
// Max 32 ports returned.
void serial_list_ports(char ports[][16], int *count);

#endif // SERIAL_H
