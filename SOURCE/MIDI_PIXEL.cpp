#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <tchar.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>

static const TCHAR VERSION[] = _T("vA0.007");
static TCHAR g_iniPath[MAX_PATH] = {};
static TCHAR g_fileName[MAX_PATH] = {};
static const int WIN_W = 760, WIN_H = 500;
static const int MIDI_CHANNELS = 16, MIDI_NOTES = 128;

struct MidiEvent {
    unsigned long long tick;
    unsigned long long usec;
    int track, channel, type, a, b;
};
struct TempoEvent { unsigned long long tick; unsigned int usecPerQuarter; };
struct TrackInfo { std::string name; unsigned long long endTick; };

static std::vector<MidiEvent> g_events;
static std::vector<TrackInfo> g_tracks;
static unsigned char g_active[MIDI_CHANNELS][MIDI_NOTES] = {};
static int g_velocity[MIDI_CHANNELS][MIDI_NOTES] = {};
static int g_program[MIDI_CHANNELS] = {};
static unsigned long long g_totalUsec = 0, g_elapsedUsec = 0;
static unsigned long long g_lastMs = 0;
static size_t g_eventIndex = 0;
static unsigned long long g_maxTick = 0;
static unsigned short g_division = 480;
static unsigned int g_initialTempo = 500000;
static bool g_loaded = false, g_playing = false, g_finished = false;
static HDC g_backDC = NULL;
static HBITMAP g_backBitmap = NULL;
static HBITMAP g_oldBackBitmap = NULL;
static int g_backWidth = 0, g_backHeight = 0;

static unsigned short ReadBE16(const unsigned char* p) {
    return (unsigned short)((p[0] << 8) | p[1]);
}
static unsigned int ReadBE32(const unsigned char* p) {
    return ((unsigned int)p[0] << 24) | ((unsigned int)p[1] << 16) | ((unsigned int)p[2] << 8) | p[3];
}
static bool ReadVLQ(const std::vector<unsigned char>& b, size_t& p, size_t end, unsigned int& value) {
    value = 0;
    for (int n = 0; n < 4; ++n) {
        if (p >= end) return false;
        unsigned char c = b[p++];
        value = (value << 7) | (c & 0x7f);
        if (!(c & 0x80)) return true;
    }
    return false;
}
static void ClearPlayback() {
    memset(g_active, 0, sizeof(g_active));
    memset(g_velocity, 0, sizeof(g_velocity));
    memset(g_program, 0, sizeof(g_program));
    g_elapsedUsec = 0;
    g_eventIndex = 0;
    g_lastMs = 0;
    g_finished = false;
}
static bool LoadMidiFile(const TCHAR* path) {
    FILE* f = NULL;
    if (_tfopen_s(&f, path, _T("rb")) != 0 || !f) return false;
    fseek(f, 0, SEEK_END);
    long fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (fileSize < 14 || fileSize > 64 * 1024 * 1024) { fclose(f); return false; }
    std::vector<unsigned char> bytes((size_t)fileSize);
    if (fread(bytes.data(), 1, bytes.size(), f) != bytes.size()) { fclose(f); return false; }
    fclose(f);
    if (memcmp(bytes.data(), "MThd", 4) != 0 || ReadBE32(&bytes[4]) < 6) return false;
    unsigned short format = ReadBE16(&bytes[8]);
    unsigned short trackCount = ReadBE16(&bytes[10]);
    unsigned short division = ReadBE16(&bytes[12]);
    if (format > 1 || trackCount == 0 || (division & 0x8000)) return false;

    std::vector<MidiEvent> events;
    std::vector<TempoEvent> tempos;
    std::vector<TrackInfo> tracks;
    size_t pos = 8 + ReadBE32(&bytes[4]);
    unsigned long long maxTick = 0;
    for (unsigned int ti = 0; ti < trackCount; ++ti) {
        if (pos + 8 > bytes.size() || memcmp(&bytes[pos], "MTrk", 4) != 0) return false;
        unsigned int chunkLen = ReadBE32(&bytes[pos + 4]);
        pos += 8;
        if (pos + chunkLen > bytes.size()) return false;
        size_t p = pos, end = pos + chunkLen;
        unsigned long long tick = 0;
        unsigned char running = 0;
        TrackInfo track;
        while (p < end) {
            unsigned int delta = 0;
            if (!ReadVLQ(bytes, p, end, delta)) return false;
            tick += delta;
            if (p >= end) break;
            unsigned char status = bytes[p];
            if (status & 0x80) {
                ++p;
                if (status < 0xf0) running = status;
            } else {
                if (!running) return false;
                status = running;
            }

            if (status == 0xff) {
                if (p >= end) return false;
                unsigned char metaType = bytes[p++];
                unsigned int len = 0;
                if (!ReadVLQ(bytes, p, end, len) || p + len > end) return false;
                const unsigned char* payload = &bytes[p];
                if (metaType == 0x03 && len > 0) track.name.assign((const char*)payload, (size_t)len);
                if (metaType == 0x51 && len == 3) {
                    unsigned int tempo = ((unsigned int)payload[0] << 16) | ((unsigned int)payload[1] << 8) | payload[2];
                    if (tempo) tempos.push_back({ tick, tempo });
                }
                p += len;
                if (metaType == 0x2f) break;
            } else if (status == 0xf0 || status == 0xf7) {
                unsigned int len = 0;
                if (!ReadVLQ(bytes, p, end, len) || p + len > end) return false;
                p += len;
                running = 0;
            } else {
                int command = status & 0xf0;
                int channel = status & 0x0f;
                int dataCount = (command == 0xc0 || command == 0xd0) ? 1 : 2;
                if (p + dataCount > end) return false;
                int a = bytes[p++], b = dataCount == 2 ? bytes[p++] : 0;
                if (a & 0x80 || b & 0x80) return false;
                if (command == 0x80) events.push_back({ tick, 0, (int)ti, channel, 0, a, b });
                else if (command == 0x90) events.push_back({ tick, 0, (int)ti, channel, b ? 1 : 0, a, b });
                else if (command == 0xc0) events.push_back({ tick, 0, (int)ti, channel, 2, a, 0 });
            }
        }
        track.endTick = tick;
        if (tick > maxTick) maxTick = tick;
        tracks.push_back(track);
        pos = end;
    }
    if (events.empty()) return false;

    std::sort(tempos.begin(), tempos.end(), [](const TempoEvent& a, const TempoEvent& b) { return a.tick < b.tick; });
    std::sort(events.begin(), events.end(), [](const MidiEvent& a, const MidiEvent& b) {
        if (a.tick != b.tick) return a.tick < b.tick;
        if (a.track != b.track) return a.track < b.track;
        return a.type < b.type;
    });

    unsigned int tempo = 500000;
    unsigned long long lastTick = 0, elapsed = 0;
    size_t tempoIndex = 0;
    while (tempoIndex < tempos.size() && tempos[tempoIndex].tick == 0) {
        tempo = tempos[tempoIndex].usecPerQuarter;
        ++tempoIndex;
    }
    for (size_t i = 0; i < events.size(); ++i) {
        while (tempoIndex < tempos.size() && tempos[tempoIndex].tick <= events[i].tick) {
            unsigned long long dt = tempos[tempoIndex].tick - lastTick;
            elapsed += (dt * (unsigned long long)tempo) / division;
            lastTick = tempos[tempoIndex].tick;
            tempo = tempos[tempoIndex].usecPerQuarter;
            ++tempoIndex;
        }
        unsigned long long dt = events[i].tick - lastTick;
        elapsed += (dt * (unsigned long long)tempo) / division;
        lastTick = events[i].tick;
        events[i].usec = elapsed;
    }
    while (tempoIndex < tempos.size()) {
        unsigned long long dt = tempos[tempoIndex].tick - lastTick;
        elapsed += (dt * (unsigned long long)tempo) / division;
        lastTick = tempos[tempoIndex].tick;
        tempo = tempos[tempoIndex].usecPerQuarter;
        ++tempoIndex;
    }
    unsigned long long tail = maxTick > lastTick ? maxTick - lastTick : 0;
    elapsed += (tail * (unsigned long long)tempo) / division;

    g_events.swap(events);
    g_tracks.swap(tracks);
    g_division = division;
    g_initialTempo = tempos.empty() ? 500000 : tempos[0].usecPerQuarter;
    g_maxTick = maxTick;
    g_totalUsec = elapsed;
    g_loaded = true;
    g_playing = false;
    ClearPlayback();
    return true;
}
static RECT NoteRect(int channel, int note, int clientWidth) {
    const int timelineX = 225, timelineR = clientWidth - 28, rowTop = 42, rowH = 18;
    int x = timelineX + (note * (timelineR - timelineX - 4)) / 127;
    int y = rowTop + channel * rowH - 7;
    RECT r = { x, y, x + 4, y + 6 };
    return r;
}
static void InvalidateDynamic(HWND hwnd) {
    RECT r;
    GetClientRect(hwnd, &r);
    RECT notes = { 220, 32, r.right - 20, 42 + MIDI_CHANNELS * 18 };
    InvalidateRect(hwnd, &notes, FALSE);
    RECT presets = { 68, 32, 130, 42 + MIDI_CHANNELS * 18 };
    InvalidateRect(hwnd, &presets, FALSE);
    RECT progress = { 0, r.bottom - 75, r.right, r.bottom - 40 };
    InvalidateRect(hwnd, &progress, FALSE);
}
static void AdvancePlayback(HWND hwnd) {
    if (!g_playing) return;
    unsigned long long now = GetTickCount64();
    if (g_lastMs) g_elapsedUsec += (now - g_lastMs) * 1000;
    g_lastMs = now;
    RECT client; GetClientRect(hwnd, &client);
    bool dynamicChanged = false;
    while (g_eventIndex < g_events.size() && g_events[g_eventIndex].usec <= g_elapsedUsec) {
        const MidiEvent& e = g_events[g_eventIndex++];
        if (e.type == 2) {
            if (g_program[e.channel] != e.a) {
                g_program[e.channel] = e.a;
                RECT pr = { 68, 42 + e.channel * 18 - 6, 130, 42 + e.channel * 18 + 12 };
                InvalidateRect(hwnd, &pr, FALSE);
            }
        } else {
            RECT oldRect = NoteRect(e.channel, e.a, client.right);
            if (e.type == 1) {
                g_active[e.channel][e.a] = 1;
                g_velocity[e.channel][e.a] = e.b;
            } else {
                g_active[e.channel][e.a] = 0;
                g_velocity[e.channel][e.a] = 0;
            }
            RECT newRect = NoteRect(e.channel, e.a, 760);
            InvalidateRect(hwnd, &oldRect, FALSE);
            InvalidateRect(hwnd, &newRect, FALSE);
            dynamicChanged = true;
        }
    }
    if (g_elapsedUsec >= g_totalUsec) {
        g_playing = false;
        g_finished = true;
    }
    RECT client; GetClientRect(hwnd, &client);
    RECT progress = { 0, client.bottom - 75, client.right, client.bottom };
    InvalidateRect(hwnd, &progress, FALSE);
    (void)dynamicChanged;
}
static void StartPlayback() {
    if (!g_loaded) return;
    if (g_finished || g_elapsedUsec >= g_totalUsec) ClearPlayback();
    g_playing = true;
    g_lastMs = GetTickCount64();
}
static void StopPlayback() {
    g_playing = false;
    ClearPlayback();
}
static void LoadDialog(HWND hwnd) {
    TCHAR path[MAX_PATH] = {};
    OPENFILENAME ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = _T("Fichiers MIDI (*.mid;*.midi)\0*.mid;*.midi\0Tous les fichiers (*.*)\0*.*\0");
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrTitle = _T("Charger un fichier MIDI");
    if (GetOpenFileName(&ofn)) {
        if (LoadMidiFile(path)) {
            lstrcpyn(g_fileName, path, MAX_PATH);
            TCHAR* base = _tcsrchr(g_fileName, _T('\\'));
            if (base) lstrcpyn(g_fileName, base + 1, MAX_PATH);
            DestroyBackBuffer(); InvalidateRect(hwnd, NULL, FALSE); InvalidateDynamic(hwnd);
        } else {
            MessageBox(hwnd, _T("Fichier MIDI non reconnu ou format non pris en charge."), _T("MIDI PIXEL"), MB_ICONWARNING | MB_OK);
        }
    }
}
static void FillRectColor(HDC dc, RECT r, COLORREF color) {
    HBRUSH b = CreateSolidBrush(color); FillRect(dc, &r, b); DeleteObject(b);
}
static void Txt(HDC dc, int x, int y, LPCTSTR s, COLORREF color, int size = 13, int weight = FW_NORMAL) {
    SetBkMode(dc, TRANSPARENT); SetTextColor(dc, color);
    HFONT f = CreateFont(size, 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("Segoe UI"));
    HFONT old = (HFONT)SelectObject(dc, f); TextOut(dc, x, y, s, lstrlen(s)); SelectObject(dc, old); DeleteObject(f);
}
static void DrawLine(HDC dc, int x1, int y, int x2, COLORREF color) {
    HPEN p = CreatePen(PS_SOLID, 2, color); HPEN old = (HPEN)SelectObject(dc, p);
    MoveToEx(dc, x1, y, NULL); LineTo(dc, x2, y); SelectObject(dc, old); DeleteObject(p);
}
static void SaveWindow(HWND hwnd) {
    if (IsIconic(hwnd) || IsZoomed(hwnd)) return;
    RECT r;
    if (!GetWindowRect(hwnd, &r)) return;
    TCHAR n[32];
    _stprintf_s(n, _T("%d"), r.left); WritePrivateProfileString(_T("Window"), _T("X"), n, g_iniPath);
    _stprintf_s(n, _T("%d"), r.top); WritePrivateProfileString(_T("Window"), _T("Y"), n, g_iniPath);
    _stprintf_s(n, _T("%d"), r.right - r.left); WritePrivateProfileString(_T("Window"), _T("Width"), n, g_iniPath);
    _stprintf_s(n, _T("%d"), r.bottom - r.top); WritePrivateProfileString(_T("Window"), _T("Height"), n, g_iniPath);
}
static void DrawStaticBackground(HDC dc, int width, int height) {
    RECT cr = { 0, 0, width, height };
    FillRectColor(dc, cr, RGB(49, 46, 72));
    const COLORREF label = RGB(190, 190, 205), line = RGB(105, 103, 139);
    const int channelX = 24, presetX = 72, timelineX = 225, timelineR = width - 28;
    const int rowTop = 42, rowH = 18;
    Txt(dc, 24, 10, _T("MIDI PIXEL"), RGB(215, 218, 235), 15, FW_SEMIBOLD);
    Txt(dc, width - 82, 12, VERSION, RGB(155, 160, 190), 11);
    Txt(dc, channelX, 27, _T("CH"), label, 9, FW_SEMIBOLD);
    Txt(dc, presetX, 27, _T("GM-PRESET"), label, 9, FW_SEMIBOLD);
    for (int i = 0; i < MIDI_CHANNELS; ++i) {
        int y = rowTop + i * rowH;
        TCHAR channelText[8];
        _stprintf_s(channelText, _T("%02d"), i + 1);
        Txt(dc, channelX, y - 5, channelText, label, 10);
        DrawLine(dc, timelineX, y + 2, timelineR, line);
    }
    int infoY = rowTop + MIDI_CHANNELS * rowH + 8;
    Txt(dc, channelX, infoY, _T("MIDI FILE"), label, 10);
    Txt(dc, timelineX, infoY, g_fileName[0] ? g_fileName : _T("No MIDI file loaded"), RGB(235, 235, 242), 14);
    TCHAR info[128];
    if (g_loaded) {
        _stprintf_s(info, _T("%u tracks   /   %u ticks per quarter"), (unsigned int)g_tracks.size(), (unsigned int)g_division);
        Txt(dc, timelineX, infoY + 22, info, label, 10);
        _stprintf_s(info, _T("tempo initial : %u BPM"), (unsigned int)(60000000ULL / g_initialTempo));
        Txt(dc, timelineX, infoY + 39, info, label, 10);
    }
    int barY = height - 75;
    FillRectColor(dc, RECT{ 0, barY, width, barY + 30 }, RGB(105, 111, 112));
    FillRectColor(dc, RECT{ 0, barY + 30, width, height }, RGB(57, 62, 65));
    FillRectColor(dc, RECT{ 0, barY, width, barY + 2 }, RGB(150, 155, 155));
    Txt(dc, 24, barY + 39, _T("STOP"), RGB(235, 235, 242), 12, FW_SEMIBOLD);
    Txt(dc, 92, barY + 39, _T("PLAY"), RGB(235, 235, 242), 12, FW_SEMIBOLD);
    Txt(dc, width / 2 - 24, barY + 39, _T("LOAD"), RGB(235, 235, 242), 13, FW_SEMIBOLD);
    Txt(dc, width / 2 + 45, barY + 39, _T("Options"), label, 13);
}
static void DestroyBackBuffer() {
    if (g_backDC) {
        if (g_oldBackBitmap) SelectObject(g_backDC, g_oldBackBitmap);
        if (g_backBitmap) DeleteObject(g_backBitmap);
        DeleteDC(g_backDC);
    }
    g_backDC = NULL; g_backBitmap = NULL; g_oldBackBitmap = NULL;
    g_backWidth = g_backHeight = 0;
}
static bool EnsureBackBuffer(HWND hwnd) {
    RECT cr; GetClientRect(hwnd, &cr);
    int width = cr.right, height = cr.bottom;
    if (g_backDC && width == g_backWidth && height == g_backHeight) return true;
    DestroyBackBuffer();
    HDC screen = GetDC(hwnd);
    if (!screen) return false;
    g_backDC = CreateCompatibleDC(screen);
    g_backBitmap = CreateCompatibleBitmap(screen, width, height);
    ReleaseDC(hwnd, screen);
    if (!g_backDC || !g_backBitmap) { DestroyBackBuffer(); return false; }
    g_oldBackBitmap = (HBITMAP)SelectObject(g_backDC, g_backBitmap);
    g_backWidth = width; g_backHeight = height;
    DrawStaticBackground(g_backDC, width, height);
    return true;
}
static void Paint(HWND hwnd) {
    PAINTSTRUCT ps; HDC dc = BeginPaint(hwnd, &ps);
    RECT cr; GetClientRect(hwnd, &cr);
    if (EnsureBackBuffer(hwnd)) BitBlt(dc, ps.rcPaint.left, ps.rcPaint.top,
        ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top,
        g_backDC, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);
    else FillRectColor(dc, ps.rcPaint, RGB(49, 46, 72));

    const int timelineX = 225, timelineR = cr.right - 28, rowTop = 42, rowH = 18;
    for (int ch = 0; ch < MIDI_CHANNELS; ++ch) {
        TCHAR presetText[8];
        _stprintf_s(presetText, _T("%03d"), g_program[ch] + 1);
        Txt(dc, 72, rowTop + ch * rowH - 5, g_loaded ? presetText : _T("---"), RGB(160, 170, 195), 10);
        if (!g_loaded) continue;
        for (int note = 0; note < MIDI_NOTES; ++note) {
            if (!g_active[ch][note]) continue;
            RECT nr = NoteRect(ch, note, cr.right);
            if (nr.right < ps.rcPaint.left || nr.left > ps.rcPaint.right ||
                nr.bottom < ps.rcPaint.top || nr.top > ps.rcPaint.bottom) continue;
            int intensity = 170 + (g_velocity[ch][note] * 85) / 127;
            FillRectColor(dc, nr, RGB(intensity, intensity, 245));
        }
    }
    int barY = cr.bottom - 75;
    FillRectColor(dc, RECT{ 4, barY + 2, 4 + (g_totalUsec ? (int)((g_elapsedUsec * (cr.right - 8)) / g_totalUsec) : 0), barY + 26 }, RGB(225, 225, 220));
    Txt(dc, cr.right - 115, barY + 5, g_playing ? _T("PLAYING") : (g_finished ? _T("FINISHED") : _T("STOPPED")), RGB(235, 235, 242), 11, FW_SEMIBOLD);
    EndPaint(hwnd, &ps);
}
static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_PAINT: Paint(hwnd); return 0;
    case WM_SIZE: DestroyBackBuffer(); InvalidateRect(hwnd, NULL, FALSE); return 0;
    case WM_TIMER: AdvancePlayback(hwnd); return 0;
    case WM_LBUTTONDOWN: {
        RECT client; GetClientRect(hwnd, &client);
        int x = LOWORD(lp), y = HIWORD(lp), barY = client.bottom - 75;
        if (y >= barY + 30) {
            if (x < 75) StopPlayback();
            else if (x < 150) StartPlayback();
            else if (x >= 260 && x < 390) LoadDialog(hwnd);
            InvalidateDynamic(hwnd);
        }
        return 0;
    }
    case WM_KEYDOWN:
        if (wp == VK_SPACE) { if (g_playing) g_playing = false; else StartPlayback(); InvalidateDynamic(hwnd); return 0; }
        if (wp == VK_ESCAPE) { StopPlayback(); InvalidateDynamic(hwnd); return 0; }
        break;
    case WM_CLOSE: KillTimer(hwnd, 1); SaveWindow(hwnd); DestroyBackBuffer(); DestroyWindow(hwnd); return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}
int APIENTRY WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int show) {
    TCHAR exe[MAX_PATH]; GetModuleFileName(NULL, exe, MAX_PATH);
    lstrcpyn(g_iniPath, exe, MAX_PATH);
    TCHAR* slash = _tcsrchr(g_iniPath, _T('\\')); if (slash) *(slash + 1) = 0;
    _tcscat_s(g_iniPath, MAX_PATH, _T("SETTINGS.INI"));
    int x = GetPrivateProfileInt(_T("Window"), _T("X"), 100, g_iniPath);
    int y = GetPrivateProfileInt(_T("Window"), _T("Y"), 100, g_iniPath);
    int w = GetPrivateProfileInt(_T("Window"), _T("Width"), WIN_W, g_iniPath);
    int h = GetPrivateProfileInt(_T("Window"), _T("Height"), WIN_H, g_iniPath);
    if (w < 500 || w > 2000) w = WIN_W; if (h < 350 || h > 1400) h = WIN_H;
    const TCHAR CLASS_NAME[] = _T("MIDI_PIXEL_FIRST_WINDOW");
    WNDCLASS wc = {}; wc.lpfnWndProc = WindowProc; wc.hInstance = instance; wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    if (!RegisterClass(&wc)) return 1;
    TCHAR title[128]; _stprintf_s(title, _T("VISTA12HEURES_test %s"), VERSION);
    HWND hwnd = CreateWindowEx(0, CLASS_NAME, title, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        x, y, w, h, NULL, NULL, instance, NULL);
    if (!hwnd) return 2;
    ShowWindow(hwnd, show); UpdateWindow(hwnd); SetTimer(hwnd, 1, 10, NULL);
    MSG msg; while (GetMessage(&msg, NULL, 0, 0) > 0) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return (int)msg.wParam;
}
