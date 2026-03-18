#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "resource.h"
#include "serial.h"

#define SPEED_MIN   5
#define SPEED_MAX   25
#define SPEED_DEF   15
#define DEBOUNCE_MS 300
#define TIMER_SPEED 1

static HANDLE g_serial  = INVALID_HANDLE_VALUE;
static int    g_wpm     = SPEED_DEF;

// --- helpers ---

static void set_status(HWND hwnd, BOOL connected) {
    HWND hst = GetDlgItem(hwnd, IDC_STATUS);
    SetWindowTextA(hst, "●");
    // Green = 0x00AA00, Red = 0x0000CC (BGR in COLORREF)
    // We use a static control with owner-draw colour via WM_CTLCOLORSTATIC
    SetWindowLongPtrA(hst, GWLP_USERDATA, connected ? 1 : 0);
    InvalidateRect(hst, NULL, TRUE);
}

static void update_speed_label(HWND hwnd, int wpm) {
    char buf[16];
    _snprintf(buf, sizeof(buf), "%d WPM", wpm);
    SetDlgItemTextA(hwnd, IDC_SPEED_LABEL, buf);
}

static void send_speed(int wpm) {
    if (g_serial == INVALID_HANDLE_VALUE) return;
    char buf[16];
    _snprintf(buf, sizeof(buf), "S:%d\n", wpm);
    serial_write(g_serial, buf);
}

static void send_message(HWND hwnd) {
    if (g_serial == INVALID_HANDLE_VALUE) return;
    char text[256] = {0};
    GetDlgItemTextA(hwnd, IDC_INPUT, text, sizeof(text) - 1);
    if (text[0] == '\0') return;

    // Uppercase before send — Arduino only knows uppercase
    for (int i = 0; text[i]; i++) text[i] = (char)toupper((unsigned char)text[i]);

    char msg[280];
    _snprintf(msg, sizeof(msg), "M:%s\n", text);
    serial_write(g_serial, msg);
    SetDlgItemTextA(hwnd, IDC_INPUT, "");
}

static void do_connect(HWND hwnd) {
    if (g_serial != INVALID_HANDLE_VALUE) {
        serial_close(g_serial);
        g_serial = INVALID_HANDLE_VALUE;
        SetDlgItemTextA(hwnd, IDC_CONNECT, "Connect");
        set_status(hwnd, FALSE);
        return;
    }
    char port[16] = {0};
    HWND hcb = GetDlgItem(hwnd, IDC_PORT);
    int sel = (int)SendMessageA(hcb, CB_GETCURSEL, 0, 0);
    if (sel == CB_ERR) return;
    SendMessageA(hcb, CB_GETLBTEXT, (WPARAM)sel, (LPARAM)port);
    if (port[0] == '\0') return;

    g_serial = serial_open(port);
    if (g_serial != INVALID_HANDLE_VALUE) {
        SetDlgItemTextA(hwnd, IDC_CONNECT, "Disconnect");
        set_status(hwnd, TRUE);
        send_speed(g_wpm);
    } else {
        MessageBoxA(hwnd, "Failed to open port.", "Error", MB_ICONERROR | MB_OK);
        set_status(hwnd, FALSE);
    }
}

// --- dialog proc ---

INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_INITDIALOG: {
        // Populate COM ports
        HWND hcb = GetDlgItem(hwnd, IDC_PORT);
        char ports[32][16];
        int count = 0;
        serial_list_ports(ports, &count);
        for (int i = 0; i < count; i++) {
            SendMessageA(hcb, CB_ADDSTRING, 0, (LPARAM)ports[i]);
        }
        if (count > 0) SendMessageA(hcb, CB_SETCURSEL, 0, 0);

        // Init TrackBar
        HWND htb = GetDlgItem(hwnd, IDC_SPEED);
        SendMessageA(htb, TBM_SETRANGE,  TRUE, MAKELONG(SPEED_MIN, SPEED_MAX));
        SendMessageA(htb, TBM_SETPOS,    TRUE, SPEED_DEF);
        SendMessageA(htb, TBM_SETTICFREQ, 5, 0);
        update_speed_label(hwnd, SPEED_DEF);

        set_status(hwnd, FALSE);
        SetFocus(GetDlgItem(hwnd, IDC_INPUT));
        return FALSE;
    }

    case WM_COMMAND: {
        int id = LOWORD(wp);
        int notif = HIWORD(wp);
        if (id == IDC_CONNECT && notif == BN_CLICKED) {
            do_connect(hwnd);
            return TRUE;
        }
        if (id == IDC_INPUT && notif == EN_CHANGE) {
            // handled by WM_GETDLGCODE / subclass not needed — Enter caught via IDOK
        }
        if (id == IDOK) {
            send_message(hwnd);
            return TRUE;
        }
        if (id == IDCANCEL) {
            SendMessageA(hwnd, WM_CLOSE, 0, 0);
            return TRUE;
        }
        return FALSE;
    }

    case WM_HSCROLL: {
        HWND htb = GetDlgItem(hwnd, IDC_SPEED);
        if ((HWND)lp == htb) {
            g_wpm = (int)SendMessageA(htb, TBM_GETPOS, 0, 0);
            update_speed_label(hwnd, g_wpm);
            // Debounce: reset timer on every scroll event
            KillTimer(hwnd, TIMER_SPEED);
            SetTimer(hwnd, TIMER_SPEED, DEBOUNCE_MS, NULL);
        }
        return TRUE;
    }

    case WM_TIMER: {
        if (wp == TIMER_SPEED) {
            KillTimer(hwnd, TIMER_SPEED);
            send_speed(g_wpm);
        }
        return TRUE;
    }

    case WM_CTLCOLORSTATIC: {
        HWND hctl = (HWND)lp;
        HWND hst  = GetDlgItem(hwnd, IDC_STATUS);
        if (hctl == hst) {
            HDC hdc = (HDC)wp;
            LONG_PTR connected = GetWindowLongPtrA(hst, GWLP_USERDATA);
            SetTextColor(hdc, connected ? RGB(0, 170, 0) : RGB(204, 0, 0));
            SetBkMode(hdc, TRANSPARENT);
            return (INT_PTR)GetStockObject(NULL_BRUSH);
        }
        return FALSE;
    }

    case WM_CLOSE:
        if (g_serial != INVALID_HANDLE_VALUE) serial_close(g_serial);
        EndDialog(hwnd, 0);
        return TRUE;

    default:
        return FALSE;
    }
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    (void)hPrev; (void)lpCmd; (void)nShow;
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_BAR_CLASSES };
    InitCommonControlsEx(&icc);
    DialogBoxA(hInst, MAKEINTRESOURCEA(IDD_MAIN), NULL, DialogProc);
    return 0;
}
