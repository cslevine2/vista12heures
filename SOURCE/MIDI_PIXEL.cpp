#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <cstdio>

static const TCHAR VERSION[] = _T("vA0.004");
static TCHAR g_iniPath[MAX_PATH] = {};
static const int WIN_W = 760;
static const int WIN_H = 500;

static const TCHAR* tracks[] = {
    _T("Piccolo"), _T("Clarinette"), _T(""), _T("Drums"),
    _T("Lead / sawtooth 1"), _T("Brass"), _T("Strings Ensemble"),
    _T("Synth"), _T("Acoustic Grand Piano 2"), _T("Acoustic Guitar 1"),
    _T("Syn Bass 1"), _T("")
};
static const int TRACK_COUNT = sizeof(tracks) / sizeof(tracks[0]);

static void Fill(HDC dc, int x, int y, int w, int h, COLORREF color) {
    HBRUSH b = CreateSolidBrush(color);
    RECT r = { x, y, x + w, y + h };
    FillRect(dc, &r, b);
    DeleteObject(b);
}
static void Txt(HDC dc, int x, int y, LPCTSTR s, COLORREF color, int size = 13, int weight = FW_NORMAL) {
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    HFONT f = CreateFont(size, 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, _T("Segoe UI"));
    HFONT old = (HFONT)SelectObject(dc, f);
    TextOut(dc, x, y, s, lstrlen(s));
    SelectObject(dc, old);
    DeleteObject(f);
}
static void DrawLine(HDC dc, int x1, int y, int x2, COLORREF color) {
    HPEN p = CreatePen(PS_SOLID, 2, color);
    HPEN old = (HPEN)SelectObject(dc, p);
    MoveToEx(dc, x1, y, NULL); LineTo(dc, x2, y);
    SelectObject(dc, old); DeleteObject(p);
}
static void SaveWindow(HWND hwnd) {
    if (IsIconic(hwnd) || IsZoomed(hwnd)) return;
    RECT r;
    if (!GetWindowRect(hwnd, &r)) return;
    TCHAR n[32];
    _stprintf_s(n, _T("%d"), r.left); WritePrivateProfileString(_T("Window"), _T("X"), n, g_iniPath);
    _stprintf_s(n, _T("%d"), r.top); WritePrivateProfileString(_T("Window"), _T("Y"), n, g_iniPath);
    _stprintf_s(n, _T("%d"), r.right-r.left); WritePrivateProfileString(_T("Window"), _T("Width"), n, g_iniPath);
    _stprintf_s(n, _T("%d"), r.bottom-r.top); WritePrivateProfileString(_T("Window"), _T("Height"), n, g_iniPath);
}
static void Paint(HWND hwnd) {
    PAINTSTRUCT ps; HDC dc = BeginPaint(hwnd, &ps);
    RECT cr; GetClientRect(hwnd, &cr);
    Fill(dc, 0, 0, cr.right, cr.bottom, RGB(49, 46, 72));

    const COLORREF label = RGB(190, 190, 205);
    const COLORREF line = RGB(105, 103, 139);
    const COLORREF faint = RGB(75, 75, 105);
    const COLORREF white = RGB(235, 235, 242);
    const int labelX = 24, timelineX = 225, timelineR = cr.right - 28;
    const int rowTop = 28, rowH = 20;

    Txt(dc, 24, 10, _T("MIDI PIXEL"), RGB(215, 218, 235), 15, FW_SEMIBOLD);
    Txt(dc, cr.right - 82, 12, VERSION, RGB(155, 160, 190), 11);

    // Compact track overview: instrument names at left, note events on clean horizontal lanes.
    for (int i = 0; i < TRACK_COUNT; ++i) {
        int y = rowTop + i * rowH;
        if (tracks[i][0]) Txt(dc, labelX, y - 5, tracks[i], label, 10);
        DrawLine(dc, timelineX, y + 2, timelineR, line);
        for (int k = 0; k < 3; ++k) {
            int tickX = timelineX + 70 + ((i * 47 + k * 83) % (timelineR - timelineX - 100));
            Fill(dc, tickX, y, 2, 5, faint);
        }
    }

    // Illustrative events only, to establish the compact visual language.
    const int events[][4] = {
        {0, 390, 0, 0}, {1, 145, 0, 0},
        {3, 40, 0, 0}, {3, 55, 0, 0}, {3, 105, 0, 0},
        {4, 285, 0, 0}, {5, 310, 0, 0}, {5, 365, 0, 0},
        {6, 260, 0, 0}, {6, 315, 0, 0}, {6, 360, 0, 0},
        {7, 330, 0, 0}, {8, 180, 0, 0}, {8, 225, 0, 0},
        {8, 245, 0, 0}, {8, 260, 0, 0}, {9, 120, 0, 0},
        {10, 80, 0, 0}
    };
    for (int i = 0; i < sizeof(events)/sizeof(events[0]); ++i) {
        int x = timelineX + events[i][1];
        int y = rowTop + events[i][0] * rowH - 7;
        Fill(dc, x, y, 3, 6, white);
    }

    int metaY = rowTop + TRACK_COUNT * rowH + 18;
    Txt(dc, labelX, metaY, _T("MIDI comments"), label, 10);
    Txt(dc, timelineX, metaY, _T("comments   - standard MIDI text"), RGB(190, 190, 205), 12);
    Txt(dc, timelineX, metaY + 17, _T("comment line 2"), RGB(190, 190, 205), 12);
    Txt(dc, timelineX, metaY + 48, _T("Jour_de_fete.MID"), white, 19);
    Txt(dc, cr.right - 125, metaY + 38, _T("tempo   120"), label, 12);
    Txt(dc, cr.right - 125, metaY + 57, _T("MEASURE   23"), label, 12);
    Txt(dc, cr.right - 125, metaY + 76, _T("signature   4 / 4"), label, 12);

    int barY = cr.bottom - 75;
    Fill(dc, 0, barY, cr.right, 30, RGB(105, 111, 112));
    Fill(dc, 0, barY + 30, cr.right, 45, RGB(57, 62, 65));
    Fill(dc, 0, barY, cr.right, 2, RGB(150, 155, 155));
    Fill(dc, 0, barY + 29, cr.right, 1, RGB(30, 34, 36));
    Fill(dc, 215, barY + 2, 5, 26, RGB(225, 225, 220));
    Txt(dc, cr.right - 145, barY + 5, _T("00:25:39 / 01:03:55"), white, 12, FW_SEMIBOLD);

    Txt(dc, 24, barY + 39, _T(">     ■     <     >     |>>"), white, 16);
    Txt(dc, cr.right / 2 - 24, barY + 40, _T("LOAD"), white, 13, FW_SEMIBOLD);
    Txt(dc, cr.right / 2 + 45, barY + 40, _T("Options"), label, 13);
    Txt(dc, cr.right - 118, barY + 40, _T("VOL  |||||||"), white, 12);
    EndPaint(hwnd, &ps);
}
static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_PAINT: Paint(hwnd); return 0;
    case WM_CLOSE: SaveWindow(hwnd); DestroyWindow(hwnd); return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}
int APIENTRY WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int show) {
    TCHAR exe[MAX_PATH];
    GetModuleFileName(NULL, exe, MAX_PATH);
    lstrcpyn(g_iniPath, exe, MAX_PATH);
    TCHAR* slash = _tcsrchr(g_iniPath, _T('\\'));
    if (slash) *(slash + 1) = 0;
    _tcscat_s(g_iniPath, MAX_PATH, _T("SETTINGS.INI"));

    int x = GetPrivateProfileInt(_T("Window"), _T("X"), 100, g_iniPath);
    int y = GetPrivateProfileInt(_T("Window"), _T("Y"), 100, g_iniPath);
    int w = GetPrivateProfileInt(_T("Window"), _T("Width"), WIN_W, g_iniPath);
    int h = GetPrivateProfileInt(_T("Window"), _T("Height"), WIN_H, g_iniPath);
    if (w < 500 || w > 2000) w = WIN_W;
    if (h < 350 || h > 1400) h = WIN_H;

    const TCHAR CLASS_NAME[] = _T("MIDI_PIXEL_FIRST_WINDOW");
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc; wc.hInstance = instance;
    wc.lpszClassName = CLASS_NAME; wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    if (!RegisterClass(&wc)) return 1;

    TCHAR title[128]; _stprintf_s(title, _T("VISTA12HEURES_test %s"), VERSION);
    // Fixed-size for this alpha; resizing can be added once the layout is ready to scale.
    HWND hwnd = CreateWindowEx(0, CLASS_NAME, title,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        x, y, w, h, NULL, NULL, instance, NULL);
    if (!hwnd) return 2;
    ShowWindow(hwnd, show); UpdateWindow(hwnd);
    MSG msg; while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg); DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
