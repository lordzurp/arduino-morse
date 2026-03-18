#include "serial.h"
#include <stdio.h>
#include <string.h>

HANDLE serial_open(const char *port) {
    // COM ports > COM9 require the \\.\COMxx form
    char path[24];
    if (strlen(port) > 4) {
        snprintf(path, sizeof(path), "\\\\.\\%s", port);
    } else {
        snprintf(path, sizeof(path), "%s", port);
    }

    HANDLE h = CreateFileA(
        path,
        GENERIC_READ | GENERIC_WRITE,
        0, NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (h == INVALID_HANDLE_VALUE) return INVALID_HANDLE_VALUE;

    DCB dcb = {0};
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(h, &dcb)) { CloseHandle(h); return INVALID_HANDLE_VALUE; }

    dcb.BaudRate = CBR_9600;
    dcb.ByteSize = 8;
    dcb.Parity   = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary  = TRUE;
    dcb.fParity  = FALSE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl  = DTR_CONTROL_ENABLE;
    dcb.fRtsControl  = RTS_CONTROL_ENABLE;

    if (!SetCommState(h, &dcb)) { CloseHandle(h); return INVALID_HANDLE_VALUE; }

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout         = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier  = 0;
    timeouts.ReadTotalTimeoutConstant    = 0;
    timeouts.WriteTotalTimeoutMultiplier = 2;
    timeouts.WriteTotalTimeoutConstant   = 500;
    SetCommTimeouts(h, &timeouts);

    return h;
}

bool serial_write(HANDLE h, const char *data) {
    if (h == INVALID_HANDLE_VALUE || !data) return false;
    DWORD len = (DWORD)strlen(data);
    DWORD written = 0;
    return WriteFile(h, data, len, &written, NULL) && written == len;
}

void serial_close(HANDLE h) {
    if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
}

void serial_list_ports(char ports[][16], int *count) {
    *count = 0;
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                      "HARDWARE\\DEVICEMAP\\SERIALCOMM",
                      0, KEY_READ, &hKey) != ERROR_SUCCESS) return;

    DWORD idx = 0;
    char  val_name[256];
    char  val_data[16];
    DWORD name_len, data_len, type;

    while (*count < 32) {
        name_len = sizeof(val_name);
        data_len = sizeof(val_data);
        LONG ret = RegEnumValueA(hKey, idx++, val_name, &name_len,
                                 NULL, &type, (LPBYTE)val_data, &data_len);
        if (ret == ERROR_NO_MORE_ITEMS) break;
        if (ret != ERROR_SUCCESS || type != REG_SZ) continue;
        snprintf(ports[*count], 16, "%s", val_data);
        (*count)++;
    }
    RegCloseKey(hKey);
}
