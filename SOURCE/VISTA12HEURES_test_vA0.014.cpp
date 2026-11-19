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
#include <cmath>

static const TCHAR VERSION[] = _T("vA0.015");
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
struct TimeSignatureEvent { unsigned long long tick; unsigned char numerator, denominatorPower, clocksPerClick, thirtySecondsPerQuarter; };
struct TrackInfo { std::string name; unsigned long long endTick; };

static std::vector<MidiEvent> g_events;
static std::vector<TrackInfo> g_tracks;
static unsigned char g_active[MIDI_CHANNELS][MIDI_NOTES] = {};
static int g_velocity[MIDI_CHANNELS][MIDI_NOTES] = {};
static COLORREF g_noteColors[12] = {
 RGB(232,222,90), RGB(207,207,134), RGB(70,70,70), RGB(173,53,53),
 RGB(212,92,53), RGB(210,222,222), RGB(101,133,195), RGB(30,65,133),
 RGB(47,111,207), RGB(90,175,114), RGB(8,212,79), RGB(58,84,51)
};
static double g_velocityMinimum = 0.10, g_velocityCurve = 1.00;
static COLORREF g_uiBackground = RGB(49,46,72);
static COLORREF g_uiLines = RGB(105,103,139);
static COLORREF g_uiProgress = RGB(225,225,220);
static COLORREF g_uiTransportTop = RGB(105,111,112);
static COLORREF g_uiTransportBottom = RGB(57,62,65);
static COLORREF g_uiTransportEdge = RGB(150,155,155);
static int g_progressHeight = 24;
static int g_lineThickness = 2;
static int g_firstVisibleNote = 12;
static int g_lastVisibleNote = 115;
static COLORREF g_linesC3Color = RGB(255, 210, 90);
static COLORREF g_linesCColor = RGB(125, 145, 190);
static int g_program[MIDI_CHANNELS] = {};
static int g_initialProgram[MIDI_CHANNELS] = {};
static TCHAR g_programNames[128][64] = {};
static unsigned long long g_totalUsec = 0, g_elapsedUsec = 0;
static unsigned long long g_lastMs = 0;
static size_t g_eventIndex = 0;
static unsigned long long g_maxTick = 0;
static unsigned short g_division = 480;
static unsigned int g_initialTempo = 500000;
static unsigned int g_currentTempo = 500000;
static unsigned char g_initialTimeNumerator = 4, g_initialTimeDenominatorPower = 2;
static unsigned char g_currentTimeNumerator = 4, g_currentTimeDenominatorPower = 2;
static bool g_loaded = false, g_playing = false, g_finished = false;
static void DestroyBackBuffer();
static HDC g_backDC = NULL;
static HBITMAP g_backBitmap = NULL;
static HBITMAP g_oldBackBitmap = NULL;
static int g_backWidth = 0, g_backHeight = 0;


static void LoadColorPalette() {
 const TCHAR* keys[12] = {_T("C"),_T("C_SHARP"),_T("D"),_T("D_SHARP"),_T("E"),_T("F"),_T("F_SHARP"),_T("G"),_T("G_SHARP"),_T("A"),_T("A_SHARP"),_T("B")};
 for (int i=0;i<12;++i) {
  TCHAR value[64]={}; GetPrivateProfileString(_T("Notes"),keys[i],_T(""),value,64,g_iniPath);
  int rgb[3]={GetRValue(g_noteColors[i]),GetGValue(g_noteColors[i]),GetBValue(g_noteColors[i])};
  int a=rgb[0],b=rgb[1],c=rgb[2];
  if (value[0] && _stscanf_s(value,_T("%d , %d , %d"),&a,&b,&c)==3) {
   if(a>=0&&a<=255)rgb[0]=a; if(b>=0&&b<=255)rgb[1]=b; if(c>=0&&c<=255)rgb[2]=c;
  }
  g_noteColors[i]=RGB(rgb[0],rgb[1],rgb[2]);
 }
 int minimum=GetPrivateProfileInt(_T("Velocity"),_T("MinimumPercent"),10,g_iniPath);
 if(minimum<0)minimum=0; if(minimum>100)minimum=100;
 g_velocityMinimum=minimum/100.0;
 TCHAR curve[32]={}; GetPrivateProfileString(_T("Velocity"),_T("Curve"),_T("1.00"),curve,32,g_iniPath);
 double v=_tcstod(curve,NULL); if(v<0.10||v>4.00)v=1.00; g_velocityCurve=v;
}
static double SrgbToLinear(int x) {
 double c=x/255.0; return c<=0.04045?c/12.92:pow((c+0.055)/1.055,2.4);
}
static int LinearToSrgb(double x) {
 if(x<0)x=0; if(x>1)x=1;
 double c=x<=0.0031308?12.92*x:1.055*pow(x,1.0/2.4)-0.055;
 int n=(int)(c*255.0+0.5); if(n<0)n=0; if(n>255)n=255; return n;
}
static COLORREF ColorForNote(int note,int velocity) {
 COLORREF base=g_noteColors[note%12];
 double v=velocity/127.0;
 double factor=g_velocityMinimum+(1.0-g_velocityMinimum)*pow(v,g_velocityCurve);
 return RGB(LinearToSrgb(SrgbToLinear(GetRValue(base))*factor),
            LinearToSrgb(SrgbToLinear(GetGValue(base))*factor),
            LinearToSrgb(SrgbToLinear(GetBValue(base))*factor));
}


static COLORREF ReadIniRgb(const TCHAR* section, const TCHAR* key, COLORREF fallback) {
    TCHAR value[64] = {};
    GetPrivateProfileString(section, key, _T(""), value, 64, g_iniPath);
    if (!value[0]) return fallback;
    int r = GetRValue(fallback), g = GetGValue(fallback), b = GetBValue(fallback);
    if (_stscanf_s(value, _T("%d , %d , %d"), &r, &g, &b) != 3) return fallback;
    if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) return fallback;
    return RGB(r, g, b);
}
static void LoadInterfaceColors() {
    g_uiBackground = ReadIniRgb(_T("Interface"), _T("Background"), RGB(49,46,72));
    g_uiLines = ReadIniRgb(_T("Interface"), _T("Lines"), RGB(105,103,139));
    g_uiProgress = ReadIniRgb(_T("Interface"), _T("Progress"), RGB(225,225,220));
    g_uiTransportTop = ReadIniRgb(_T("Interface"), _T("TransportTop"), RGB(105,111,112));
    g_uiTransportBottom = ReadIniRgb(_T("Interface"), _T("TransportBottom"), RGB(57,62,65));
    g_uiTransportEdge = ReadIniRgb(_T("Interface"), _T("TransportEdge"), RGB(150,155,155));
    g_progressHeight = GetPrivateProfileInt(_T("Interface"), _T("ProgressHeight"), 24, g_iniPath);
    if (g_progressHeight < 2) g_progressHeight = 2;
    if (g_progressHeight > 28) g_progressHeight = 28;
    g_lineThickness = GetPrivateProfileInt(_T("Display"), _T("LineThickness"), 2, g_iniPath);
    if (g_lineThickness < 1) g_lineThickness = 1;
    if (g_lineThickness > 8) g_lineThickness = 8;
    g_firstVisibleNote = GetPrivateProfileInt(_T("Display"), _T("FirstNote"), 12, g_iniPath);
    g_lastVisibleNote = GetPrivateProfileInt(_T("Display"), _T("LastNote"), 115, g_iniPath);
    if (g_firstVisibleNote < 0) g_firstVisibleNote = 0;
    if (g_firstVisibleNote > 126) g_firstVisibleNote = 126;
    if (g_lastVisibleNote < 1) g_lastVisibleNote = 1;
    if (g_lastVisibleNote > 127) g_lastVisibleNote = 127;
    if (g_lastVisibleNote <= g_firstVisibleNote) { g_firstVisibleNote = 12; g_lastVisibleNote = 115; }
    g_linesC3Color = ReadIniRgb(_T("Display"), _T("LinesC3color"), RGB(255,210,90));
    g_linesCColor = ReadIniRgb(_T("Display"), _T("LinesCcolor"), RGB(125,145,190));
}

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

static bool ReadXmlAttribute(const std::string& tag, const char* attribute, std::string& value) {
    std::string key = attribute;
    key += "=";
    size_t p = tag.find(key);
    if (p == std::string::npos) return false;
    p += key.size();
    if (p >= tag.size() || (tag[p] != '"' && tag[p] != '\'')) return false;
    char quote = tag[p++];
    size_t end = tag.find(quote, p);
    if (end == std::string::npos) return false;
    value.assign(tag, p, end - p);
    return true;
}
static void LoadProgramNames() {
    TCHAR path[MAX_PATH];
    lstrcpyn(path, g_iniPath, MAX_PATH);
    TCHAR* slash = _tcsrchr(path, _T('\\'));
    if (slash) *(slash + 1) = 0;
    _tcscat_s(path, MAX_PATH, _T("GM-XG.XML"));

    FILE* f = NULL;
    if (_tfopen_s(&f, path, _T("rb")) != 0 || !f) return;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size <= 0 || size > 1024 * 1024) { fclose(f); return; }
    std::string xml((size_t)size, '\0');
    size_t read = fread(&xml[0], 1, xml.size(), f);
    fclose(f);
    if (read != xml.size()) return;

    size_t pos = 0;
    while ((pos = xml.find("<Program", pos)) != std::string::npos) {
        size_t end = xml.find('>', pos);
        if (end == std::string::npos) break;
        std::string tag = xml.substr(pos, end - pos + 1);
        std::string numberText, name;
        if (ReadXmlAttribute(tag, "number", numberText) &&
            ReadXmlAttribute(tag, "name", name)) {
            int number = atoi(numberText.c_str());
            if (number >= 1 && number <= 128 && !name.empty()) {
                #ifdef UNICODE
                MultiByteToWideChar(CP_UTF8, 0, name.c_str(), -1, g_programNames[number - 1], 64);
                #else
                int chars = MultiByteToWideChar(CP_UTF8, 0, name.c_str(), -1, NULL, 0);
                if (chars > 0) {
                    WCHAR wideName[64] = {};
                    if (chars > 64) chars = 64;
                    MultiByteToWideChar(CP_UTF8, 0, name.c_str(), -1, wideName, chars);
                    WideCharToMultiByte(CP_ACP, 0, wideName, -1, g_programNames[number - 1], 64, NULL, NULL);
                }
                #endif
            }
        }
        pos = end + 1;
    }
}

static void ClearPlayback() {
    memset(g_active, 0, sizeof(g_active));
    memset(g_velocity, 0, sizeof(g_velocity));
    memcpy(g_program, g_initialProgram, sizeof(g_program));
    g_elapsedUsec = 0;
    g_eventIndex = 0;
    g_lastMs = 0;
    g_currentTempo = g_initialTempo;
    g_currentTimeNumerator = g_initialTimeNumerator;
    g_currentTimeDenominatorPower = g_initialTimeDenominatorPower;
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
    std::vector<TimeSignatureEvent> timeSignatures;
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
                if (metaType == 0x58 && len == 4 && payload[0] > 0 && payload[1] <= 7) {
                    timeSignatures.push_back({ tick, payload[0], payload[1], payload[2], payload[3] });
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
    std::sort(timeSignatures.begin(), timeSignatures.end(), [](const TimeSignatureEvent& a, const TimeSignatureEvent& b) { return a.tick < b.tick; });
    for (size_t i = 0; i < tempos.size(); ++i)
        events.push_back({ tempos[i].tick, 0, -1, 0, 3, (int)tempos[i].usecPerQuarter, 0 });
    for (size_t i = 0; i < timeSignatures.size(); ++i)
        events.push_back({ timeSignatures[i].tick, 0, -1, 0, 4, timeSignatures[i].numerator, timeSignatures[i].denominatorPower });
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
    memset(g_initialProgram, 0, sizeof(g_initialProgram));
    bool initialProgramFound[MIDI_CHANNELS] = {};
    for (size_t i = 0; i < g_events.size(); ++i) {
        const MidiEvent& e = g_events[i];
        if (e.type == 2 && !initialProgramFound[e.channel]) {
            g_initialProgram[e.channel] = e.a;
            initialProgramFound[e.channel] = true;
        }
    }
    g_division = division;
    g_initialTempo = 500000;
    g_initialTimeNumerator = 4;
    g_initialTimeDenominatorPower = 2;
    for (size_t i = 0; i < g_events.size(); ++i) {
        const MidiEvent& e = g_events[i];
        if (e.tick != 0) break;
        if (e.type == 3) g_initialTempo = (unsigned int)e.a;
        else if (e.type == 4) { g_initialTimeNumerator = (unsigned char)e.a; g_initialTimeDenominatorPower = (unsigned char)e.b; }
    }
    g_maxTick = maxTick;
    g_totalUsec = elapsed;
    g_loaded = true;
    g_playing = false;
    ClearPlayback();
    return true;
}
static int NotePixelX(int note, int clientWidth) {
    const int timelineX = 225, timelineR = clientWidth - 28;
    return timelineX + ((note - g_firstVisibleNote) * (timelineR - timelineX - 4)) / (g_lastVisibleNote - g_firstVisibleNote);
}
static RECT NoteRect(int channel, int note, int clientWidth) {
    const int timelineX = 225, timelineR = clientWidth - 28, rowTop = 42, rowH = 18;
    if (note < g_firstVisibleNote || note > g_lastVisibleNote) return RECT{ -100, -100, -96, -94 };
    int x = NotePixelX(note, clientWidth);
    int y = rowTop + (MIDI_CHANNELS - 1 - channel) * rowH - 7;
    RECT r = { x, y, x + 4, y + 6 };
    return r;
}
static void InvalidateDynamic(HWND hwnd) {
    RECT r;
    GetClientRect(hwnd, &r);
    RECT notes = { 220, 32, r.right - 20, 42 + MIDI_CHANNELS * 18 };
    InvalidateRect(hwnd, &notes, FALSE);
    RECT presets = { 68, 32, 222, 42 + MIDI_CHANNELS * 18 };
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
        if (e.type == 3) {
            g_currentTempo = (unsigned int)e.a;
            RECT meta = { 220, 42 + MIDI_CHANNELS * 18 + 20, 560, 42 + MIDI_CHANNELS * 18 + 65 };
            InvalidateRect(hwnd, &meta, FALSE);
        } else if (e.type == 4) {
            g_currentTimeNumerator = (unsigned char)e.a;
            g_currentTimeDenominatorPower = (unsigned char)e.b;
            RECT meta = { 220, 42 + MIDI_CHANNELS * 18 + 20, 560, 42 + MIDI_CHANNELS * 18 + 65 };
            InvalidateRect(hwnd, &meta, FALSE);
        } else if (e.type == 2) {
            if (g_program[e.channel] != e.a) {
                g_program[e.channel] = e.a;
                RECT pr = { 68, 42 + (MIDI_CHANNELS - 1 - e.channel) * 18 - 6, 222, 42 + (MIDI_CHANNELS - 1 - e.channel) * 18 + 12 };
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
            RECT newRect = NoteRect(e.channel, e.a, client.right);
            InvalidateRect(hwnd, &oldRect, FALSE);
            InvalidateRect(hwnd, &newRect, FALSE);
            dynamicChanged = true;
        }
    }
    if (g_elapsedUsec >= g_totalUsec) {
        g_playing = false;
        g_finished = true;
    }
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

static void TxtClipped(HDC dc, RECT r, LPCTSTR s, COLORREF color, int size = 9, int weight = FW_NORMAL) {
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    HFONT f = CreateFont(size, 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, _T("Segoe UI"));
    HFONT old = (HFONT)SelectObject(dc, f);
    DrawText(dc, s, -1, &r, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
    SelectObject(dc, old);
    DeleteObject(f);
}

static void DrawLine(HDC dc, int x1, int y, int x2, COLORREF color) {
    HPEN p = CreatePen(PS_SOLID, g_lineThickness, color); HPEN old = (HPEN)SelectObject(dc, p);
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
    FillRectColor(dc, cr, g_uiBackground);
    const COLORREF label = RGB(190, 190, 205), line = g_uiLines;
    const int channelX = 24, presetX = 72, timelineX = 225, timelineR = width - 28;
    const int rowTop = 42, rowH = 18;
    Txt(dc, 24, 10, _T("MIDI PIXEL"), RGB(215, 218, 235), 15, FW_SEMIBOLD);
    Txt(dc, width - 82, 12, VERSION, RGB(155, 160, 190), 11);
    Txt(dc, channelX, 27, _T("CH"), label, 9, FW_SEMIBOLD);
    Txt(dc, presetX, 27, _T("PROGRAM"), label, 9, FW_SEMIBOLD);
    for (int i = 0; i < MIDI_CHANNELS; ++i) {
        int y = rowTop + (MIDI_CHANNELS - 1 - i) * rowH;
        TCHAR channelText[8];
        _stprintf_s(channelText, _T("%02d"), i + 1);
        Txt(dc, channelX, y - 5, channelText, label, 10);
        DrawLine(dc, timelineX, y + 2, timelineR, line);
        // Octave C markers sit directly on each channel guide line.
        for (int note = g_firstVisibleNote; note <= g_lastVisibleNote; ++note) {
            if ((note % 12) != 0) continue;
            const int markerX = NotePixelX(note, width);
            const int half = g_lineThickness / 2;
            RECT marker = { markerX, y + 2 - half, markerX + 4, y + 2 - half + g_lineThickness };
            FillRectColor(dc, marker, note == 60 ? g_linesC3Color : g_linesCColor);
        }
    }
    int infoY = rowTop + MIDI_CHANNELS * rowH + 8;
    Txt(dc, channelX, infoY, _T("MIDI FILE"), label, 10);
    Txt(dc, timelineX, infoY, g_fileName[0] ? g_fileName : _T("No MIDI file loaded"), RGB(235, 235, 242), 14);
    TCHAR info[128];
    if (g_loaded) {
        _stprintf_s(info, _T("%u tracks   /   %u ticks per quarter"), (unsigned int)g_tracks.size(), (unsigned int)g_division);
        Txt(dc, timelineX, infoY + 22, info, label, 10);
        // Tempo and time signature are drawn dynamically in Paint, so mid-song changes remain visible.
    }
    int barY = height - 75;
    FillRectColor(dc, RECT{ 0, barY, width, barY + g_progressHeight }, g_uiTransportTop);
    FillRectColor(dc, RECT{ 0, barY + g_progressHeight, width, height }, g_uiTransportBottom);
    FillRectColor(dc, RECT{ 0, barY, width, barY + 2 }, g_uiTransportEdge);
    int controlsY = barY + g_progressHeight + 9;
    Txt(dc, 24, controlsY, _T("STOP"), RGB(235, 235, 242), 12, FW_SEMIBOLD);
    Txt(dc, 92, controlsY, _T("PLAY"), RGB(235, 235, 242), 12, FW_SEMIBOLD);
    Txt(dc, width / 2 - 24, controlsY, _T("LOAD"), RGB(235, 235, 242), 13, FW_SEMIBOLD);
    Txt(dc, width / 2 + 45, controlsY, _T("Options"), label, 13);
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
    else FillRectColor(dc, ps.rcPaint, g_uiBackground);

    const int timelineX = 225, timelineR = cr.right - 28, rowTop = 42, rowH = 18;
    int infoY = rowTop + MIDI_CHANNELS * rowH + 8;
    if (g_loaded) {
        TCHAR metaText[128];
        double bpm = g_currentTempo ? 60000000.0 / (double)g_currentTempo : 120.0;
        _stprintf_s(metaText, _T("TEMPO  %.1f BPM"), bpm);
        Txt(dc, timelineX, infoY + 39, metaText, RGB(205, 210, 225), 10);
        unsigned int denominator = 1u << g_currentTimeDenominatorPower;
        _stprintf_s(metaText, _T("TIME SIGNATURE  %u/%u"), (unsigned int)g_currentTimeNumerator, denominator);
        Txt(dc, timelineX + 150, infoY + 39, metaText, RGB(205, 210, 225), 10);
    }
    for (int ch = 0; ch < MIDI_CHANNELS; ++ch) {
        TCHAR presetText[8];
        _stprintf_s(presetText, _T("%03d"), g_program[ch] + 1);
        int programY = rowTop + (MIDI_CHANNELS - 1 - ch) * rowH;
        Txt(dc, 72, programY - 5, g_loaded ? presetText : _T("---"), RGB(160, 170, 195), 10);
        if (g_loaded && g_programNames[g_program[ch]][0]) {
            RECT nameRect = { 106, programY - 7, timelineX - 7, programY + 9 };
            TxtClipped(dc, nameRect, g_programNames[g_program[ch]], RGB(205, 210, 225), 9);
        }
        if (!g_loaded) continue;
        for (int note = 0; note < MIDI_NOTES; ++note) {
            if (!g_active[ch][note]) continue;
            RECT nr = NoteRect(ch, note, cr.right);
            if (nr.right < ps.rcPaint.left || nr.left > ps.rcPaint.right ||
                nr.bottom < ps.rcPaint.top || nr.top > ps.rcPaint.bottom) continue;
            FillRectColor(dc, nr, ColorForNote(note, g_velocity[ch][note]));
        }
    }
    int barY = cr.bottom - 75;
    FillRectColor(dc, RECT{ 4, barY, 4 + (g_totalUsec ? (int)((g_elapsedUsec * (cr.right - 8)) / g_totalUsec) : 0), barY + g_progressHeight }, g_uiProgress);
    Txt(dc, cr.right - 115, barY + (g_progressHeight > 15 ? 5 : 1), g_playing ? _T("PLAYING") : (g_finished ? _T("FINISHED") : _T("STOPPED")), RGB(235, 235, 242), 11, FW_SEMIBOLD);
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
        if (y >= barY + g_progressHeight) {
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

    LoadColorPalette();
    LoadInterfaceColors();
    LoadProgramNames();
    int x = GetPrivateProfileInt(_T("Window"), _T("X"), 100, g_iniPath);
    int y = GetPrivateProfileInt(_T("Window"), _T("Y"), 100, g_iniPath);
    int w = GetPrivateProfileInt(_T("Window"), _T("Width"), WIN_W, g_iniPath);
    int h = GetPrivateProfileInt(_T("Window"), _T("Height"), WIN_H, g_iniPath);
    if (w < 500 || w > 2000) w = WIN_W; if (h < 350 || h > 1400) h = WIN_H;
    const TCHAR CLASS_NAME[] = _T("MIDI_PIXEL_FIRST_WINDOW");
    WNDCLASS wc = {}; wc.lpfnWndProc = WindowProc; wc.hInstance = instance; wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    if (!RegisterClass(&wc)) return 1;
    TCHAR title[128]; _stprintf_s(title, _T("MIDI_PIXEL %s"), VERSION);
    HWND hwnd = CreateWindowEx(0, CLASS_NAME, title, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        x, y, w, h, NULL, NULL, instance, NULL);
    if (!hwnd) return 2;
    ShowWindow(hwnd, show); UpdateWindow(hwnd); SetTimer(hwnd, 1, 10, NULL);
    MSG msg; while (GetMessage(&msg, NULL, 0, 0) > 0) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return (int)msg.wParam;
}
