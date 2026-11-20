#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <commdlg.h>
#include <shellapi.h>
#include <tchar.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

static const TCHAR VERSION[] = _T("v0.06");
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
struct MidiTextInfo { std::string label, text; };

static std::vector<MidiEvent> g_events;
static std::vector<TrackInfo> g_tracks;
static std::vector<MidiTextInfo> g_midiText;
static HWND g_infoWnd = NULL;
static int g_infoScroll = 0;
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
static unsigned long long g_lastDisplayedSecond = ~0ULL;
static size_t g_eventIndex = 0;
static unsigned long long g_maxTick = 0;
static unsigned short g_division = 480;
static unsigned int g_initialTempo = 500000;
static unsigned int g_currentTempo = 500000;
static unsigned char g_initialTimeNumerator = 4, g_initialTimeDenominatorPower = 2;
static unsigned char g_currentTimeNumerator = 4, g_currentTimeDenominatorPower = 2;
static bool g_loaded = false, g_playing = false, g_finished = false;
static HMIDIOUT g_midiOut = NULL;
static void DestroyBackBuffer();
static HDC g_backDC = NULL;
static HBITMAP g_backBitmap = NULL;
static HBITMAP g_oldBackBitmap = NULL;
static int g_backWidth = 0, g_backHeight = 0;
static UINT g_keyLeft=VK_LEFT, g_keyRight=VK_RIGHT, g_keyPlayPause=VK_SPACE;
static UINT g_keyVolumeUp=VK_UP, g_keyVolumeDown=VK_DOWN;
static int g_seekSeconds=5;
static int g_footerAppSize=12,g_footerAuthorSize=9,g_footerVersionSize=10;
static COLORREF g_footerAppColor=RGB(30,30,45),g_footerAuthorColor=RGB(30,30,45),g_footerVersionColor=RGB(30,30,45);
static bool g_fullscreen=false;
static RECT g_windowedRect={};
static LONG_PTR g_windowedStyle=0;


static UINT ParseVirtualKey(const TCHAR* value, UINT fallback) {
    if(!value || !value[0]) return fallback;
    struct KeyName { const TCHAR* name; UINT code; };
    static const KeyName names[] = {
        {_T("LEFT"),VK_LEFT},{_T("RIGHT"),VK_RIGHT},{_T("UP"),VK_UP},{_T("DOWN"),VK_DOWN},
        {_T("SPACE"),VK_SPACE},{_T("HOME"),VK_HOME},{_T("END"),VK_END},
        {_T("PAGEUP"),VK_PRIOR},{_T("PAGEDOWN"),VK_NEXT},{_T("TAB"),VK_TAB},
        {_T("ENTER"),VK_RETURN},{_T("INSERT"),VK_INSERT},{_T("DELETE"),VK_DELETE},
        {_T("BACKSPACE"),VK_BACK},{_T("F1"),VK_F1},{_T("F2"),VK_F2},{_T("F3"),VK_F3},
        {_T("F4"),VK_F4},{_T("F5"),VK_F5},{_T("F6"),VK_F6},{_T("F7"),VK_F7},
        {_T("F8"),VK_F8},{_T("F9"),VK_F9},{_T("F10"),VK_F10},{_T("F11"),VK_F11},{_T("F12"),VK_F12}
    };
    for(size_t i=0;i<sizeof(names)/sizeof(names[0]);++i)
        if(_tcsicmp(value,names[i].name)==0)return names[i].code;
    if(value[0] && !value[1]) {
        TCHAR c=value[0];
        if(c>=_T('a')&&c<=_T('z'))c-=(_T('a')-_T('A'));
        if((c>=_T('A')&&c<=_T('Z'))||(c>=_T('0')&&c<=_T('9')))return (UINT)c;
    }
    if(!_tcsnicmp(value,_T("0x"),2)) {
        unsigned long n=_tcstoul(value+2,NULL,16);
        if(n>0&&n<=255)return (UINT)n;
    }
    return fallback;
}
static void LoadKeyboardSettings() {
    TCHAR value[32]={};
    GetPrivateProfileString(_T("Keyboard"),_T("Left"),_T("LEFT"),value,32,g_iniPath); g_keyLeft=ParseVirtualKey(value,VK_LEFT);
    GetPrivateProfileString(_T("Keyboard"),_T("Right"),_T("RIGHT"),value,32,g_iniPath); g_keyRight=ParseVirtualKey(value,VK_RIGHT);
    GetPrivateProfileString(_T("Keyboard"),_T("PlayPause"),_T("SPACE"),value,32,g_iniPath); g_keyPlayPause=ParseVirtualKey(value,VK_SPACE);
    GetPrivateProfileString(_T("Keyboard"),_T("VolumeUp"),_T("UP"),value,32,g_iniPath); g_keyVolumeUp=ParseVirtualKey(value,VK_UP);
    GetPrivateProfileString(_T("Keyboard"),_T("VolumeDown"),_T("DOWN"),value,32,g_iniPath); g_keyVolumeDown=ParseVirtualKey(value,VK_DOWN);
    g_seekSeconds=GetPrivateProfileInt(_T("Keyboard"),_T("SeekSeconds"),5,g_iniPath);
    if(g_seekSeconds<1)g_seekSeconds=1; if(g_seekSeconds>600)g_seekSeconds=600;
}
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
    g_footerAppSize=GetPrivateProfileInt(_T("Footer"),_T("AppNameFontSize"),12,g_iniPath);
    g_footerAuthorSize=GetPrivateProfileInt(_T("Footer"),_T("AuthorFontSize"),9,g_iniPath);
    g_footerVersionSize=GetPrivateProfileInt(_T("Footer"),_T("VersionFontSize"),10,g_iniPath);
    if(g_footerAppSize<6)g_footerAppSize=6;if(g_footerAppSize>30)g_footerAppSize=30;
    if(g_footerAuthorSize<6)g_footerAuthorSize=6;if(g_footerAuthorSize>30)g_footerAuthorSize=30;
    if(g_footerVersionSize<6)g_footerVersionSize=6;if(g_footerVersionSize>30)g_footerVersionSize=30;
    g_footerAppColor=ReadIniRgb(_T("Footer"),_T("AppNameColor"),RGB(30,30,45));
    g_footerAuthorColor=ReadIniRgb(_T("Footer"),_T("AuthorColor"),RGB(30,30,45));
    g_footerVersionColor=ReadIniRgb(_T("Footer"),_T("VersionColor"),RGB(30,30,45));
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

static std::string MidiTextToAnsi(const unsigned char* data,unsigned int length) {
    std::string raw((const char*)data,length);
    if(raw.empty())return raw;
    int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,raw.c_str(),(int)raw.size(),NULL,0);
    if(count>0) {
        std::vector<WCHAR> wide((size_t)count+1,0);
        MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,raw.c_str(),(int)raw.size(),wide.data(),count);
        int bytes=WideCharToMultiByte(CP_ACP,0,wide.data(),count,NULL,0,NULL,NULL);
        if(bytes>0) { std::string converted((size_t)bytes,'\\0'); WideCharToMultiByte(CP_ACP,0,wide.data(),count,&converted[0],bytes,NULL,NULL); raw.swap(converted); }
    }
    for(size_t i=0;i<raw.size();++i)if((unsigned char)raw[i]<32)raw[i]=' ';
    return raw;
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

static void SendMidiShort(unsigned char status, unsigned char data1, unsigned char data2) {
    if (!g_midiOut) return;
    DWORD message = (DWORD)status | ((DWORD)(data1 & 0x7f) << 8) | ((DWORD)(data2 & 0x7f) << 16);
    midiOutShortMsg(g_midiOut, message);
}
static void AllNotesOff() {
    if (!g_midiOut) return;
    for (int ch = 0; ch < MIDI_CHANNELS; ++ch) {
        SendMidiShort((unsigned char)(0xB0 | ch), 64, 0);
        SendMidiShort((unsigned char)(0xB0 | ch), 123, 0);
    }
}
static bool OpenMidiOutput(HWND owner) {
    const TCHAR target[] = _T("Microsoft GS Wavetable Synth");
    UINT count = midiOutGetNumDevs();
    for (UINT i = 0; i < count; ++i) {
        MIDIOUTCAPS caps = {};
        if (midiOutGetDevCaps(i, &caps, sizeof(caps)) != MMSYSERR_NOERROR) continue;
        if (_tcsnicmp(caps.szPname, target, _tcslen(target)) != 0) continue;
        MMRESULT result = midiOutOpen(&g_midiOut, i, 0, 0, CALLBACK_NULL);
        if (result == MMSYSERR_NOERROR) {
            AllNotesOff();
            return true;
        }
        g_midiOut = NULL;
        break;
    }
    MessageBox(owner, _T("Microsoft GS Wavetable Synth was not found or could not be opened.\nMIDI PIXEL will remain available in visual-only mode."), _T("MIDI PIXEL - MIDI output"), MB_ICONWARNING | MB_OK);
    return false;
}
static void CloseMidiOutput() {
    if (!g_midiOut) return;
    AllNotesOff();
    midiOutClose(g_midiOut);
    g_midiOut = NULL;
}
static void ResumeActiveNotes() {
    if (!g_midiOut) return;
    for (int ch = 0; ch < MIDI_CHANNELS; ++ch) {
        for (int note = 0; note < MIDI_NOTES; ++note) {
            if (g_active[ch][note] && g_velocity[ch][note] > 0)
                SendMidiShort((unsigned char)(0x90 | ch), (unsigned char)note, (unsigned char)g_velocity[ch][note]);
        }
    }
}
static void ClearPlayback() {
    memset(g_active, 0, sizeof(g_active));
    memset(g_velocity, 0, sizeof(g_velocity));
    memcpy(g_program, g_initialProgram, sizeof(g_program));
    g_elapsedUsec = 0;
    g_lastDisplayedSecond = ~0ULL;
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
    std::vector<MidiTextInfo> midiText;
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
                if (metaType == 0x03 && len > 0) track.name = MidiTextToAnsi(payload,len);
                if(len>0) {
                    const char* label=NULL;
                    switch(metaType) {
                    case 0x01: label="Text"; break;
                    case 0x02: label="Copyright"; break;
                    case 0x04: label="Instrument"; break;
                    case 0x05: label="Lyric"; break;
                    case 0x06: label="Marker"; break;
                    case 0x07: label="Cue"; break;
                    }
                    if(label)midiText.push_back({label,MidiTextToAnsi(payload,len)});
                }
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
    g_midiText.swap(midiText);
    g_infoScroll=0;
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
    const int timelineX = 225, timelineR = clientWidth - 28, rowTop = 34, rowH = 18;
    if (note < g_firstVisibleNote || note > g_lastVisibleNote) return RECT{ -100, -100, -96, -94 };
    int x = NotePixelX(note, clientWidth);
    int y = rowTop + (MIDI_CHANNELS - 1 - channel) * rowH - 6;
    RECT r = { x, y, x + 4, y + 6 };
    return r;
}
static int ProgressWidth(int clientWidth) {
    if (!g_totalUsec || clientWidth <= 0) return 0;
    unsigned long long width=(g_elapsedUsec*(unsigned long long)clientWidth)/g_totalUsec;
    if(width>(unsigned long long)clientWidth)width=clientWidth;
    return (int)width;
}
static void InvalidateInfoBlock(HWND hwnd) {
    RECT client; GetClientRect(hwnd,&client);
    RECT info={20,300,560,client.bottom-77};
    if(info.bottom<info.top)info.bottom=info.top;
    InvalidateRect(hwnd,&info,FALSE);
}
static void InvalidateDynamic(HWND hwnd) {
    RECT r;
    GetClientRect(hwnd, &r);
    RECT notes = { 220, 32, r.right - 20, 34 + MIDI_CHANNELS * 18 };
    InvalidateRect(hwnd, &notes, FALSE);
    RECT presets = { 68, 32, 222, 34 + MIDI_CHANNELS * 18 };
    InvalidateRect(hwnd, &presets, FALSE);
    InvalidateInfoBlock(hwnd);
    RECT progress = { 0, r.bottom - 77, r.right, r.bottom - 40 };
    InvalidateRect(hwnd, &progress, FALSE);
}
static void AdvancePlayback(HWND hwnd) {
    if (!g_playing) return;
    RECT client; GetClientRect(hwnd, &client);
    int oldProgressWidth = ProgressWidth(client.right);
    bool wasFinished = g_finished;
    unsigned long long now = GetTickCount64();
    if (g_lastMs) g_elapsedUsec += (now - g_lastMs) * 1000;
    g_lastMs = now;
    if (g_elapsedUsec > g_totalUsec) g_elapsedUsec = g_totalUsec;
    bool dynamicChanged = false;
    unsigned long long displayedSecond = g_elapsedUsec / 1000000ULL;
    if (displayedSecond != g_lastDisplayedSecond) {
        RECT progressText = { 0, client.bottom - 77, client.right, client.bottom - 77 + g_progressHeight };
        InvalidateRect(hwnd, &progressText, FALSE);
        g_lastDisplayedSecond = displayedSecond;
    }
    while (g_eventIndex < g_events.size() && g_events[g_eventIndex].usec <= g_elapsedUsec) {
        const MidiEvent& e = g_events[g_eventIndex++];
        if (e.type == 3) {
            g_currentTempo = (unsigned int)e.a;
            InvalidateInfoBlock(hwnd);
        } else if (e.type == 4) {
            g_currentTimeNumerator = (unsigned char)e.a;
            g_currentTimeDenominatorPower = (unsigned char)e.b;
            InvalidateInfoBlock(hwnd);
        } else if (e.type == 2) {
            SendMidiShort((unsigned char)(0xC0 | e.channel), (unsigned char)e.a, 0);
            if (g_program[e.channel] != e.a) {
                g_program[e.channel] = e.a;
                RECT pr = { 68, 34 + (MIDI_CHANNELS - 1 - e.channel) * 18 - 6, 222, 34 + (MIDI_CHANNELS - 1 - e.channel) * 18 + 12 };
                InvalidateRect(hwnd, &pr, FALSE);
            }
        } else {
            RECT oldRect = NoteRect(e.channel, e.a, client.right);
            if (e.type == 1) {
                SendMidiShort((unsigned char)(0x90 | e.channel), (unsigned char)e.a, (unsigned char)e.b);
                g_active[e.channel][e.a] = 1;
                g_velocity[e.channel][e.a] = e.b;
            } else {
                SendMidiShort((unsigned char)(0x80 | e.channel), (unsigned char)e.a, (unsigned char)e.b);
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
        AllNotesOff();
    }
    int newProgressWidth = ProgressWidth(client.right);
    if (newProgressWidth > oldProgressWidth) {
        RECT progressStep = { oldProgressWidth, client.bottom - 73,
                              newProgressWidth, client.bottom - 77 + g_progressHeight };
        InvalidateRect(hwnd, &progressStep, FALSE);
    }
    if (wasFinished != g_finished) {
        RECT status = { client.right - 120, client.bottom - 77,
                        client.right, client.bottom - 77 + g_progressHeight };
        InvalidateRect(hwnd, &status, FALSE);
    }
    (void)dynamicChanged;
}
static void StartPlayback() {
    if (!g_loaded) return;
    if (g_finished || g_elapsedUsec >= g_totalUsec) ClearPlayback();
    g_playing = true;
    ResumeActiveNotes();
    g_lastMs = GetTickCount64();}
static void SeekPlaybackToUsec(HWND hwnd, unsigned long long target) {
    if (!g_loaded || g_totalUsec == 0) return;
    if(target>g_totalUsec)target=g_totalUsec;
    bool resume = g_playing;
    AllNotesOff();
    ClearPlayback();
    while (g_eventIndex < g_events.size() && g_events[g_eventIndex].usec <= target) {
        const MidiEvent& e = g_events[g_eventIndex++];
        if (e.type == 1) { g_active[e.channel][e.a] = 1; g_velocity[e.channel][e.a] = e.b; }
        else if (e.type == 0) { g_active[e.channel][e.a] = 0; g_velocity[e.channel][e.a] = 0; }
        else if (e.type == 2) g_program[e.channel] = e.a;
        else if (e.type == 3) g_currentTempo = (unsigned int)e.a;
        else if (e.type == 4) { g_currentTimeNumerator = (unsigned char)e.a; g_currentTimeDenominatorPower = (unsigned char)e.b; }
    }
    g_elapsedUsec = target;
    g_finished = (target >= g_totalUsec);
    for (int ch = 0; ch < MIDI_CHANNELS; ++ch)
        SendMidiShort((unsigned char)(0xC0 | ch), (unsigned char)g_program[ch], 0);
    if (resume) ResumeActiveNotes();
    g_playing = resume && !g_finished;
    g_lastMs = g_playing ? GetTickCount64() : 0;
    InvalidateDynamic(hwnd);
}
static void SeekPlayback(HWND hwnd,int x) {
    RECT client; GetClientRect(hwnd,&client);
    const int left=0,right=client.right;
    if(right<=left)return;
    if(x<left)x=left; if(x>right)x=right;
    unsigned long long target=(unsigned long long)((double)(x-left)/(double)(right-left)*(double)g_totalUsec);
    SeekPlaybackToUsec(hwnd,target);
}
static void SeekRelative(HWND hwnd,int direction) {
    if(!g_loaded||!g_totalUsec)return;
    unsigned long long delta=(unsigned long long)g_seekSeconds*1000000ULL;
    unsigned long long target=g_elapsedUsec;
    if(direction<0)target=target>delta?target-delta:0;
    else target=(g_totalUsec-target>delta)?target+delta:g_totalUsec;
    SeekPlaybackToUsec(hwnd,target);
}
static void AdjustMidiVolume(int direction) {
    if(!g_midiOut)return;
    DWORD volume=0;
    if(midiOutGetVolume(g_midiOut,&volume)!=MMSYSERR_NOERROR)return;
    const DWORD step=65535UL*5UL/100UL;
    DWORD left=volume&0xffff,right=(volume>>16)&0xffff;
    if(direction>0) {
        left=left+step>65535?65535:left+step;
        right=right+step>65535?65535:right+step;
    } else {
        left=left>step?left-step:0;
        right=right>step?right-step:0;
    }
    midiOutSetVolume(g_midiOut,(right<<16)|left);
}
static void StopPlayback() {
    g_playing = false;
    AllNotesOff();
    ClearPlayback();
}
static void OpenMidiPath(HWND hwnd,const TCHAR* path,bool showError) {
    if(!path||!path[0])return;
    AllNotesOff(); g_playing=false;
    if(LoadMidiFile(path)) {
        lstrcpyn(g_fileName,path,MAX_PATH);
        TCHAR* base1=_tcsrchr(g_fileName,_T('\\'));
        TCHAR* base2=_tcsrchr(g_fileName,_T('/'));
        TCHAR* base=base1;
        if(base2&&(!base||base2>base))base=base2;
        if(base)lstrcpyn(g_fileName,base+1,MAX_PATH);
        DestroyBackBuffer(); InvalidateRect(hwnd,NULL,FALSE); InvalidateDynamic(hwnd);
        if(g_infoWnd)InvalidateRect(g_infoWnd,NULL,FALSE);
        StartPlayback();
    } else if(showError) {
        MessageBox(hwnd,_T("Fichier MIDI non reconnu ou format non pris en charge."),_T("MIDI PIXEL"),MB_ICONWARNING|MB_OK);
    }
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
    if(GetOpenFileName(&ofn))OpenMidiPath(hwnd,path,true);
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
    const int rowTop = 34, rowH = 18;
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
    int headerY = rowTop + MIDI_CHANNELS * rowH + 4;
    Txt(dc, channelX, headerY, _T("CH"), label, 9, FW_SEMIBOLD);
    Txt(dc, presetX, headerY, _T("PROGRAM"), label, 9, FW_SEMIBOLD);
    int infoY = headerY + 12;
    TCHAR info[128];
    if (g_loaded) {
        TCHAR displayTitle[MAX_PATH]; lstrcpyn(displayTitle,g_fileName,MAX_PATH);
        TCHAR* extension=_tcsrchr(displayTitle,_T('.')); if(extension)*extension=0;
        Txt(dc,timelineX,infoY+2,displayTitle,RGB(235,235,242),17);
        _stprintf_s(info, _T("%u tracks   /   %u ticks per quarter"), (unsigned int)g_tracks.size(), (unsigned int)g_division);
        int barY = height - 77;
        Txt(dc, 24, barY - 55, info, label, 10);
    }
    int barY = height - 77;
    FillRectColor(dc, RECT{ 0, barY, width, barY + g_progressHeight }, g_uiTransportTop);
    FillRectColor(dc, RECT{ 0, barY + g_progressHeight, width, height }, g_uiTransportBottom);
    FillRectColor(dc, RECT{ 0, barY, width, barY + 2 }, g_uiTransportEdge);
    int controlsY = barY + g_progressHeight + 9;
    Txt(dc, 40, controlsY, _T("SETTINGS"), RGB(235, 235, 242), 12);
    Txt(dc, width / 2 - 24, controlsY, _T("LOAD"), RGB(235, 235, 242), 13, FW_SEMIBOLD);
    Txt(dc, width / 2 + 45, controlsY, _T("INFO"), label, 13, FW_SEMIBOLD);
    Txt(dc, width - 115, controlsY, _T("STOP"), RGB(235, 235, 242), 12, FW_SEMIBOLD);
    Txt(dc, width - 55, controlsY, _T("PLAY"), RGB(235, 235, 242), 12, FW_SEMIBOLD);
    Txt(dc, width / 2 - 48, height - 17, _T("MIDI PIXEL"), g_footerAppColor, g_footerAppSize);
    Txt(dc, width / 2 + 50, height - 17, VERSION, g_footerVersionColor, g_footerVersionSize);
    Txt(dc, width - 95, height - 16, _T("cslevine+IA"), g_footerAuthorColor, g_footerAuthorSize);
    Txt(dc,width-24,5,_T("X"),RGB(235,235,242),14,FW_BOLD);
}
static void ToggleFullscreen(HWND hwnd) {
    if(!g_fullscreen) {
        if(!GetWindowRect(hwnd,&g_windowedRect))return;
        g_windowedStyle=GetWindowLongPtr(hwnd,GWL_STYLE);
        MONITORINFO mi={};mi.cbSize=sizeof(mi);
        HMONITOR mon=MonitorFromWindow(hwnd,MONITOR_DEFAULTTONEAREST);
        if(!GetMonitorInfo(mon,&mi))return;
        SetWindowLongPtr(hwnd,GWL_STYLE,WS_POPUP);
        SetWindowPos(hwnd,HWND_TOP,mi.rcMonitor.left,mi.rcMonitor.top,
            mi.rcMonitor.right-mi.rcMonitor.left,mi.rcMonitor.bottom-mi.rcMonitor.top,
            SWP_FRAMECHANGED|SWP_NOOWNERZORDER);
        g_fullscreen=true;
    } else {
        SetWindowLongPtr(hwnd,GWL_STYLE,g_windowedStyle);
        SetWindowPos(hwnd,NULL,g_windowedRect.left,g_windowedRect.top,
            g_windowedRect.right-g_windowedRect.left,g_windowedRect.bottom-g_windowedRect.top,
            SWP_FRAMECHANGED|SWP_NOZORDER|SWP_NOOWNERZORDER);
        g_fullscreen=false;
    }
    DestroyBackBuffer();
    InvalidateRect(hwnd,NULL,FALSE);
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

    const int timelineX = 225, timelineR = cr.right - 28, rowTop = 34, rowH = 18;
    int headerY = rowTop + MIDI_CHANNELS * rowH + 4;
    int infoY = headerY + 12;
    if (g_loaded) {
        TCHAR metaText[128];
        double bpm = g_currentTempo ? 60000000.0 / (double)g_currentTempo : 120.0;
        _stprintf_s(metaText, _T("TEMPO  %.1f BPM"), bpm);
        int barY = cr.bottom - 77;
        Txt(dc, 24, barY - 41, metaText, RGB(205, 210, 225), 10);
        unsigned int denominator = 1u << g_currentTimeDenominatorPower;
        _stprintf_s(metaText, _T("TIME SIGNATURE  %u/%u"), (unsigned int)g_currentTimeNumerator, denominator);
        Txt(dc, 24, barY - 27, metaText, RGB(205, 210, 225), 10);
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
    int barY = cr.bottom - 77;
    RECT progressFill = { 0, barY + 2, ProgressWidth(cr.right), barY + g_progressHeight - 2 };
    RECT visibleFill;
    if (IntersectRect(&visibleFill, &progressFill, &ps.rcPaint))
        FillRectColor(dc, visibleFill, g_uiProgress);
    TCHAR timeText[64];
    unsigned long long elapsedSec=g_elapsedUsec/1000000ULL,totalSec=g_totalUsec/1000000ULL;
    _stprintf_s(timeText,_T("%02llu:%02llu  /  %02llu:%02llu"),elapsedSec/60,elapsedSec%60,totalSec/60,totalSec%60);
    SIZE timeSize={}; GetTextExtentPoint32(dc,timeText,lstrlen(timeText),&timeSize);
    int progressFontSize = g_progressHeight - 2;
    if(progressFontSize < 8) progressFontSize = 8;
    if(progressFontSize > 26) progressFontSize = 26;
    int progressTextY = barY + (g_progressHeight - progressFontSize) / 2;
    Txt(dc,cr.right-timeSize.cx-14,progressTextY,timeText,RGB(235,235,242),progressFontSize);
    Txt(dc,20,progressTextY,g_fileName,RGB(235,235,242),progressFontSize);
    EndPaint(hwnd, &ps);
}
static LRESULT CALLBACK InfoWindowProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) {
    switch(msg) {
    case WM_ERASEBKGND: return 1;
    case WM_MOUSEWHEEL: {
        RECT client; GetClientRect(hwnd,&client);
        int maxScroll=(int)(g_midiText.size()*19+g_tracks.size()*18+100)-(client.bottom-45);
        if(maxScroll<0)maxScroll=0;
        g_infoScroll-=GET_WHEEL_DELTA_WPARAM(wp)/WHEEL_DELTA*57;
        if(g_infoScroll<0)g_infoScroll=0;if(g_infoScroll>maxScroll)g_infoScroll=maxScroll;
        InvalidateRect(hwnd,NULL,FALSE); return 0;
    }
    case WM_LBUTTONDOWN: {
        RECT r; GetClientRect(hwnd,&r);
        if(LOWORD(lp)>=r.right-38 && HIWORD(lp)<30){ShowWindow(hwnd,SW_HIDE);return 0;}
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc=BeginPaint(hwnd,&ps); RECT r; GetClientRect(hwnd,&r);
        FillRectColor(dc,r,g_uiBackground);
        Txt(dc,14,10,_T("MIDI FILE INFORMATION"),RGB(235,235,242),12,FW_SEMIBOLD);
        Txt(dc,r.right-25,8,_T("X"),RGB(235,235,242),14,FW_BOLD);
        HPEN pen=CreatePen(PS_SOLID,1,g_uiLines); HPEN old=(HPEN)SelectObject(dc,pen);
        MoveToEx(dc,10,31,NULL); LineTo(dc,r.right-10,31); SelectObject(dc,old); DeleteObject(pen);
        int y=43-g_infoScroll;
        Txt(dc,14,y,_T("FILE"),RGB(160,170,195),9,FW_BOLD); y+=18;
        TCHAR file[512]={}; 
#ifdef UNICODE
        MultiByteToWideChar(CP_ACP,0,g_fileName,-1,file,512);
#else
        lstrcpynA(file,g_fileName,512);
#endif
        TxtClipped(dc,RECT{14,y,r.right-14,y+18},file,RGB(235,235,242),11); y+=28;
        for(size_t i=0;i<g_midiText.size();++i) {
            std::string line=g_midiText[i].label+": "+g_midiText[i].text;
            TCHAR buf[512]={};
#ifdef UNICODE
            MultiByteToWideChar(CP_ACP,0,line.c_str(),-1,buf,512);
#else
            lstrcpynA(buf,line.c_str(),512);
#endif
            TxtClipped(dc,RECT{14,y,r.right-14,y+18},buf,RGB(225,225,232),10); y+=19;
        }
        y+=10; Txt(dc,14,y,_T("TRACKS"),RGB(160,170,195),9,FW_BOLD); y+=19;
        for(size_t i=0;i<g_tracks.size();++i) {
            char line[512]; _snprintf_s(line,sizeof(line),_TRUNCATE,"Track %u: %s",(unsigned int)(i+1),g_tracks[i].name.empty()?"(unnamed)":g_tracks[i].name.c_str());
            TCHAR buf[512]={};
#ifdef UNICODE
            MultiByteToWideChar(CP_ACP,0,line,-1,buf,512);
#else
            lstrcpynA(buf,line,512);
#endif
            TxtClipped(dc,RECT{14,y,r.right-14,y+17},buf,RGB(205,210,225),10); y+=18;
        }
        EndPaint(hwnd,&ps); return 0;
    }
    case WM_CLOSE: ShowWindow(hwnd,SW_HIDE); return 0;
    }
    return DefWindowProc(hwnd,msg,wp,lp);
}
static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_PAINT: Paint(hwnd); return 0;
    case WM_SIZE:
        DestroyBackBuffer(); InvalidateRect(hwnd,NULL,FALSE);
        if(g_infoWnd) {
            if(IsIconic(hwnd))ShowWindow(g_infoWnd,SW_HIDE);
            else {RECT wr;GetWindowRect(hwnd,&wr);int ix=wr.right+1;if(ix+350>GetSystemMetrics(SM_CXSCREEN))ix=wr.left-351;SetWindowPos(g_infoWnd,NULL,ix,wr.top,350,wr.bottom-wr.top,SWP_NOZORDER|SWP_NOACTIVATE);}
        }
        return 0;
    case WM_MOVE:
        if(g_infoWnd && IsWindowVisible(g_infoWnd)){RECT wr;GetWindowRect(hwnd,&wr);int ix=wr.right+1;if(ix+350>GetSystemMetrics(SM_CXSCREEN))ix=wr.left-351;SetWindowPos(g_infoWnd,NULL,ix,wr.top,350,wr.bottom-wr.top,SWP_NOZORDER|SWP_NOACTIVATE);}
        return 0;
    case WM_TIMER: AdvancePlayback(hwnd); return 0;
    case WM_DROPFILES: {
        HDROP drop=(HDROP)wp; TCHAR path[MAX_PATH]={};
        if(DragQueryFile(drop,0,path,MAX_PATH))OpenMidiPath(hwnd,path,true);
        DragFinish(drop); return 0;
    }
    case WM_LBUTTONDOWN: {
        RECT client; GetClientRect(hwnd, &client);
        int x = LOWORD(lp), y = HIWORD(lp), barY = client.bottom - 77;
        if (g_loaded && y >= barY && y < barY + g_progressHeight) {
            SeekPlayback(hwnd, x);
            return 0;
        }
        if(y<27 && x>=client.right-32){SendMessage(hwnd,WM_CLOSE,0,0);return 0;}
        if (y >= barY + g_progressHeight) {
            if (x >= client.right-85 && x < client.right-30) StartPlayback();
            else if (x >= client.right-150 && x < client.right-90) StopPlayback();
            else if (x >= 25 && x < 125) { /* SETTINGS placeholder */ }
            else if (x >= client.right/2-45 && x < client.right/2+35) LoadDialog(hwnd);
            else if (x >= client.right/2+35 && x < client.right/2+125) {
                if(g_infoWnd && IsWindowVisible(g_infoWnd))ShowWindow(g_infoWnd,SW_HIDE);
                else {
                    if(!g_infoWnd) {
                        RECT wr;GetWindowRect(hwnd,&wr);int ix=wr.right+1;if(ix+350>GetSystemMetrics(SM_CXSCREEN))ix=wr.left-351;
                        g_infoWnd=CreateWindowEx(WS_EX_TOOLWINDOW,_T("MIDI_PIXEL_INFO_WINDOW"),_T(""),WS_POPUP|WS_BORDER,ix,wr.top,350,wr.bottom-wr.top,hwnd,NULL,(HINSTANCE)GetWindowLongPtr(hwnd,GWLP_HINSTANCE),NULL);
                    }
                    if(g_infoWnd){InvalidateRect(g_infoWnd,NULL,TRUE);ShowWindow(g_infoWnd,SW_SHOWNOACTIVATE);UpdateWindow(g_infoWnd);}
                }
            }
            InvalidateDynamic(hwnd);
        }
        return 0;
    }
    case WM_KEYDOWN:
        if(wp==VK_ESCAPE){SendMessage(hwnd,WM_CLOSE,0,0);return 0;}
        if(wp=='F'){ToggleFullscreen(hwnd);return 0;}
        if(wp==g_keyPlayPause) {
            if(g_playing){g_playing=false;g_lastMs=0;AllNotesOff();}
            else StartPlayback();
            InvalidateDynamic(hwnd);return 0;
        }
        if(wp==g_keyLeft){SeekRelative(hwnd,-1);return 0;}
        if(wp==g_keyRight){SeekRelative(hwnd,1);return 0;}
        if(wp==g_keyVolumeUp){AdjustMidiVolume(1);return 0;}
        if(wp==g_keyVolumeDown){AdjustMidiVolume(-1);return 0;}
        break;
    case WM_CLOSE: KillTimer(hwnd, 1); SaveWindow(hwnd); CloseMidiOutput(); DestroyBackBuffer(); DestroyWindow(hwnd); return 0;
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
    LoadKeyboardSettings();
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
    WNDCLASS iwc={}; iwc.lpfnWndProc=InfoWindowProc; iwc.hInstance=instance; iwc.lpszClassName=_T("MIDI_PIXEL_INFO_WINDOW");
    iwc.hCursor=LoadCursor(NULL,IDC_ARROW); iwc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
    if(!RegisterClass(&iwc))return 1;
    TCHAR title[128]; _stprintf_s(title, _T("MIDI PIXEL %s"), VERSION);
    HWND hwnd = CreateWindowEx(0, CLASS_NAME, title, WS_POPUP | WS_THICKFRAME,
        x, y, w, h, NULL, NULL, instance, NULL);
    if (!hwnd) return 2;
    OpenMidiOutput(hwnd);
    DragAcceptFiles(hwnd,TRUE);
    ShowWindow(hwnd, show); UpdateWindow(hwnd); SetTimer(hwnd, 1, 10, NULL);
    int argc=0;
    LPWSTR* argv=CommandLineToArgvW(GetCommandLineW(),&argc);
    if(argv) {
        if(argc>1) {
            TCHAR initialPath[MAX_PATH]={};
#ifdef UNICODE
            lstrcpyn(initialPath,argv[1],MAX_PATH);
#else
            WideCharToMultiByte(CP_ACP,0,argv[1],-1,initialPath,MAX_PATH,NULL,NULL);
#endif
            OpenMidiPath(hwnd,initialPath,true);
        }
        LocalFree(argv);
    }
    MSG msg; while (GetMessage(&msg, NULL, 0, 0) > 0) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return (int)msg.wParam;
}
