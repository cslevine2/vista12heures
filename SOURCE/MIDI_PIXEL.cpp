#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <cstdio>

static const int CHANNELS = 16;
static const int NOTES = 128;
static const int LEFT = 118;
static const int TOP = 92;
static const int ROW_H = 24;
static const int CELL_W = 5;
static const int GRID_W = NOTES * CELL_W;
static DWORD g_start = 0;

static COLORREF pitchColors[12] = {
    RGB(230, 82, 82), RGB(225, 125, 65), RGB(220, 175, 65),
    RGB(175, 205, 70), RGB(95, 195, 95), RGB(65, 190, 155),
    RGB(65, 175, 205), RGB(75, 125, 220), RGB(115, 100, 220),
    RGB(165, 95, 205), RGB(205, 90, 165), RGB(220, 90, 115)
};

static void Fill(HDC dc, int x, int y, int w, int h, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    RECT r = { x, y, x + w, y + h };
    FillRect(dc, &r, brush);
    DeleteObject(brush);
}

static void Text(HDC dc, int x, int y, LPCTSTR value, COLORREF color, int height = 18) {
    SetTextColor(dc, color);
    SetBkMode(dc, TRANSPARENT);
    HFONT font = CreateFont(height, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("Segoe UI"));
    HFONT old = (HFONT)SelectObject(dc, font);
    TextOut(dc, x, y, value, lstrlen(value));
    SelectObject(dc, old);
    DeleteObject(font);
}

static void Paint(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(hwnd, &ps);
    RECT client;
    GetClientRect(hwnd, &client);
    Fill(dc, 0, 0, client.right, client.bottom, RGB(19, 22, 28));

    Text(dc, 24, 18, _T("MIDI PIXEL"), RGB(235, 240, 246), 30);
    Text(dc, 25, 53, _T("WIN32 / GDI  -  FIRST VISUAL PROTOTYPE"), RGB(135, 151, 170), 15);

    // Note ruler: all 128 MIDI note numbers, with octave C positions marked.
    for (int n = 0; n < NOTES; ++n) {
        int x = LEFT + n * CELL_W;
        if ((n % 12) == 0) {
            Fill(dc, x, TOP - 12, 1, CHANNELS * ROW_H + 13, RGB(88, 105, 125));
            TCHAR label[16];
            _stprintf_s(label, _T("C%d"), (n / 12) - 1);
            Text(dc, x - 3, TOP - 34, label, RGB(165, 180, 198), 13);
        } else if ((n % 12) == 6) {
            Fill(dc, x, TOP - 5, 1, CHANNELS * ROW_H + 6, RGB(48, 58, 70));
        }
    }

    DWORD tick = GetTickCount() - g_start;
    int phase = (int)(tick / 180);
    for (int ch = 0; ch < CHANNELS; ++ch) {
        int y = TOP + ch * ROW_H;
        TCHAR channel[8];
        _stprintf_s(channel, _T("%02d"), ch + 1);
        Text(dc, 25, y + 3, channel, RGB(145, 160, 180), 15);
        Fill(dc, LEFT, y + ROW_H - 2, GRID_W, 1, RGB(43, 51, 62));

        // Deliberately synthetic note activity for layout testing only.
        for (int k = 0; k < 5; ++k) {
            int note = (ch * 7 + k * 19 + phase * (k + 1)) % NOTES;
            int velocity = (ch * 13 + k * 29 + phase * 11) % 128;
            int barHeight = 5 + velocity / 18;
            COLORREF c = pitchColors[note % 12];
            Fill(dc, LEFT + note * CELL_W, y + ROW_H - 4 - barHeight,
                 CELL_W - 1, barHeight, c);
        }
    }

    int footerY = TOP + CHANNELS * ROW_H + 20;
    Fill(dc, 24, footerY, client.right - 48, 1, RGB(55, 65, 78));
    Text(dc, 25, footerY + 12, _T("STOP    PAUSE    BACKWARD    FORWARD"), RGB(185, 197, 211), 16);
    Text(dc, client.right - 116, footerY + 12, _T("PLAY  >"), RGB(120, 220, 155), 16);
    Text(dc, 25, footerY + 39,
         _T("DEMO NOTES ONLY - MIDI FILE LOADING AND PLAYBACK ARE NOT IMPLEMENTED"),
         RGB(235, 178, 92), 13);

    EndPaint(hwnd, &ps);
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        g_start = GetTickCount();
        SetTimer(hwnd, 1, 80, NULL);
        return 0;
    case WM_TIMER:
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    case WM_PAINT:
        Paint(hwnd);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_DESTROY:
        KillTimer(hwnd, 1);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

int APIENTRY WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int show) {
    const TCHAR CLASS_NAME[] = _T("MIDI_PIXEL_FIRST_WINDOW");
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    if (!RegisterClass(&wc)) return 1;

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, _T("MIDI PIXEL - first visual prototype"),
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 900, 650,
        NULL, NULL, instance, NULL);
    if (!hwnd) return 2;
    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
