// ============================================================
// MIDIPIXELS
// Créé en vibe-coding par Claude-Samuel Lévine fin novembre 2026
// à Étretat - www.cslevine.com
// IA génératrice des sources C++ : ChatGPT ("Lia")
// ============================================================

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <objidl.h>
#define IDR_MIDIPIXELS_TEXT 201
#define IDR_HISTORY_TEXT 202
#define IDR_AUTHORS_TEXT 203
#define IDR_AUTHORS_GIF 204
#include <mmsystem.h>
#include <commdlg.h>
#include <commctrl.h>
#include <windowsx.h>
#include <shellapi.h>
#include <tchar.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

static const TCHAR VERSION[] = _T("v0.501b");
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
struct MidiTextInfo { unsigned long long tick, usec; std::string label, text; };

static std::vector<MidiEvent> g_events;
static std::vector<TrackInfo> g_tracks;
static std::vector<MidiTextInfo> g_midiText;
static HWND g_infoWnd = NULL;
static int g_infoScroll = 0;
static int g_infoWindowWidth=350,g_infoContent=0;
static std::vector<TCHAR> g_infoText;
static Gdiplus::Image* g_authorsGif=NULL;
static IStream* g_authorsGifStream=NULL;
static GUID g_gifFrameDimension={};
static UINT g_gifFrameCount=0,g_gifFrameIndex=0;
static std::vector<UINT> g_gifFrameDelaysMs;
static ULONG_PTR g_gdiplusToken=0;
static unsigned char g_active[MIDI_CHANNELS][MIDI_NOTES] = {};
static std::vector<std::vector<unsigned char>> g_trackActive,g_trackPercussionActive;
static std::vector<std::vector<int>> g_trackVelocity,g_trackPercussionVelocity;
static std::vector<std::vector<int>> g_trackNoteChannel;
static std::vector<int> g_trackExpression,g_trackPan;
static COLORREF g_verticalPanCenterLine=RGB(110,100,75);
static COLORREF g_midiLinearControlsColor=RGB(205,150,55);
static bool g_trackMode=false;
static COLORREF g_percussionColor=RGB(230,110,190);
static int g_velocity[MIDI_CHANNELS][MIDI_NOTES] = {};
static int g_pitchBend[MIDI_CHANNELS] = {}; // MIDI Pitch Bend: -8192..+8191
static bool g_pitchBendDisplay = true;
static int g_pitchBendRange = 20; // Maximum indicator length in pixels
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
static int g_sharpPixelShift = 1;
static int g_notePixelW = 4, g_notePixelH = 6;
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
static TCHAR g_midiOutIniPath[MAX_PATH]={};
static UINT g_selectedMidiDevice=(UINT)-1;
static bool g_useMidiMapper=false;
static HWND g_settingsWnd=NULL,g_outputCombo=NULL,g_mapperCheck=NULL;
static void DestroyBackBuffer();
static void AdjustWindowForDisplay(HWND hwnd);
static HDC g_backDC = NULL;
static HBITMAP g_backBitmap = NULL;
static HBITMAP g_oldBackBitmap = NULL;
static int g_backWidth = 0, g_backHeight = 0;
static UINT g_keyLeft=VK_LEFT, g_keyRight=VK_RIGHT, g_keyPlayPause=VK_SPACE;
static UINT g_keyVolumeUp=VK_UP, g_keyVolumeDown=VK_DOWN;
static UINT g_keyMidiPanic='1';
static bool g_keyMidiPanicShift=true;
static bool g_numpadZeroPaused=false;
static UINT g_keyMiniMode='M',g_keySettings='P',g_keyDisplayMode=VK_TAB;
static int g_seekSeconds=5;
static int g_footerAppSize=12,g_footerAuthorSize=9,g_footerVersionSize=10;
static int g_trackTitleFontSize=17;
static int g_textesFontSize=10;
static int g_instrumentFontSize=9;
static int g_musicdataFontSize=10;
static COLORREF g_footerAppColor=RGB(30,30,45),g_footerAuthorColor=RGB(30,30,45),g_footerVersionColor=RGB(30,30,45);
static bool g_fullscreen=false;
static bool g_miniMode=false;
static RECT g_miniRestoreRect={};
static const int MINI_TRANSPORT_HEIGHT=54;
static const int MINI_SCREEN_X=168,MINI_SCREEN_W=130,MINI_SCREEN_H=34;
static bool g_startFullscreen=false;
static COLORREF g_windowButtonsColor=RGB(49,46,72);
static bool g_windowButtonsHover=false;
static RECT g_windowedRect={};
static LONG_PTR g_windowedStyle=0;


static bool GetIniValueAnySection(const TCHAR* wantedKey,TCHAR* output,DWORD outputSize,const TCHAR* fallback) {
    if(!output||outputSize==0)return false;
    lstrcpyn(output,fallback?fallback:_T(""),(int)outputSize);
    FILE* file=NULL;
    if(_tfopen_s(&file,g_iniPath,_T("rt"))!=0||!file)return false;
    TCHAR line[1024]; bool found=false;
    while(_fgetts(line,(int)(sizeof(line)/sizeof(line[0])),file)) {
        TCHAR* p=line;
        while(*p==_T(' ')||*p==_T('\t'))++p;
        if(!*p||*p==_T(';')||*p==_T('#')||*p==_T('['))continue;
        TCHAR* equal=_tcschr(p,_T('='));
        if(!equal)continue;
        TCHAR* keyEnd=equal;
        while(keyEnd>p&&(keyEnd[-1]==_T(' ')||keyEnd[-1]==_T('\t')))--keyEnd;
        *keyEnd=0;
        if(_tcsicmp(p,wantedKey)!=0)continue;
        TCHAR* value=equal+1;
        while(*value==_T(' ')||*value==_T('\t'))++value;
        TCHAR* end=value+_tcslen(value);
        while(end>value&&(end[-1]==_T('\r')||end[-1]==_T('\n')||end[-1]==_T(' ')||end[-1]==_T('\t')))--end;
        *end=0; lstrcpyn(output,value,(int)outputSize); found=true; break;
    }
    fclose(file); return found;
}
static int GetIniIntAnySection(const TCHAR* key,int fallback) {
    TCHAR value[64]={};
    if(!GetIniValueAnySection(key,value,64,_T(""))||!value[0])return fallback;
    return (int)_tcstol(value,NULL,10);
}
static COLORREF ReadIniRgbAnySection(const TCHAR* key,COLORREF fallback) {
    TCHAR value[64]={};
    if(!GetIniValueAnySection(key,value,64,_T(""))||!value[0])return fallback;
    int r=GetRValue(fallback),g=GetGValue(fallback),b=GetBValue(fallback);
    if(_stscanf_s(value,_T("%d , %d , %d"),&r,&g,&b)!=3)return fallback;
    if(r<0||r>255||g<0||g>255||b<0||b>255)return fallback;
    return RGB(r,g,b);
}

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
static void LoadInstrumentDisplaySettings() {
 TCHAR mode[32]={};GetIniValueAnySection(_T("instrumentsMidiMode"),mode,32,_T("canal"));
 g_trackMode=(_tcsicmp(mode,_T("track"))==0);
 g_percussionColor=ReadIniRgbAnySection(_T("PercussionColor"),RGB(230,110,190));
 g_midiLinearControlsColor=ReadIniRgbAnySection(_T("MidiLinearControlsColor"),RGB(205,150,55));
}
static void SaveInstrumentDisplaySettings() {
 WritePrivateProfileString(_T("Interface"),_T("instrumentsMidiMode"),g_trackMode?_T("track"):_T("canal"),g_iniPath);
}
static void LoadKeyboardSettings() {
    TCHAR value[32]={};
    GetIniValueAnySection(_T("Left"),value,32,_T("LEFT")); g_keyLeft=ParseVirtualKey(value,VK_LEFT);
    GetIniValueAnySection(_T("Right"),value,32,_T("RIGHT")); g_keyRight=ParseVirtualKey(value,VK_RIGHT);
    GetIniValueAnySection(_T("PlayPause"),value,32,_T("SPACE")); g_keyPlayPause=ParseVirtualKey(value,VK_SPACE);
    GetIniValueAnySection(_T("VolumeUp"),value,32,_T("UP")); g_keyVolumeUp=ParseVirtualKey(value,VK_UP);
    GetIniValueAnySection(_T("VolumeDown"),value,32,_T("DOWN")); g_keyVolumeDown=ParseVirtualKey(value,VK_DOWN);
    GetIniValueAnySection(_T("MidiPanic"),value,32,_T("!"));
    g_keyMidiPanicShift=(_tcscmp(value,_T("!"))==0);
    g_keyMidiPanic=g_keyMidiPanicShift?'1':ParseVirtualKey(value,'1');
    g_seekSeconds=GetIniIntAnySection(_T("SeekSeconds"),5);
    if(g_seekSeconds<1)g_seekSeconds=1; if(g_seekSeconds>600)g_seekSeconds=600;
    GetIniValueAnySection(_T("MiniMode"),value,32,_T("M")); g_keyMiniMode=ParseVirtualKey(value,'M');
    GetIniValueAnySection(_T("Settings"),value,32,_T("P")); g_keySettings=ParseVirtualKey(value,'P');
    GetIniValueAnySection(_T("DisplayMode"),value,32,_T("TAB")); g_keyDisplayMode=ParseVirtualKey(value,VK_TAB);
}
static void LoadColorPalette() {
 const TCHAR* keys[12] = {_T("C"),_T("C_SHARP"),_T("D"),_T("D_SHARP"),_T("E"),_T("F"),_T("F_SHARP"),_T("G"),_T("G_SHARP"),_T("A"),_T("A_SHARP"),_T("B")};
 for (int i=0;i<12;++i) {
  TCHAR value[64]={}; GetIniValueAnySection(keys[i],value,64,_T(""));
  int rgb[3]={GetRValue(g_noteColors[i]),GetGValue(g_noteColors[i]),GetBValue(g_noteColors[i])};
  int a=rgb[0],b=rgb[1],c=rgb[2];
  if (value[0] && _stscanf_s(value,_T("%d , %d , %d"),&a,&b,&c)==3) {
   if(a>=0&&a<=255)rgb[0]=a; if(b>=0&&b<=255)rgb[1]=b; if(c>=0&&c<=255)rgb[2]=c;
  }
  g_noteColors[i]=RGB(rgb[0],rgb[1],rgb[2]);
 }
 g_pitchBendDisplay=GetIniIntAnySection(_T("pitchbend"),1)!=0;
 g_pitchBendRange=GetIniIntAnySection(_T("pitchbendrange"),20);
 if(g_pitchBendRange<0)g_pitchBendRange=0;
 if(g_pitchBendRange>200)g_pitchBendRange=200;
 int minimum=GetIniIntAnySection(_T("VelocityMinimumPercent"),10);
 if(minimum<0)minimum=0; if(minimum>100)minimum=100;
 g_velocityMinimum=minimum/100.0;
 TCHAR curve[32]={}; GetIniValueAnySection(_T("VelocityCurve"),curve,32,_T("1.00"));
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


static void LoadInterfaceColors() {
    g_infoWindowWidth=GetIniIntAnySection(_T("InfoWindowWidth"),350);
    if(g_infoWindowWidth<160)g_infoWindowWidth=160;if(g_infoWindowWidth>1000)g_infoWindowWidth=1000;
    g_trackTitleFontSize=GetIniIntAnySection(_T("TrackTitleFontSize"),17);
    if(g_trackTitleFontSize<8)g_trackTitleFontSize=8;if(g_trackTitleFontSize>40)g_trackTitleFontSize=40;
    g_textesFontSize=GetIniIntAnySection(_T("TextesFontSize"),10);
    if(g_textesFontSize<6)g_textesFontSize=6;if(g_textesFontSize>30)g_textesFontSize=30;
    g_instrumentFontSize=GetIniIntAnySection(_T("InstrumentFontSize"),9);
    if(g_instrumentFontSize<6)g_instrumentFontSize=6;if(g_instrumentFontSize>24)g_instrumentFontSize=24;
    g_musicdataFontSize=GetIniIntAnySection(_T("MusicdataFontSize"),10);
    if(g_musicdataFontSize<6)g_musicdataFontSize=6;if(g_musicdataFontSize>24)g_musicdataFontSize=24;
    g_footerAppSize=GetIniIntAnySection(_T("AppNameFontSize"),12);
    g_footerAuthorSize=GetIniIntAnySection(_T("AuthorFontSize"),9);
    g_footerVersionSize=GetIniIntAnySection(_T("VersionFontSize"),10);
    if(g_footerAppSize<6)g_footerAppSize=6;if(g_footerAppSize>30)g_footerAppSize=30;
    if(g_footerAuthorSize<6)g_footerAuthorSize=6;if(g_footerAuthorSize>30)g_footerAuthorSize=30;
    if(g_footerVersionSize<6)g_footerVersionSize=6;if(g_footerVersionSize>30)g_footerVersionSize=30;
    g_footerAppColor=ReadIniRgbAnySection(_T("AppNameColor"),RGB(30,30,45));
    g_footerAuthorColor=ReadIniRgbAnySection(_T("AuthorColor"),RGB(30,30,45));
    g_footerVersionColor=ReadIniRgbAnySection(_T("VersionColor"),RGB(30,30,45));
    g_uiBackground = ReadIniRgbAnySection(_T("Background"),RGB(49,46,72));
    g_uiLines = ReadIniRgbAnySection(_T("Lines"),RGB(105,103,139));
    g_uiProgress = ReadIniRgbAnySection(_T("ProgressBar"),RGB(225,225,220));
    g_uiTransportTop = ReadIniRgbAnySection(_T("ProgressBackground"),RGB(105,111,112));
    g_uiTransportBottom = ReadIniRgbAnySection(_T("TransportZoneBackground"),RGB(57,62,65));
    g_uiTransportEdge = ReadIniRgbAnySection(_T("ProgressTopEdge"),RGB(150,155,155));
    g_progressHeight = GetIniIntAnySection(_T("ProgressHeight"),24);
    if (g_progressHeight < 2) g_progressHeight = 2;
    if (g_progressHeight > 28) g_progressHeight = 28;
    TCHAR notePixelWH[64]={};
    if(GetIniValueAnySection(_T("NotePixelWH"),notePixelWH,64,_T(""))) {
        int nw=4,nh=6;
        if(_stscanf_s(notePixelWH,_T(" %d , %d"),&nw,&nh)==2) {
            if(nw>=1&&nw<=32)g_notePixelW=nw;
            if(nh>=1&&nh<=32)g_notePixelH=nh;
        }
    }
    g_sharpPixelShift = GetIniIntAnySection(_T("SharpPixelShift"),1);
    if(g_sharpPixelShift<0)g_sharpPixelShift=0;
    if(g_sharpPixelShift>8)g_sharpPixelShift=8;
    g_lineThickness = GetIniIntAnySection(_T("LineThickness"),2);
    if (g_lineThickness < 1) g_lineThickness = 1;
    if (g_lineThickness > 8) g_lineThickness = 8;
    g_firstVisibleNote = GetIniIntAnySection(_T("FirstNote"),12);
    g_lastVisibleNote = GetIniIntAnySection(_T("LastNote"),115);
    if (g_firstVisibleNote < 0) g_firstVisibleNote = 0;
    if (g_firstVisibleNote > 126) g_firstVisibleNote = 126;
    if (g_lastVisibleNote < 1) g_lastVisibleNote = 1;
    if (g_lastVisibleNote > 127) g_lastVisibleNote = 127;
    if (g_lastVisibleNote <= g_firstVisibleNote) { g_firstVisibleNote = 12; g_lastVisibleNote = 115; }
    g_linesC3Color = ReadIniRgbAnySection(_T("LinesMiddleC3color"),RGB(255,210,90));
    g_linesCColor = ReadIniRgbAnySection(_T("LinesCOctavesCcolor"),RGB(125,145,190));
    g_verticalPanCenterLine = ReadIniRgbAnySection(_T("verticalPanCenterLine"),RGB(110,100,75));
    g_windowButtonsColor = ReadIniRgbAnySection(_T("WindowButtonsColor"),g_uiBackground);
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
    _tcscat_s(path, MAX_PATH, _T("datas programme\\GM-XG.XML"));

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
static void MidiPanic() {
    if(g_midiOut) {
        for(int ch=0;ch<MIDI_CHANNELS;++ch)
            SendMidiShort((unsigned char)(0xB0|ch),123,0);
    }
    memset(g_active,0,sizeof(g_active));
    memset(g_velocity,0,sizeof(g_velocity));
    for(size_t t=0;t<g_trackActive.size();++t) {
        std::fill(g_trackActive[t].begin(),g_trackActive[t].end(),0);
        std::fill(g_trackPercussionActive[t].begin(),g_trackPercussionActive[t].end(),0);
        std::fill(g_trackVelocity[t].begin(),g_trackVelocity[t].end(),0);
        std::fill(g_trackPercussionVelocity[t].begin(),g_trackPercussionVelocity[t].end(),0);
    }
}
static void BuildMidiOutIniPath() {
    TCHAR exe[MAX_PATH]={};GetModuleFileName(NULL,exe,MAX_PATH);
    lstrcpyn(g_midiOutIniPath,exe,MAX_PATH);
    TCHAR* slash=_tcsrchr(g_midiOutIniPath,_T('\\'));
    if(slash)*(slash+1)=0;
    _tcscat_s(g_midiOutIniPath,MAX_PATH,_T("MIDIOUT.INI"));
}
static void LoadMidiOutputSettings() {
    BuildMidiOutIniPath();
    g_useMidiMapper=GetPrivateProfileInt(_T("MidiOut"),_T("UseMidiMapper"),0,g_midiOutIniPath)!=0;
    TCHAR savedName[256]={};
    GetPrivateProfileString(_T("MidiOut"),_T("DeviceName"),_T("Microsoft GS Wavetable Synth"),savedName,256,g_midiOutIniPath);
    UINT count=midiOutGetNumDevs();
    g_selectedMidiDevice=(UINT)-1;
    int savedIndex=GetPrivateProfileInt(_T("MidiOut"),_T("DeviceIndex"),-1,g_midiOutIniPath);
    for(UINT i=0;i<count;++i){MIDIOUTCAPS caps={};if(midiOutGetDevCaps(i,&caps,sizeof(caps))==MMSYSERR_NOERROR&&_tcsicmp(caps.szPname,savedName)==0){g_selectedMidiDevice=i;break;}}
    if(g_selectedMidiDevice==(UINT)-1&&savedIndex>=0&&(UINT)savedIndex<count)g_selectedMidiDevice=(UINT)savedIndex;
    if(g_selectedMidiDevice==(UINT)-1)for(UINT i=0;i<count;++i){MIDIOUTCAPS caps={};if(midiOutGetDevCaps(i,&caps,sizeof(caps))==MMSYSERR_NOERROR&&_tcsicmp(caps.szPname,_T("Microsoft GS Wavetable Synth"))==0){g_selectedMidiDevice=i;break;}}
    if(g_selectedMidiDevice==(UINT)-1&&count>0)g_selectedMidiDevice=0;
    if(g_selectedMidiDevice==(UINT)-1)g_useMidiMapper=true;
}
static void SaveMidiOutputSettings() {
    WritePrivateProfileString(_T("MidiOut"),_T("UseMidiMapper"),g_useMidiMapper?_T("1"):_T("0"),g_midiOutIniPath);
    TCHAR index[32]={};_stprintf_s(index,_T("%u"),g_selectedMidiDevice);
    WritePrivateProfileString(_T("MidiOut"),_T("DeviceIndex"),index,g_midiOutIniPath);
    if(!g_useMidiMapper){MIDIOUTCAPS caps={};if(midiOutGetDevCaps(g_selectedMidiDevice,&caps,sizeof(caps))==MMSYSERR_NOERROR)WritePrivateProfileString(_T("MidiOut"),_T("DeviceName"),caps.szPname,g_midiOutIniPath);}
}
static bool OpenMidiOutput(HWND owner) {
    if(g_midiOut){AllNotesOff();midiOutClose(g_midiOut);g_midiOut=NULL;}
    UINT device=g_useMidiMapper?MIDI_MAPPER:g_selectedMidiDevice;
    MMRESULT result=midiOutOpen(&g_midiOut,device,0,0,CALLBACK_NULL);
    if(result==MMSYSERR_NOERROR){AllNotesOff();return true;}
    g_midiOut=NULL;
    TCHAR message[512]={};
    if(g_useMidiMapper)lstrcpyn(message,_T("Windows MIDI Mapper could not be opened.\\nMIDI PIXEL will remain available in visual-only mode."),512);
    else {MIDIOUTCAPS caps={};TCHAR name[256]=_T("Selected MIDI output");if(midiOutGetDevCaps(g_selectedMidiDevice,&caps,sizeof(caps))==MMSYSERR_NOERROR)lstrcpyn(name,caps.szPname,256);_stprintf_s(message,_T("The MIDI output \"%s\" could not be opened.\\nMIDI PIXEL will remain available in visual-only mode."),name);}
    MessageBox(owner,message,_T("MIDI PIXEL - MIDI output"),MB_ICONWARNING|MB_OK);
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
    memset(g_pitchBend,0,sizeof(g_pitchBend));
    for(int ch=0;ch<MIDI_CHANNELS;++ch)SendMidiShort((unsigned char)(0xE0|ch),0,64);
    std::fill(g_trackExpression.begin(),g_trackExpression.end(),127);
    std::fill(g_trackPan.begin(),g_trackPan.end(),64);
    memset(g_velocity, 0, sizeof(g_velocity));
    for(size_t t=0;t<g_trackActive.size();++t){std::fill(g_trackNoteChannel[t].begin(),g_trackNoteChannel[t].end(),-1);std::fill(g_trackActive[t].begin(),g_trackActive[t].end(),0);std::fill(g_trackPercussionActive[t].begin(),g_trackPercussionActive[t].end(),0);std::fill(g_trackVelocity[t].begin(),g_trackVelocity[t].end(),0);std::fill(g_trackPercussionVelocity[t].begin(),g_trackPercussionVelocity[t].end(),0);}
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
                    if(metaType==0x03) label="Sequence";
                    if(label && (metaType==0x01 || metaType==0x02 || metaType==0x03 || metaType==0x05 || metaType==0x06 || metaType==0x07)) {
                        std::string textValue=MidiTextToAnsi(payload,len);
                        bool hasVisibleText=false;
                        for(size_t ci=0;ci<textValue.size();++ci) {
                            unsigned char ch=(unsigned char)textValue[ci];
                            if(ch!=0 && ch!=' ' && ch!='\t' && ch!='\r' && ch!='\n') { hasVisibleText=true; break; }
                        }
                        if(hasVisibleText || metaType==0x06) midiText.push_back({tick,0,label,textValue});
                    }
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
                else if (command == 0xb0) events.push_back({ tick, 0, (int)ti, channel, 5, a, b });
                else if (command == 0xe0) events.push_back({ tick, 0, (int)ti, channel, 6, a, b });
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

    // Convert text-event ticks to playback time using the same tempo map as MIDI events.
    for(size_t mi=0;mi<midiText.size();++mi) {
        unsigned int textTempo=500000;
        unsigned long long textLastTick=0,textElapsed=0;
        for(size_t ti=0;ti<tempos.size();++ti) {
            if(tempos[ti].tick>midiText[mi].tick)break;
            unsigned long long dt=tempos[ti].tick-textLastTick;
            textElapsed+=(dt*(unsigned long long)textTempo)/division;
            textLastTick=tempos[ti].tick;
            textTempo=tempos[ti].usecPerQuarter;
        }
        unsigned long long dt=midiText[mi].tick-textLastTick;
        textElapsed+=(dt*(unsigned long long)textTempo)/division;
        midiText[mi].usec=textElapsed;
    }
    std::sort(midiText.begin(),midiText.end(),[](const MidiTextInfo& a,const MidiTextInfo& b){
        if(a.usec!=b.usec)return a.usec<b.usec;
        return a.tick<b.tick;
    });
    // Hide the conventional empty first track (often named "Track 01").
    // Keep all real track indices aligned with their MIDI events.
    if(!tracks.empty()) {
        bool firstTrackHasMusicalEvents=false;
        for(size_t i=0;i<events.size();++i) {
            if(events[i].track==0) { firstTrackHasMusicalEvents=true; break; }
        }
        if(!firstTrackHasMusicalEvents) {
            tracks.erase(tracks.begin());
            for(size_t i=0;i<events.size();++i) if(events[i].track>0) --events[i].track;
        }
    }
    g_events.swap(events);
    g_tracks.swap(tracks);
    g_trackActive.assign(g_tracks.size(),std::vector<unsigned char>(MIDI_NOTES,0));
    g_trackNoteChannel.assign(g_tracks.size(),std::vector<int>(MIDI_NOTES,-1));
    g_trackPercussionActive.assign(g_tracks.size(),std::vector<unsigned char>(MIDI_NOTES,0));
    g_trackVelocity.assign(g_tracks.size(),std::vector<int>(MIDI_NOTES,0));
    g_trackPercussionVelocity.assign(g_tracks.size(),std::vector<int>(MIDI_NOTES,0));
    g_trackExpression.assign(g_tracks.size(),127);
    g_trackPan.assign(g_tracks.size(),64);
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
static int DisplayRowCount(){return(g_trackMode&&g_loaded&&!g_tracks.empty())?(int)g_tracks.size():MIDI_CHANNELS;}
static int DisplayRowY(int row){int count=DisplayRowCount();return 34+((g_trackMode&&g_loaded)?row:(count-1-row))*18;}
static RECT NoteRect(int row,int note,int clientWidth){
 if(note<g_firstVisibleNote||note>g_lastVisibleNote)return RECT{-100,-100,-96,-94};
 int x=NotePixelX(note,clientWidth),y=DisplayRowY(row)-6;
 int pitchClass=note%12;
 if(pitchClass==1||pitchClass==3||pitchClass==6||pitchClass==8||pitchClass==10)y-=g_sharpPixelShift;
 RECT r={x,y,x+g_notePixelW,y+g_notePixelH};return r;
}
static int ProgressWidth(int clientWidth) {
    if (!g_totalUsec || clientWidth <= 0) return 0;
    unsigned long long width=(g_elapsedUsec*(unsigned long long)clientWidth)/g_totalUsec;
    if(width>(unsigned long long)clientWidth)width=clientWidth;
    return (int)width;
}
static void InvalidateInfoBlock(HWND hwnd) {
    RECT client; GetClientRect(hwnd,&client);
    int barY=client.bottom-77;
    int infoStep=g_musicdataFontSize+2;
    int infoTop=barY-8-5*infoStep;
    RECT info={20,infoTop,client.right-20,barY-4};
    if(info.bottom<info.top)info.bottom=info.top;
    InvalidateRect(hwnd,&info,FALSE);
}
static void InvalidateDynamic(HWND hwnd) {
    if(g_miniMode){InvalidateRect(hwnd,NULL,FALSE);return;}
    RECT r;
    GetClientRect(hwnd, &r);
    RECT notes={20,32,r.right-20,34+DisplayRowCount()*18};InvalidateRect(hwnd,&notes,FALSE);
    RECT presets={24,32,222,34+DisplayRowCount()*18};
    InvalidateRect(hwnd, &presets, FALSE);
    InvalidateInfoBlock(hwnd);
    RECT progress = { 0, r.bottom - 77, r.right, r.bottom - 40 };
    InvalidateRect(hwnd, &progress, FALSE);
}
static int CurrentMarkerIndexAtUsec(unsigned long long usec) {
    int current=-1;
    for(size_t i=0;i<g_midiText.size();++i) {
        if(g_midiText[i].label=="Marker" && g_midiText[i].usec<=usec)current=(int)i;
    }
    return current;
}
static void AdvancePlayback(HWND hwnd) {
    if (!g_playing) return;
    RECT client; GetClientRect(hwnd, &client);
    int oldProgressWidth = ProgressWidth(client.right);
    bool wasFinished = g_finished;
    int oldMarkerIndex=CurrentMarkerIndexAtUsec(g_elapsedUsec);
    unsigned long long now = GetTickCount64();
    if (g_lastMs) g_elapsedUsec += (now - g_lastMs) * 1000;
    g_lastMs = now;
    if (g_elapsedUsec > g_totalUsec) g_elapsedUsec = g_totalUsec;
    if(CurrentMarkerIndexAtUsec(g_elapsedUsec)!=oldMarkerIndex)InvalidateInfoBlock(hwnd);
    static ULONGLONG lastMeasureRefreshMs=0;
    if(now-lastMeasureRefreshMs>=50) {
        InvalidateInfoBlock(hwnd);
        lastMeasureRefreshMs=now;
    }
    bool dynamicChanged = false;
    unsigned long long displayedSecond = g_elapsedUsec / 1000000ULL;
    if (displayedSecond != g_lastDisplayedSecond) {
        RECT progressText = g_miniMode
            ? RECT{0,0,client.right,g_progressHeight}
            : RECT{0,client.bottom-77,client.right,client.bottom-77+g_progressHeight};
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
        } else if (e.type == 5) {
            SendMidiShort((unsigned char)(0xB0 | e.channel), (unsigned char)e.a, (unsigned char)e.b);
            if(e.track>=0&&(size_t)e.track<g_trackExpression.size()&&e.a==11){g_trackExpression[e.track]=e.b;RECT cr={22,DisplayRowY(e.track)-7,82,DisplayRowY(e.track)+9};InvalidateRect(hwnd,&cr,FALSE);}
            if(e.track>=0&&(size_t)e.track<g_trackPan.size()&&e.a==10){g_trackPan[e.track]=e.b;RECT cr={22,DisplayRowY(e.track)-7,82,DisplayRowY(e.track)+9};InvalidateRect(hwnd,&cr,FALSE);}
        } else if (e.type == 6) {
            SendMidiShort((unsigned char)(0xE0 | e.channel), (unsigned char)e.a, (unsigned char)e.b);
            int bend=((e.b&0x7f)<<7)|(e.a&0x7f);
            g_pitchBend[e.channel]=bend-8192;
            if(g_pitchBendDisplay&&g_pitchBendRange>0){RECT br={220,30,client.right-20,34+DisplayRowCount()*18};InvalidateRect(hwnd,&br,FALSE);}
        } else {
            RECT oldRect = g_trackMode?NoteRect(e.track,e.a,client.right):NoteRect(e.channel,e.a,client.right);
            if (e.type == 1) {
                SendMidiShort((unsigned char)(0x90 | e.channel), (unsigned char)e.a, (unsigned char)e.b);
                g_active[e.channel][e.a]=1;g_velocity[e.channel][e.a]=e.b;
                if(e.track>=0&&(size_t)e.track<g_trackActive.size()){g_trackNoteChannel[e.track][e.a]=e.channel;if(e.channel==9){g_trackPercussionActive[e.track][e.a]=1;g_trackPercussionVelocity[e.track][e.a]=e.b;}else{g_trackActive[e.track][e.a]=1;g_trackVelocity[e.track][e.a]=e.b;}}
            } else {
                SendMidiShort((unsigned char)(0x80 | e.channel), (unsigned char)e.a, (unsigned char)e.b);
                g_active[e.channel][e.a]=0;g_velocity[e.channel][e.a]=0;
                if(e.track>=0&&(size_t)e.track<g_trackActive.size()){g_trackActive[e.track][e.a]=0;g_trackPercussionActive[e.track][e.a]=0;g_trackVelocity[e.track][e.a]=0;g_trackPercussionVelocity[e.track][e.a]=0;g_trackNoteChannel[e.track][e.a]=-1;}
            }
            RECT newRect = g_trackMode?NoteRect(e.track,e.a,client.right):NoteRect(e.channel,e.a,client.right);
            if(g_miniMode) {
                int miniTop=g_progressHeight+(MINI_TRANSPORT_HEIGHT-MINI_SCREEN_H)/2;
                int miniY=miniTop+1+(MIDI_CHANNELS-1-e.channel)*2;
                int miniX=MINI_SCREEN_X+1+e.a;
                RECT miniPixel={miniX,miniY,miniX+1,miniY+1};
                InvalidateRect(hwnd,&miniPixel,FALSE);
            } else {
                InvalidateRect(hwnd, &oldRect, FALSE);
                InvalidateRect(hwnd, &newRect, FALSE);
            }
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
        RECT progressStep = g_miniMode
            ? RECT{oldProgressWidth,0,newProgressWidth,g_progressHeight}
            : RECT{oldProgressWidth,client.bottom-73,newProgressWidth,client.bottom-77+g_progressHeight};
        InvalidateRect(hwnd, &progressStep, FALSE);
    }
    if (wasFinished != g_finished) {
        RECT status = g_miniMode
            ? RECT{client.right-120,0,client.right,g_progressHeight}
            : RECT{client.right-120,client.bottom-77,client.right,client.bottom-77+g_progressHeight};
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
        if(e.type==1){g_active[e.channel][e.a]=1;g_velocity[e.channel][e.a]=e.b;if(e.track>=0&&(size_t)e.track<g_trackActive.size()){if(e.channel==9){g_trackPercussionActive[e.track][e.a]=1;g_trackPercussionVelocity[e.track][e.a]=e.b;}else{g_trackActive[e.track][e.a]=1;g_trackVelocity[e.track][e.a]=e.b;}}}
        else if(e.type==0){g_active[e.channel][e.a]=0;g_velocity[e.channel][e.a]=0;if(e.track>=0&&(size_t)e.track<g_trackActive.size()){g_trackActive[e.track][e.a]=0;g_trackPercussionActive[e.track][e.a]=0;g_trackVelocity[e.track][e.a]=0;g_trackPercussionVelocity[e.track][e.a]=0;}}
        else if (e.type == 2) g_program[e.channel] = e.a;
        else if (e.type == 5) { SendMidiShort((unsigned char)(0xB0 | e.channel), (unsigned char)e.a, (unsigned char)e.b); if(e.track>=0&&(size_t)e.track<g_trackExpression.size()&&e.a==11)g_trackExpression[e.track]=e.b; if(e.track>=0&&(size_t)e.track<g_trackPan.size()&&e.a==10)g_trackPan[e.track]=e.b; }
        else if (e.type == 6) { SendMidiShort((unsigned char)(0xE0 | e.channel), (unsigned char)e.a, (unsigned char)e.b); int bend=((e.b&0x7f)<<7)|(e.a&0x7f); g_pitchBend[e.channel]=bend-8192; }
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
static void SeekMarker(HWND hwnd,int direction) {
    if(!g_loaded)return;
    unsigned long long target=g_elapsedUsec;
    if(direction>0) {
        for(size_t i=0;i<g_midiText.size();++i)
            if(g_midiText[i].label=="Marker" && g_midiText[i].usec>g_elapsedUsec) { target=g_midiText[i].usec;break; }
    } else {
        for(size_t i=g_midiText.size();i>0;--i)
            if(g_midiText[i-1].label=="Marker" && g_midiText[i-1].usec<g_elapsedUsec) { target=g_midiText[i-1].usec;break; }
    }
    if(target!=g_elapsedUsec)SeekPlaybackToUsec(hwnd,target);
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
        if(!g_trackMode)AdjustWindowForDisplay(hwnd);DestroyBackBuffer();InvalidateRect(hwnd,NULL,FALSE);InvalidateDynamic(hwnd);
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

static void TxtCenteredMono(HDC dc,RECT r,LPCTSTR text,COLORREF color,int size,int weight=FW_NORMAL) {
    SetBkMode(dc,TRANSPARENT);
    SetTextColor(dc,color);
    HFONT f=CreateFont(size,0,0,0,weight,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,
        FIXED_PITCH|FF_MODERN,_T("Courier New"));
    HFONT old=(HFONT)SelectObject(dc,f);
    DrawText(dc,text,-1,&r,DT_SINGLELINE|DT_VCENTER|DT_CENTER|DT_END_ELLIPSIS|DT_NOPREFIX);
    SelectObject(dc,old);
    DeleteObject(f);
}
static void TxtFixed(HDC dc,int x,int y,LPCTSTR s,COLORREF color,int size=13,int weight=FW_NORMAL) {
    SetBkMode(dc,TRANSPARENT);
    SetTextColor(dc,color);
    HFONT f=CreateFont(size,0,0,0,weight,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,
        FIXED_PITCH|FF_MODERN,_T("Courier New"));
    HFONT old=(HFONT)SelectObject(dc,f);
    TextOut(dc,x,y,s,lstrlen(s));
    SelectObject(dc,old);
    DeleteObject(f);
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

static COLORREF ColorForPercussion(int velocity){double v=velocity/127.0,f=g_velocityMinimum+(1.0-g_velocityMinimum)*pow(v,g_velocityCurve);return RGB(LinearToSrgb(SrgbToLinear(GetRValue(g_percussionColor))*f),LinearToSrgb(SrgbToLinear(GetGValue(g_percussionColor))*f),LinearToSrgb(SrgbToLinear(GetBValue(g_percussionColor))*f));}
static void DrawLine(HDC dc, int x1, int y, int x2, COLORREF color) {
    HPEN p = CreatePen(PS_SOLID, g_lineThickness, color); HPEN old = (HPEN)SelectObject(dc, p);
    MoveToEx(dc, x1, y, NULL); LineTo(dc, x2, y); SelectObject(dc, old); DeleteObject(p);
}
static void SaveWindow(HWND hwnd) {
    if(g_miniMode)return;
    WritePrivateProfileString(_T("Interface"),_T("WindowsState"),g_fullscreen?_T("1"):_T("0"),g_iniPath);
    if (g_fullscreen || IsIconic(hwnd) || IsZoomed(hwnd)) return;
    RECT r;
    if (!GetWindowRect(hwnd, &r)) return;
    TCHAR n[32];
    _stprintf_s(n, _T("%d"), r.left); WritePrivateProfileString(_T("Interface"), _T("windowX"), n, g_iniPath);
    _stprintf_s(n, _T("%d"), r.top); WritePrivateProfileString(_T("Interface"), _T("windowY"), n, g_iniPath);
    _stprintf_s(n, _T("%d"), r.right - r.left); WritePrivateProfileString(_T("Interface"), _T("windowWidth"), n, g_iniPath);
    _stprintf_s(n, _T("%d"), r.bottom - r.top); WritePrivateProfileString(_T("Interface"), _T("windowHeight"), n, g_iniPath);
}
static void DrawMetadataBlock(HDC dc,int width,int height) {
    if(!g_loaded)return;
    const int barY=height-77;
    const int timelineX=225;
// Metadata: Sequence name (0x03), first Text Event (0x01), current Marker (0x06), Copyright (0x02).
const int titleLineHeight=g_trackTitleFontSize+5;
const int eventLineHeight=(g_textesFontSize>11?g_textesFontSize:11)+5;
const int blockHeight=titleLineHeight+3*eventLineHeight;
int blockY=barY-8-blockHeight;
RECT metadataBackground={timelineX,blockY,width-24,blockY+blockHeight};
FillRectColor(dc,metadataBackground,g_uiBackground);
const MidiTextInfo* sequenceName=NULL;
const MidiTextInfo* firstTextEvent=NULL;
const MidiTextInfo* currentMarker=NULL;
const MidiTextInfo* firstCopyright=NULL;
for(size_t i=0;i<g_midiText.size();++i) {
    const MidiTextInfo& item=g_midiText[i];
    if(item.label=="Sequence" && !sequenceName) sequenceName=&item;
    else if(item.label=="Text" && !firstTextEvent) firstTextEvent=&item;
    else if(item.label=="Marker" && item.usec<=g_elapsedUsec) currentMarker=&item;
    else if(item.label=="Copyright" && !firstCopyright) firstCopyright=&item;
}
const MidiTextInfo* displayItems[4]={sequenceName,firstTextEvent,currentMarker,firstCopyright};
for(int lineIndex=0;lineIndex<4;++lineIndex) {
    TCHAR eventText[1024]={};
    if(displayItems[lineIndex]) {
#ifdef UNICODE
        MultiByteToWideChar(CP_ACP,0,displayItems[lineIndex]->text.c_str(),-1,eventText,1024);
#else
        lstrcpynA(eventText,displayItems[lineIndex]->text.c_str(),1024);
#endif
    }
    int lineY=blockY;
    int lineHeight=titleLineHeight;
    int fontSize=g_trackTitleFontSize;
    if(lineIndex>0) {
        lineY=blockY+titleLineHeight+(lineIndex-1)*eventLineHeight;
        lineHeight=eventLineHeight;
        fontSize=g_textesFontSize;
    }
    RECT eventRect={timelineX,lineY,width-24,lineY+lineHeight};
    TxtCenteredMono(dc,eventRect,eventText,RGB(lineIndex==0?235:210,lineIndex==0?235:205,lineIndex==0?242:190),fontSize);
}

}
static void DrawStaticBackground(HDC dc, int width, int height) {
    RECT cr = { 0, 0, width, height };
    FillRectColor(dc, cr, g_uiBackground);
    const COLORREF label = RGB(190, 190, 205), line = g_uiLines;
    const int channelX = 24, presetX = 72, timelineX = 225, timelineR = width - 28;
    const int rowTop = 34, rowH = 18;
    int rows=DisplayRowCount();
    for(int row=0;row<rows;++row){int y=DisplayRowY(row);
      if(!g_trackMode){TCHAR ch[8];_stprintf_s(ch,_T("%02d"),row+1);Txt(dc,channelX,y-5,ch,label,10);}
      else if(g_loaded && row<(int)g_tracks.size()){std::string tn=g_tracks[row].name;if(tn.empty()){char fb[32];_snprintf_s(fb,sizeof(fb),_TRUNCATE,"Track %02d",row+1);tn=fb;}TCHAR name[256]={};
#ifdef UNICODE
       MultiByteToWideChar(CP_ACP,0,tn.c_str(),-1,name,256);
#else
       lstrcpynA(name,tn.c_str(),256);
#endif
       RECT nr={84,y-7,timelineX-7,y+9};SetBkMode(dc,TRANSPARENT);SetTextColor(dc,label);HFONT nf=CreateFont(g_instrumentFontSize,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_DONTCARE,_T("Segoe UI"));HFONT no=(HFONT)SelectObject(dc,nf);DrawText(dc,name,-1,&nr,DT_SINGLELINE|DT_VCENTER|DT_RIGHT|DT_END_ELLIPSIS|DT_NOPREFIX);SelectObject(dc,no);DeleteObject(nf);}
      if(g_trackMode&&g_loaded){RECT vr={24,y-2,48,y+2};FillRectColor(dc,vr,RGB(35,32,25));RECT pr={54,y-2,78,y+2};FillRectColor(dc,pr,RGB(35,32,25));}
      DrawLine(dc,timelineX,y+2,timelineR,line);
      for(int n=g_firstVisibleNote;n<=g_lastVisibleNote;++n)if(n%12==0){int mx=NotePixelX(n,width),half=g_lineThickness/2;RECT mr={mx,y+2-half,mx+g_notePixelW,y+2-half+g_lineThickness};FillRectColor(dc,mr,n==60?g_linesC3Color:g_linesCColor);}
    }
    if(g_trackMode&&g_loaded&&rows>0){int firstY=DisplayRowY(0),lastY=DisplayRowY(rows-1);HPEN panPen=CreatePen(PS_SOLID,1,g_verticalPanCenterLine);HPEN oldPan=(HPEN)SelectObject(dc,panPen);MoveToEx(dc,66,firstY-7,NULL);LineTo(dc,66,lastY+8);SelectObject(dc,oldPan);DeleteObject(panPen);}
    int headerY=rowTop+rows*rowH+4;if(!g_trackMode){Txt(dc,channelX,headerY,_T("CH"),label,9,FW_SEMIBOLD);Txt(dc,presetX,headerY,_T("PROGRAM"),label,9,FW_SEMIBOLD);}int infoY=headerY+12;
    TCHAR info[128];
    int barY = height - 77;
    if (g_loaded) {
        // Track count and PPQ are drawn dynamically in Paint, not cached.
    }
    FillRectColor(dc, RECT{ 0, barY, width, barY + g_progressHeight }, g_uiTransportTop);
    FillRectColor(dc, RECT{ 0, barY + g_progressHeight, width, height }, g_uiTransportBottom);
    FillRectColor(dc, RECT{ 0, barY, width, barY + 2 }, g_uiTransportEdge);
    int controlsY = barY + g_progressHeight + 9;
    Txt(dc,40,controlsY,_T("SETTINGS"),RGB(235,235,242),12);
    Txt(dc,40,controlsY+17,g_trackMode?_T("TRACK MODE"):_T("MIDI MODE"),RGB(190,190,205),9);
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
        MONITORINFO mi={};mi.cbSize=sizeof(mi);
        HMONITOR mon=MonitorFromWindow(hwnd,MONITOR_DEFAULTTONEAREST);
        if(!GetMonitorInfo(mon,&mi))return;
        // Keep the same borderless window and resize it to the monitor bounds.
        SetWindowPos(hwnd,HWND_TOP,mi.rcMonitor.left,mi.rcMonitor.top,
            mi.rcMonitor.right-mi.rcMonitor.left,mi.rcMonitor.bottom-mi.rcMonitor.top,
            SWP_NOOWNERZORDER|SWP_NOACTIVATE);
        g_fullscreen=true;
    } else {
        SetWindowPos(hwnd,HWND_TOP,g_windowedRect.left,g_windowedRect.top,
            g_windowedRect.right-g_windowedRect.left,g_windowedRect.bottom-g_windowedRect.top,
            SWP_NOOWNERZORDER|SWP_NOACTIVATE);
        g_fullscreen=false;
    }
    DestroyBackBuffer();
    InvalidateRect(hwnd,NULL,FALSE);
}
static void ToggleMiniMode(HWND hwnd) {
    if(!g_miniMode) {
        if(g_fullscreen)ToggleFullscreen(hwnd);
        if(!GetWindowRect(hwnd,&g_miniRestoreRect))return;
        g_miniMode=true;
        if(g_infoWnd)ShowWindow(g_infoWnd,SW_HIDE);
        if(g_settingsWnd)ShowWindow(g_settingsWnd,SW_HIDE);
        int miniHeight=g_progressHeight+MINI_TRANSPORT_HEIGHT;
        int miniTop=g_miniRestoreRect.bottom-miniHeight;
        SetWindowPos(hwnd,HWND_TOP,g_miniRestoreRect.left,miniTop,
            g_miniRestoreRect.right-g_miniRestoreRect.left,miniHeight,
            SWP_NOOWNERZORDER|SWP_NOACTIVATE);
    } else {
        g_miniMode=false;
        SetWindowPos(hwnd,HWND_TOP,g_miniRestoreRect.left,g_miniRestoreRect.top,
            g_miniRestoreRect.right-g_miniRestoreRect.left,
            g_miniRestoreRect.bottom-g_miniRestoreRect.top,
            SWP_NOOWNERZORDER|SWP_NOACTIVATE);
    }
    DestroyBackBuffer();InvalidateRect(hwnd,NULL,FALSE);
}
static void AdjustWindowForDisplay(HWND hwnd){
 if(!hwnd||g_fullscreen)return;RECT wr,cr;GetWindowRect(hwnd,&wr);GetClientRect(hwnd,&cr);int rows=DisplayRowCount();if(rows<16)rows=16;
 int wantedClient=34+rows*18+160,wanted=(wr.bottom-wr.top)+(wantedClient-cr.bottom),maxH=GetSystemMetrics(SM_CYSCREEN)-60;if(wanted>maxH)wanted=maxH;int current=wr.bottom-wr.top;if(wanted<current)wanted=current;
 if(wanted!=current)SetWindowPos(hwnd,NULL,wr.left,wr.top,wr.right-wr.left,wanted,SWP_NOZORDER|SWP_NOACTIVATE);
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
static void DrawWindowButtonFrame(HDC dc,RECT r,COLORREF color) {
    HPEN pen=CreatePen(PS_SOLID,1,color);
    HPEN oldPen=(HPEN)SelectObject(dc,pen);
    HBRUSH oldBrush=(HBRUSH)SelectObject(dc,GetStockObject(NULL_BRUSH));
    Rectangle(dc,r.left,r.top,r.right,r.bottom);
    SelectObject(dc,oldBrush);SelectObject(dc,oldPen);DeleteObject(pen);
}
static void DrawNotePitchBend(HDC dc,const RECT& noteRect,int bend) {
    if(!g_pitchBendDisplay||g_pitchBendRange<=0||bend==0)return;
    int length=(int)((long long)bend*g_pitchBendRange/8192);
    if(length==0)return;
    int anchor=noteRect.left;
    int x1=length<0?anchor+length:anchor;
    int x2=length<0?anchor:anchor+length;
    int y=noteRect.top+(noteRect.bottom-noteRect.top)/2;
    HPEN pen=CreatePen(PS_SOLID,2,g_midiLinearControlsColor);
    HPEN old=(HPEN)SelectObject(dc,pen);
    MoveToEx(dc,x1,y,NULL);LineTo(dc,x2,y);
    SelectObject(dc,old);DeleteObject(pen);
}
static void Paint(HWND hwnd) {
    PAINTSTRUCT ps; HDC dc = BeginPaint(hwnd, &ps);
    RECT cr; GetClientRect(hwnd, &cr);
    if(g_miniMode) {
        const int controlsY=g_progressHeight+9;
        FillRectColor(dc,cr,g_uiBackground);
        FillRectColor(dc,RECT{0,0,cr.right,g_progressHeight},g_uiTransportTop);
        FillRectColor(dc,RECT{0,2,ProgressWidth(cr.right),g_progressHeight-2},g_uiProgress);
        FillRectColor(dc,RECT{0,g_progressHeight,cr.right,cr.bottom},g_uiTransportBottom);
        FillRectColor(dc,RECT{0,0,cr.right,2},g_uiTransportEdge);
        TCHAR timeText[64];unsigned long long es=g_elapsedUsec/1000000ULL,ts=g_totalUsec/1000000ULL;
        _stprintf_s(timeText,_T("%02llu:%02llu  /  %02llu:%02llu"),es/60,es%60,ts/60,ts%60);
        int fs=g_progressHeight-2;if(fs<8)fs=8;if(fs>26)fs=26;
        HFONT mf=CreateFont(fs,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_DONTCARE,_T("Segoe UI"));
        HFONT om=(HFONT)SelectObject(dc,mf);SIZE z={};GetTextExtentPoint32(dc,timeText,lstrlen(timeText),&z);SelectObject(dc,om);DeleteObject(mf);
        int ty=(g_progressHeight-fs)/2;Txt(dc,cr.right-z.cx-14,ty,timeText,RGB(235,235,242),fs);Txt(dc,20,ty,g_fileName,RGB(235,235,242),fs);
        Txt(dc,40,controlsY,_T("SETTINGS"),RGB(235,235,242),12);
        Txt(dc,cr.right/2-24,controlsY,_T("LOAD"),RGB(235,235,242),13,FW_SEMIBOLD);
        Txt(dc,cr.right/2+45,controlsY,_T("INFO"),RGB(190,190,205),13,FW_SEMIBOLD);
        Txt(dc,cr.right-115,controlsY,_T("STOP"),RGB(235,235,242),12,FW_SEMIBOLD);
        Txt(dc,cr.right-55,controlsY,_T("PLAY"),RGB(235,235,242),12,FW_SEMIBOLD);
        int miniTop=g_progressHeight+(MINI_TRANSPORT_HEIGHT-MINI_SCREEN_H)/2;
        RECT mini={MINI_SCREEN_X,miniTop,MINI_SCREEN_X+MINI_SCREEN_W,miniTop+MINI_SCREEN_H};
        FillRectColor(dc,mini,g_uiBackground);DrawWindowButtonFrame(dc,mini,g_uiLines);
        for(int ch=0;ch<MIDI_CHANNELS;++ch) {
            int py=mini.top+1+(MIDI_CHANNELS-1-ch)*2;
            for(int note=0;note<MIDI_NOTES;++note)if(g_active[ch][note]) {
                int px=mini.left+1+note;
                COLORREF color=(ch==9)?ColorForPercussion(g_velocity[ch][note]):ColorForNote(note,g_velocity[ch][note]);
                RECT pixel={px,py,px+1,py+1};
                FillRectColor(dc,pixel,color);
            }
        }
        Txt(dc,cr.right/2-48,cr.bottom-17,_T("MIDI PIXEL"),g_footerAppColor,g_footerAppSize);
        Txt(dc,cr.right/2+50,cr.bottom-17,VERSION,g_footerVersionColor,g_footerVersionSize);
        Txt(dc,cr.right-95,cr.bottom-16,_T("cslevine+IA"),g_footerAuthorColor,g_footerAuthorSize);
        EndPaint(hwnd,&ps);return;
    }
    if (EnsureBackBuffer(hwnd)) BitBlt(dc, ps.rcPaint.left, ps.rcPaint.top,
        ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top,
        g_backDC, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);
    else FillRectColor(dc, ps.rcPaint, g_uiBackground);
    // Metadata is dynamic (not part of the cached static background).
    DrawMetadataBlock(dc,cr.right,cr.bottom);

    const int timelineX = 225, timelineR = cr.right - 28, rowTop = 34, rowH = 18;
    int headerY = rowTop + MIDI_CHANNELS * rowH + 4;
    int infoY = headerY + 12;
    if (g_loaded) {
        TCHAR metaText[128];
        int barY = cr.bottom - 77;
        int infoStep=g_musicdataFontSize+2;
        int infoTop=barY-8-5*infoStep;
        _stprintf_s(metaText, _T("TRACKS : %8u"), (unsigned int)g_tracks.size());
        TxtFixed(dc, 24, infoTop, metaText, RGB(205, 210, 225), g_musicdataFontSize);
        _stprintf_s(metaText, _T("PPQ    : %8u"), (unsigned int)g_division);
        TxtFixed(dc, 24, infoTop+infoStep, metaText, RGB(205, 210, 225), g_musicdataFontSize);
        double bpm = g_currentTempo ? 60000000.0 / (double)g_currentTempo : 120.0;
        _stprintf_s(metaText, _T("TEMPO  : %8.0f"), bpm);
        TxtFixed(dc, 24, infoTop+2*infoStep, metaText, RGB(205, 210, 225), g_musicdataFontSize);
        unsigned int denominator = 1u << g_currentTimeDenominatorPower;
        TCHAR meterValue[32];
        _stprintf_s(meterValue, _T("%u/%u"), (unsigned int)g_currentTimeNumerator, denominator);
        _stprintf_s(metaText, _T("METER  : %8s"), meterValue);
        TxtFixed(dc, 24, infoTop+3*infoStep, metaText, RGB(205, 210, 225), g_musicdataFontSize);
        unsigned long long lastUsec=0,lastTick=0;
        for(size_t i=0;i<g_events.size();++i) {
            if(g_events[i].usec>g_elapsedUsec)break;
            lastUsec=g_events[i].usec;lastTick=g_events[i].tick;
        }
        unsigned long long currentTick=lastTick;
        if(g_currentTempo>0)currentTick+= (unsigned long long)((double)(g_elapsedUsec-lastUsec)*(double)g_division/(double)g_currentTempo);
        unsigned int denom=1u<<g_currentTimeDenominatorPower;
        unsigned long long ticksPerBeat=(unsigned long long)g_division*4u/denom;
        if(!ticksPerBeat)ticksPerBeat=1;
        unsigned long long ticksPerMeasure=ticksPerBeat*(g_currentTimeNumerator?g_currentTimeNumerator:4);
        if(!ticksPerMeasure)ticksPerMeasure=1;
        unsigned long long measure=currentTick/ticksPerMeasure+1;
        unsigned long long beat=(currentTick%ticksPerMeasure)/ticksPerBeat+1;
TCHAR measureValue[64]={};
        _stprintf_s(measureValue,_T("%llu %llu"),measure,beat);
        _stprintf_s(metaText,_T("MEASURE: %8s"),measureValue);
        TxtFixed(dc,24,infoTop+4*infoStep,metaText,RGB(205,210,225),g_musicdataFontSize);
    }
    int rows=DisplayRowCount();
    if(!g_trackMode){for(int ch=0;ch<MIDI_CHANNELS;++ch){int y=DisplayRowY(ch);TCHAR p[16]={};if(ch==9)lstrcpyn(p,_T("RYTHME"),16);else _stprintf_s(p,_T("%03d"),g_program[ch]+1);Txt(dc,72,y-5,g_loaded?p:_T("---"),RGB(160,170,195),10);
      if(ch!=9&&g_loaded&&g_programNames[g_program[ch]][0]){RECT nr={106,y-7,timelineX-7,y+9};TxtClipped(dc,nr,g_programNames[g_program[ch]],RGB(205,210,225),g_instrumentFontSize);}
      if(!g_loaded)continue;for(int n=0;n<MIDI_NOTES;++n)if(g_active[ch][n]){RECT nr=NoteRect(ch,n,cr.right);if(nr.right<ps.rcPaint.left||nr.left>ps.rcPaint.right||nr.bottom<ps.rcPaint.top||nr.top>ps.rcPaint.bottom)continue;FillRectColor(dc,nr,ch==9?ColorForPercussion(g_velocity[ch][n]):ColorForNote(n,g_velocity[ch][n]));DrawNotePitchBend(dc,nr,g_pitchBend[ch]);}}}
    else if(g_loaded){for(int tr=0;tr<rows;++tr){int y=DisplayRowY(tr);RECT vr={24,y-2,24+(24*g_trackExpression[tr])/127,y+2};if(vr.right>vr.left&&vr.right>ps.rcPaint.left&&vr.left<ps.rcPaint.right&&vr.bottom>ps.rcPaint.top&&vr.top<ps.rcPaint.bottom)FillRectColor(dc,vr,g_midiLinearControlsColor);int px=54+(24*g_trackPan[tr])/127;RECT pan={px-1,y-5,px+2,y+6};if(pan.right>ps.rcPaint.left&&pan.left<ps.rcPaint.right&&pan.bottom>ps.rcPaint.top&&pan.top<ps.rcPaint.bottom)FillRectColor(dc,pan,g_midiLinearControlsColor);for(int n=0;n<MIDI_NOTES;++n){RECT nr=NoteRect(tr,n,cr.right);if(nr.right<ps.rcPaint.left||nr.left>ps.rcPaint.right||nr.bottom<ps.rcPaint.top||nr.top>ps.rcPaint.bottom)continue;if(g_trackActive[tr][n]){FillRectColor(dc,nr,ColorForNote(n,g_trackVelocity[tr][n]));int ch=g_trackNoteChannel[tr][n];if(ch>=0&&ch<MIDI_CHANNELS)DrawNotePitchBend(dc,nr,g_pitchBend[ch]);}if(g_trackPercussionActive[tr][n]){FillRectColor(dc,nr,ColorForPercussion(g_trackPercussionVelocity[tr][n]));int ch=g_trackNoteChannel[tr][n];if(ch>=0&&ch<MIDI_CHANNELS)DrawNotePitchBend(dc,nr,g_pitchBend[ch]);}}}}
    int barY = cr.bottom - 77;
    RECT progressFill = { 0, barY + 2, ProgressWidth(cr.right), barY + g_progressHeight - 2 };
    RECT visibleFill;
    if (IntersectRect(&visibleFill, &progressFill, &ps.rcPaint))
        FillRectColor(dc, visibleFill, g_uiProgress);
    TCHAR timeText[64];
    unsigned long long elapsedSec=g_elapsedUsec/1000000ULL,totalSec=g_totalUsec/1000000ULL;
    _stprintf_s(timeText,_T("%02llu:%02llu  /  %02llu:%02llu"),elapsedSec/60,elapsedSec%60,totalSec/60,totalSec%60);
    int progressFontSize = g_progressHeight - 2;
    if(progressFontSize < 8) progressFontSize = 8;
    if(progressFontSize > 26) progressFontSize = 26;
    HFONT measureFont=CreateFont(progressFontSize,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_DONTCARE,_T("Segoe UI"));
    HFONT oldMeasure=(HFONT)SelectObject(dc,measureFont);
    SIZE timeSize={}; GetTextExtentPoint32(dc,timeText,lstrlen(timeText),&timeSize);
    SelectObject(dc,oldMeasure); DeleteObject(measureFont);
    int progressTextY = barY + (g_progressHeight - progressFontSize) / 2;
    Txt(dc,cr.right-timeSize.cx-14,progressTextY,timeText,RGB(235,235,242),progressFontSize);
    Txt(dc,20,progressTextY,g_fileName,RGB(235,235,242),progressFontSize);
    if(g_windowButtonsHover) {
        const int buttonY=4,buttonW=21,buttonH=19,buttonGap=3;
        const int closeX=cr.right-27,maxX=closeX-buttonW-buttonGap,minX=maxX-buttonW-buttonGap,miniX=minX-buttonW-buttonGap;
        RECT band={miniX-5,0,cr.right,buttonY+buttonH+4};
        HBRUSH bandBrush=CreateSolidBrush(g_windowButtonsColor);
        FillRect(dc,&band,bandBrush);
        DeleteObject(bandBrush);
        RECT button={minX,buttonY,minX+buttonW,buttonY+buttonH};
        TxtCenteredMono(dc,button,_T("_"),RGB(225,225,232),12,FW_BOLD);
        button={maxX,buttonY,maxX+buttonW,buttonY+buttonH};
        {
            const wchar_t maximizeGlyph[] = L"\xE922";
            SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(225,225,232));
            HFONT glyphFont=CreateFontW(13,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_DONTCARE,
                L"Segoe MDL2 Assets");
            HFONT oldGlyphFont=(HFONT)SelectObject(dc,glyphFont);
            DrawTextW(dc,maximizeGlyph,-1,&button,DT_SINGLELINE|DT_VCENTER|DT_CENTER|DT_NOPREFIX);
            SelectObject(dc,oldGlyphFont);DeleteObject(glyphFont);
        }
        button={miniX,buttonY,miniX+buttonW,buttonY+buttonH};
        TxtCenteredMono(dc,button,_T("M"),RGB(225,225,232),11,FW_BOLD);
        button={closeX,buttonY,closeX+buttonW,buttonY+buttonH};
        TxtCenteredMono(dc,button,_T("X"),RGB(225,225,232),11,FW_BOLD);
    }
    EndPaint(hwnd, &ps);
}
static void FillRectColor(HDC dc,RECT r,COLORREF color);
static void RefreshOutputCombo() {
    if(!g_outputCombo)return;
    SendMessage(g_outputCombo,CB_RESETCONTENT,0,0);
    UINT count=midiOutGetNumDevs();int selected=-1;
    for(UINT i=0;i<count;++i){MIDIOUTCAPS caps={};if(midiOutGetDevCaps(i,&caps,sizeof(caps))!=MMSYSERR_NOERROR)continue;int row=(int)SendMessage(g_outputCombo,CB_ADDSTRING,0,(LPARAM)caps.szPname);SendMessage(g_outputCombo,CB_SETITEMDATA,row,(LPARAM)i);if(i==g_selectedMidiDevice)selected=row;}
    if(selected>=0)SendMessage(g_outputCombo,CB_SETCURSEL,selected,0);else if(SendMessage(g_outputCombo,CB_GETCOUNT,0,0)>0)SendMessage(g_outputCombo,CB_SETCURSEL,0,0);
    EnableWindow(g_outputCombo,!g_useMidiMapper);
}

struct SettingItem { const TCHAR* section; const TCHAR* key; const TCHAR* label; bool color; };
static std::vector<SettingItem> g_settingItems;
static int g_settingsPage=0;
static HWND g_settingsTree=NULL,g_settingsScrollBar=NULL;
static int g_settingsScroll=0,g_settingsMaxScroll=0;
static const TCHAR* g_settingsPageNames[]={_T("MIDI OUTPUT"),_T("INTERFACE"),_T("NOTES & COLORS"),_T("DISPLAY"),_T("PROGRESS"),_T("FOOTER"),_T("KEYBOARD")};
static const int SETTINGS_NAV_BASE=300,SETTINGS_FIELD_BASE=2000;
static HFONT g_settingsFont=NULL;
static void AddSettingItem(const TCHAR* section,const TCHAR* key,const TCHAR* label,bool color=false) {
    SettingItem item={section,key,label,color};g_settingItems.push_back(item);
}
static void LoadSettingsPageItems(int page) {
    g_settingItems.clear();
    if(page==1) {
        AddSettingItem(_T("Interface"),_T("Background"),_T("Main background"),true);
        AddSettingItem(_T("Interface"),_T("WindowButtonsColor"),_T("Buttons color"),true);
        AddSettingItem(_T("Interface"),_T("instrumentsMidiMode"),_T("Initial display mode"));
        AddSettingItem(_T("Interface"),_T("InfoWindowWidth"),_T("Information window width"));
    } else if(page==2) {
        const TCHAR* keys[]={_T("C"),_T("C_SHARP"),_T("D"),_T("D_SHARP"),_T("E"),_T("F"),_T("F_SHARP"),_T("G"),_T("G_SHARP"),_T("A"),_T("A_SHARP"),_T("B")};
        const TCHAR* labels[]={_T("C"),_T("C sharp"),_T("D"),_T("D sharp"),_T("E"),_T("F"),_T("F sharp"),_T("G"),_T("G sharp"),_T("A"),_T("A sharp"),_T("B")};
        for(int i=0;i<12;++i)AddSettingItem(_T("Notes"),keys[i],labels[i],true);
        AddSettingItem(_T("Notes"),_T("PercussionColor"),_T("Percussion"),true);
        AddSettingItem(_T("Notes"),_T("MidiLinearControlsColor"),_T("MIDI controls"),true);
        AddSettingItem(_T("Notes"),_T("NotePixelWH"),_T("Note pixel size (W,H)"));
        AddSettingItem(_T("Notes"),_T("SharpPixelShift"),_T("Sharp note shift"));
        AddSettingItem(_T("Notes"),_T("VelocityMinimumPercent"),_T("Minimum brightness (%)"));
        AddSettingItem(_T("Notes"),_T("VelocityCurve"),_T("Velocity curve"));
    } else if(page==3) {
        AddSettingItem(_T("Display"),_T("FirstNote"),_T("First visible MIDI note"));
        AddSettingItem(_T("Display"),_T("LastNote"),_T("Last visible MIDI note"));
        AddSettingItem(_T("Display"),_T("LineThickness"),_T("Guide line thickness"));
        AddSettingItem(_T("Display"),_T("Lines"),_T("Guide lines"),true);
        AddSettingItem(_T("Display"),_T("LinesMiddleC3color"),_T("Middle C3 marker"),true);
        AddSettingItem(_T("Display"),_T("LinesCOctavesCcolor"),_T("Other C markers"),true);
        AddSettingItem(_T("Display"),_T("verticalPanCenterLine"),_T("Pan center line"),true);
        AddSettingItem(_T("Display"),_T("TrackTitleFontSize"),_T("Track title font size"));
        AddSettingItem(_T("Display"),_T("TextesFontSize"),_T("Lyrics / marker font size"));
        AddSettingItem(_T("Display"),_T("InstrumentFontSize"),_T("Instrument font size"));
        AddSettingItem(_T("Display"),_T("MusicdataFontSize"),_T("Information font size"));
    } else if(page==4) {
        AddSettingItem(_T("Progress"),_T("ProgressTopEdge"),_T("Progress top edge"),true);
        AddSettingItem(_T("Progress"),_T("ProgressBar"),_T("Progress bar"),true);
        AddSettingItem(_T("Progress"),_T("ProgressBackground"),_T("Progress background"),true);
        AddSettingItem(_T("Progress"),_T("ProgressHeight"),_T("Progress bar height"));
    } else if(page==5) {
        AddSettingItem(_T("Footer"),_T("TransportZoneBackground"),_T("Transport zone background"),true);
        AddSettingItem(_T("Footer"),_T("AppNameFontSize"),_T("Application name size"));
        AddSettingItem(_T("Footer"),_T("AppNameColor"),_T("Application name color"),true);
        AddSettingItem(_T("Footer"),_T("AuthorFontSize"),_T("Author size"));
        AddSettingItem(_T("Footer"),_T("AuthorColor"),_T("Author color"),true);
        AddSettingItem(_T("Footer"),_T("VersionFontSize"),_T("Version size"));
        AddSettingItem(_T("Footer"),_T("VersionColor"),_T("Version color"),true);
    } else if(page==6) {
        AddSettingItem(_T("Keyboard"),_T("PlayPause"),_T("Play / Pause"));
        AddSettingItem(_T("Keyboard"),_T("Left"),_T("Seek backward"));
        AddSettingItem(_T("Keyboard"),_T("Right"),_T("Seek forward"));
        AddSettingItem(_T("Keyboard"),_T("VolumeUp"),_T("Volume up"));
        AddSettingItem(_T("Keyboard"),_T("VolumeDown"),_T("Volume down"));
        AddSettingItem(_T("Keyboard"),_T("MidiPanic"),_T("MIDI Panic (! = Shift+1)"));
        AddSettingItem(_T("Keyboard"),_T("SeekSeconds"),_T("Seek interval (seconds)"));
        AddSettingItem(_T("Keyboard"),_T("MiniMode"),_T("Mini mode toggle"));
        AddSettingItem(_T("Keyboard"),_T("Settings"),_T("Settings toggle"));
        AddSettingItem(_T("Keyboard"),_T("DisplayMode"),_T("Channels / Tracks toggle"));
    }
}
struct SettingsValue { std::basic_string<TCHAR> section,key,value; };
static std::vector<SettingsValue> g_settingsOriginal,g_settingsDraft;
static bool g_settingsOriginalUseMapper=false;
static UINT g_settingsOriginalDevice=(UINT)-1;
static std::basic_string<TCHAR> GetDraftSettingValue(const TCHAR* section,const TCHAR* key,const TCHAR* fallback) {
    for(size_t i=0;i<g_settingsDraft.size();++i)
        if(_tcsicmp(g_settingsDraft[i].section.c_str(),section)==0&&_tcsicmp(g_settingsDraft[i].key.c_str(),key)==0)return g_settingsDraft[i].value;
    return fallback?fallback:_T("");
}
static void SetDraftSettingValue(const TCHAR* section,const TCHAR* key,const TCHAR* value) {
    for(size_t i=0;i<g_settingsDraft.size();++i)
        if(_tcsicmp(g_settingsDraft[i].section.c_str(),section)==0&&_tcsicmp(g_settingsDraft[i].key.c_str(),key)==0){g_settingsDraft[i].value=value?value:_T("");return;}
    SettingsValue item;item.section=section;item.key=key;item.value=value?value:_T("");g_settingsDraft.push_back(item);
}
static void BeginSettingsTransaction() {
    g_settingsOriginal.clear();g_settingsDraft.clear();
    for(int page=1;page<=6;++page) {
        LoadSettingsPageItems(page);
        for(size_t i=0;i<g_settingItems.size();++i) {
            TCHAR value[128]={};GetIniValueAnySection(g_settingItems[i].key,value,128,_T(""));
            SettingsValue item;item.section=g_settingItems[i].section;item.key=g_settingItems[i].key;item.value=value;
            g_settingsOriginal.push_back(item);g_settingsDraft.push_back(item);
        }
    }
    g_settingsOriginalUseMapper=g_useMidiMapper;g_settingsOriginalDevice=g_selectedMidiDevice;
    LoadSettingsPageItems(g_settingsPage);
}
static void WriteSettingsValues(const std::vector<SettingsValue>& values) {
    for(size_t i=0;i<values.size();++i)
        WritePrivateProfileString(values[i].section.c_str(),values[i].key.c_str(),values[i].value.c_str(),g_iniPath);
}
static void SettingsCreateLabel(HWND hwnd,const TCHAR* text,int x,int y,int w,int h,int id) {
    HWND c=CreateWindowEx(0,_T("STATIC"),text,WS_CHILD|WS_VISIBLE,x,y,w,h,hwnd,(HMENU)(INT_PTR)id,NULL,NULL);
    if(g_settingsFont)SendMessage(c,WM_SETFONT,(WPARAM)g_settingsFont,TRUE);
}
static void UpdateSettingsControlVisibility(HWND hwnd) {
    for(int id=SETTINGS_FIELD_BASE;id<SETTINGS_FIELD_BASE+200;++id) {
        HWND c=GetDlgItem(hwnd,id);if(!c)continue;
        RECT r;GetWindowRect(c,&r);MapWindowPoints(NULL,hwnd,(POINT*)&r,2);
        ShowWindow(c,(r.bottom>18&&r.top<383)?SW_SHOWNA:SW_HIDE);
    }
}
static void BuildSettingsPage(HWND hwnd,int page) {
    g_settingsPage=page;g_settingsScroll=0;
    for(int id=SETTINGS_FIELD_BASE;id<SETTINGS_FIELD_BASE+200;++id){HWND c=GetDlgItem(hwnd,id);if(c)DestroyWindow(c);}
    ShowWindow(GetDlgItem(hwnd,101),page==0?SW_SHOW:SW_HIDE);
    ShowWindow(GetDlgItem(hwnd,102),page==0?SW_SHOW:SW_HIDE);
    ShowWindow(GetDlgItem(hwnd,105),page==0?SW_SHOW:SW_HIDE);
    ShowWindow(g_settingsScrollBar,page==0?SW_HIDE:SW_SHOW);
    LoadSettingsPageItems(page);
    g_settingsMaxScroll=(int)g_settingItems.size()*27-340;if(g_settingsMaxScroll<0)g_settingsMaxScroll=0;
    if(g_settingsScrollBar){SCROLLINFO si={sizeof(si),SIF_RANGE|SIF_PAGE|SIF_POS,0,g_settingsMaxScroll+339,340,0};SetScrollInfo(g_settingsScrollBar,SB_CTL,&si,TRUE);}
    for(size_t i=0;i<g_settingItems.size();++i) {
        int y=28+(int)i*27-g_settingsScroll;
        SettingsCreateLabel(hwnd,g_settingItems[i].label,155,y+4,145,21,SETTINGS_FIELD_BASE+100+(int)i);
        std::basic_string<TCHAR> draft=GetDraftSettingValue(g_settingItems[i].section,g_settingItems[i].key,_T(""));
        TCHAR value[128]={};lstrcpyn(value,draft.c_str(),128);
        HWND edit=CreateWindowEx(WS_EX_CLIENTEDGE,_T("EDIT"),value,WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,305,y,190,22,hwnd,(HMENU)(INT_PTR)(SETTINGS_FIELD_BASE+(int)i),NULL,NULL);
        if(g_settingsFont)SendMessage(edit,WM_SETFONT,(WPARAM)g_settingsFont,TRUE);
        if(g_settingItems[i].color) {
            HWND swatch=CreateWindowEx(0,_T("BUTTON"),_T(""),WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,500,y,26,22,hwnd,(HMENU)(INT_PTR)(SETTINGS_FIELD_BASE+50+(int)i),NULL,NULL);
            if(g_settingsFont)SendMessage(swatch,WM_SETFONT,(WPARAM)g_settingsFont,TRUE);
        }
    }
    UpdateSettingsControlVisibility(hwnd);
    InvalidateRect(hwnd,NULL,TRUE);
}
static void SaveSettingsPage(HWND hwnd) {
    for(size_t i=0;i<g_settingItems.size();++i) {
        TCHAR value[128]={};HWND edit=GetDlgItem(hwnd,SETTINGS_FIELD_BASE+(int)i);
        if(!edit)continue;
        GetWindowText(edit,value,128);
        SetDraftSettingValue(g_settingItems[i].section,g_settingItems[i].key,value);
    }
}
static void ChooseSettingColor(HWND hwnd,int index) {
    if(index<0||(size_t)index>=g_settingItems.size()||!g_settingItems[index].color)return;
    HWND edit=GetDlgItem(hwnd,SETTINGS_FIELD_BASE+index);if(!edit)return;
    TCHAR value[128]={};GetWindowText(edit,value,128);
    int r=0,g=0,b=0;_stscanf_s(value,_T("%d , %d , %d"),&r,&g,&b);
    COLORREF custom[16]={};CHOOSECOLOR cc={sizeof(cc)};cc.hwndOwner=hwnd;cc.rgbResult=RGB(r,g,b);cc.lpCustColors=custom;cc.Flags=CC_FULLOPEN|CC_RGBINIT;
    if(ChooseColor(&cc)){_stprintf_s(value,_T("%d, %d, %d"),GetRValue(cc.rgbResult),GetGValue(cc.rgbResult),GetBValue(cc.rgbResult));SetWindowText(edit,value);HWND swatch=GetDlgItem(hwnd,SETTINGS_FIELD_BASE+50+index);if(swatch)InvalidateRect(swatch,NULL,TRUE);}
}
static void ApplyAllSettings(HWND hwnd,bool commit) {
    SaveSettingsPage(hwnd);
    g_useMidiMapper=SendMessage(g_mapperCheck,BM_GETCHECK,0,0)==BST_CHECKED;
    if(!g_useMidiMapper){int row=(int)SendMessage(g_outputCombo,CB_GETCURSEL,0,0);if(row>=0)g_selectedMidiDevice=(UINT)SendMessage(g_outputCombo,CB_GETITEMDATA,row,0);}
    WriteSettingsValues(g_settingsDraft);
    OpenMidiOutput(hwnd);
    LoadColorPalette();LoadInterfaceColors();LoadInstrumentDisplaySettings();LoadKeyboardSettings();
    if(commit)SaveMidiOutputSettings();
    else WriteSettingsValues(g_settingsOriginal);
    DestroyBackBuffer();HWND owner=GetParent(hwnd);if(owner)InvalidateRect(owner,NULL,FALSE);
}
static void CancelSettings(HWND hwnd) {
    WriteSettingsValues(g_settingsOriginal);
    g_useMidiMapper=g_settingsOriginalUseMapper;g_selectedMidiDevice=g_settingsOriginalDevice;
    OpenMidiOutput(hwnd);
    LoadColorPalette();LoadInterfaceColors();LoadInstrumentDisplaySettings();LoadKeyboardSettings();
    DestroyBackBuffer();HWND owner=GetParent(hwnd);if(owner)InvalidateRect(owner,NULL,FALSE);
    ShowWindow(hwnd,SW_HIDE);
}
static LRESULT CALLBACK SettingsWindowProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) {
    switch(msg) {
    case WM_CREATE: {
        g_settingsFont=CreateFont(13,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_DONTCARE,_T("Segoe UI"));
        INITCOMMONCONTROLSEX icc={sizeof(icc),ICC_TREEVIEW_CLASSES};InitCommonControlsEx(&icc);
        g_settingsTree=CreateWindowEx(WS_EX_CLIENTEDGE,WC_TREEVIEW,_T(""),WS_CHILD|WS_VISIBLE|TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS|TVS_SHOWSELALWAYS,10,18,130,365,hwnd,(HMENU)110,NULL,NULL);
        if(g_settingsFont)SendMessage(g_settingsTree,WM_SETFONT,(WPARAM)g_settingsFont,TRUE);
        SendMessage(g_settingsTree,TVM_SETBKCOLOR,0,(LPARAM)GetSysColor(COLOR_3DFACE));
        SendMessage(g_settingsTree,TVM_SETTEXTCOLOR,0,(LPARAM)GetSysColor(COLOR_WINDOWTEXT));
        for(int i=0;i<7;++i) {
            TVINSERTSTRUCT t={};t.hParent=TVI_ROOT;t.hInsertAfter=TVI_LAST;t.item.mask=TVIF_TEXT|TVIF_PARAM;
            t.item.pszText=(LPTSTR)g_settingsPageNames[i];t.item.lParam=i;
            HTREEITEM item=(HTREEITEM)SendMessage(g_settingsTree,TVM_INSERTITEM,0,(LPARAM)&t);
            if(i==0)SendMessage(g_settingsTree,TVM_SELECTITEM,TVGN_CARET,(LPARAM)item);
        }
        g_mapperCheck=CreateWindowEx(0,_T("BUTTON"),_T("Use MIDI Mapper"),WS_CHILD|BS_AUTOCHECKBOX,145,62,170,23,hwnd,(HMENU)101,NULL,NULL);
        g_outputCombo=CreateWindowEx(WS_EX_CLIENTEDGE,_T("COMBOBOX"),_T(""),WS_CHILD|WS_VSCROLL|CBS_DROPDOWNLIST,305,55,221,160,hwnd,(HMENU)102,NULL,NULL);
        HWND midiLabel=CreateWindowEx(0,_T("STATIC"),_T("Output device"),WS_CHILD,305,32,200,20,hwnd,(HMENU)105,NULL,NULL);
        g_settingsScrollBar=CreateWindowEx(0,_T("SCROLLBAR"),_T(""),WS_CHILD|SBS_VERT,535,18,16,365,hwnd,(HMENU)106,NULL,NULL);
        HWND ok=CreateWindowEx(0,_T("BUTTON"),_T("OK"),WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,300,393,68,27,hwnd,(HMENU)103,NULL,NULL);
        HWND cancel=CreateWindowEx(0,_T("BUTTON"),_T("Cancel"),WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,375,393,68,27,hwnd,(HMENU)104,NULL,NULL);
        HWND apply=CreateWindowEx(0,_T("BUTTON"),_T("Apply"),WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,450,393,68,27,hwnd,(HMENU)107,NULL,NULL);
        if(g_settingsFont){SendMessage(midiLabel,WM_SETFONT,(WPARAM)g_settingsFont,TRUE);SendMessage(g_mapperCheck,WM_SETFONT,(WPARAM)g_settingsFont,TRUE);SendMessage(g_outputCombo,WM_SETFONT,(WPARAM)g_settingsFont,TRUE);SendMessage(ok,WM_SETFONT,(WPARAM)g_settingsFont,TRUE);SendMessage(cancel,WM_SETFONT,(WPARAM)g_settingsFont,TRUE);SendMessage(apply,WM_SETFONT,(WPARAM)g_settingsFont,TRUE);}
        SendMessage(g_mapperCheck,BM_SETCHECK,g_useMidiMapper?BST_CHECKED:BST_UNCHECKED,0);RefreshOutputCombo();
        BuildSettingsPage(hwnd,0);
        return 0;
    }
    case WM_NOTIFY: {
        NMHDR* hdr=(NMHDR*)lp;
        if(hdr&&hdr->idFrom==110&&hdr->code==TVN_SELCHANGED) {
            NMTREEVIEW* tv=(NMTREEVIEW*)lp;SaveSettingsPage(hwnd);BuildSettingsPage(hwnd,(int)tv->itemNew.lParam);return 0;
        }
        break;
    }
    case WM_VSCROLL: {
        if((HWND)lp==g_settingsScrollBar) {
            SCROLLINFO si={sizeof(si),SIF_TRACKPOS};GetScrollInfo(g_settingsScrollBar,SB_CTL,&si);
            int pos=g_settingsScroll;
            switch(LOWORD(wp)){case SB_LINEUP:pos-=27;break;case SB_LINEDOWN:pos+=27;break;case SB_PAGEUP:pos-=270;break;case SB_PAGEDOWN:pos+=270;break;case SB_THUMBTRACK:case SB_THUMBPOSITION:pos=si.nTrackPos;break;case SB_TOP:pos=0;break;case SB_BOTTOM:pos=g_settingsMaxScroll;break;}
            if(pos<0)pos=0;if(pos>g_settingsMaxScroll)pos=g_settingsMaxScroll;
            int delta=g_settingsScroll-pos;g_settingsScroll=pos;
            for(int id=SETTINGS_FIELD_BASE;id<SETTINGS_FIELD_BASE+200;++id){HWND c=GetDlgItem(hwnd,id);if(c){RECT r;GetWindowRect(c,&r);MapWindowPoints(NULL,hwnd,(POINT*)&r,2);SetWindowPos(c,NULL,r.left,r.top+delta,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);}}UpdateSettingsControlVisibility(hwnd);
            SCROLLINFO out={sizeof(out),SIF_POS};out.nPos=pos;SetScrollInfo(g_settingsScrollBar,SB_CTL,&out,TRUE);return 0;
        }
        break;
    }
    case WM_MOUSEWHEEL: {
        POINT pt={GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};ScreenToClient(hwnd,&pt);
        if(pt.x>=145&&pt.x<555&&g_settingsPage!=0){int pos=g_settingsScroll-(GET_WHEEL_DELTA_WPARAM(wp)/WHEEL_DELTA)*54;if(pos<0)pos=0;if(pos>g_settingsMaxScroll)pos=g_settingsMaxScroll;int delta=g_settingsScroll-pos;g_settingsScroll=pos;for(int id=SETTINGS_FIELD_BASE;id<SETTINGS_FIELD_BASE+200;++id){HWND c=GetDlgItem(hwnd,id);if(c){RECT r;GetWindowRect(c,&r);MapWindowPoints(NULL,hwnd,(POINT*)&r,2);SetWindowPos(c,NULL,r.left,r.top+delta,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);}}UpdateSettingsControlVisibility(hwnd);SCROLLINFO si={sizeof(si),SIF_POS};si.nPos=pos;SetScrollInfo(g_settingsScrollBar,SB_CTL,&si,TRUE);return 0;}
        break;
    }
    case WM_COMMAND: {
        int id=LOWORD(wp);
        if(id>=SETTINGS_FIELD_BASE+50&&id<SETTINGS_FIELD_BASE+100){ChooseSettingColor(hwnd,id-SETTINGS_FIELD_BASE-50);return 0;}
        if(id==101&&HIWORD(wp)==BN_CLICKED){EnableWindow(g_outputCombo,SendMessage(g_mapperCheck,BM_GETCHECK,0,0)!=BST_CHECKED);return 0;}
        if(id==103){ApplyAllSettings(hwnd,true);ShowWindow(hwnd,SW_HIDE);return 0;}
        if(id==104){CancelSettings(hwnd);return 0;}
        if(id==107){ApplyAllSettings(hwnd,false);return 0;}
        break;
    }
    case WM_DRAWITEM: {
        DRAWITEMSTRUCT* dis=(DRAWITEMSTRUCT*)lp;
        if(dis && dis->CtlID>=SETTINGS_FIELD_BASE+50 && dis->CtlID<SETTINGS_FIELD_BASE+100) {
            int index=(int)dis->CtlID-SETTINGS_FIELD_BASE-50;
            COLORREF color=RGB(128,128,128);
            if(index>=0&&(size_t)index<g_settingItems.size()) {
                HWND edit=GetDlgItem(hwnd,SETTINGS_FIELD_BASE+index);TCHAR value[128]={};
                if(edit)GetWindowText(edit,value,128);
                int r=128,g=128,b=128;if(_stscanf_s(value,_T("%d , %d , %d"),&r,&g,&b)==3)color=RGB(r,g,b);
            }
            FillRect(dis->hDC,&dis->rcItem,GetSysColorBrush(COLOR_3DFACE));
            RECT sample=dis->rcItem;InflateRect(&sample,-4,-4);
            HBRUSH brush=CreateSolidBrush(color);FillRect(dis->hDC,&sample,brush);DeleteObject(brush);
            FrameRect(dis->hDC,&sample,(HBRUSH)GetStockObject(GRAY_BRUSH));
            if(dis->itemState&ODS_FOCUS)DrawFocusRect(dis->hDC,&dis->rcItem);
            return TRUE;
        }
        break;
    }
    case WM_CTLCOLORSTATIC: {
        HDC dc=(HDC)wp;SetTextColor(dc,GetSysColor(COLOR_WINDOWTEXT));SetBkColor(dc,GetSysColor(COLOR_3DFACE));SetBkMode(dc,TRANSPARENT);return (LRESULT)GetSysColorBrush(COLOR_3DFACE);
    }
    case WM_CTLCOLORBTN: {
        HDC dc=(HDC)wp;SetTextColor(dc,GetSysColor(COLOR_BTNTEXT));SetBkColor(dc,GetSysColor(COLOR_3DFACE));return (LRESULT)GetSysColorBrush(COLOR_3DFACE);
    }
    case WM_CTLCOLOREDIT: {
        HDC dc=(HDC)wp;SetTextColor(dc,GetSysColor(COLOR_WINDOWTEXT));SetBkColor(dc,GetSysColor(COLOR_WINDOW));return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
    }
    case WM_ERASEBKGND: {
        HDC dc=(HDC)wp;RECT r;GetClientRect(hwnd,&r);FillRect(dc,&r,GetSysColorBrush(COLOR_3DFACE));return 1;
    }
    case WM_CLOSE:CancelSettings(hwnd);return 0;
    case WM_DESTROY:if(g_settingsFont){DeleteObject(g_settingsFont);g_settingsFont=NULL;}return 0;
    }
    return DefWindowProc(hwnd,msg,wp,lp);
}
static void ShowMidiOutputSettings(HWND owner) {
    if(g_settingsWnd && IsWindowVisible(g_settingsWnd)){CancelSettings(g_settingsWnd);return;}
    if(!g_settingsWnd)g_settingsWnd=CreateWindowEx(WS_EX_TOOLWINDOW,_T("MIDI_PIXEL_SETTINGS_WINDOW"),_T("MIDI PIXEL - Settings"),WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_BORDER,CW_USEDEFAULT,CW_USEDEFAULT,570,470,owner,NULL,(HINSTANCE)GetWindowLongPtr(owner,GWLP_HINSTANCE),NULL);
    if(g_settingsWnd){BeginSettingsTransaction();RECT wr;GetWindowRect(owner,&wr);SetWindowPos(g_settingsWnd,HWND_TOP,wr.left+25,wr.top+25,570,470,SWP_SHOWWINDOW);SendMessage(g_mapperCheck,BM_SETCHECK,g_useMidiMapper?BST_CHECKED:BST_UNCHECKED,0);RefreshOutputCombo();EnableWindow(g_outputCombo,!g_useMidiMapper);BuildSettingsPage(g_settingsWnd,g_settingsPage);ShowWindow(g_settingsWnd,SW_SHOW);SetFocus(g_settingsTree);UpdateWindow(g_settingsWnd);}
}

static void BuildSideFilePath(const TCHAR* fileName,TCHAR* path,DWORD size) {
 lstrcpyn(path,g_iniPath,(int)size);TCHAR* slash=_tcsrchr(path,_T('\\\\'));if(slash)*(slash+1)=0;_tcsncat_s(path,size,_T("infos\\\\"),_TRUNCATE);_tcsncat_s(path,size,fileName,_TRUNCATE);
}
static bool IsValidUtf8(const unsigned char* s,size_t n) {
 size_t i=0;
 while(i<n) {
  unsigned char c=s[i++];
  if(c<=0x7f)continue;
  if(c>=0xc2&&c<=0xdf) {
   if(i>=n||(s[i]&0xc0)!=0x80)return false;
   ++i;continue;
  }
  if(c>=0xe0&&c<=0xef) {
   if(i+1>=n)return false;
   unsigned char c1=s[i],c2=s[i+1];
   if((c1&0xc0)!=0x80||(c2&0xc0)!=0x80)return false;
   if(c==0xe0&&c1<0xa0)return false;
   if(c==0xed&&c1>=0xa0)return false;
   i+=2;continue;
  }
  if(c>=0xf0&&c<=0xf4) {
   if(i+2>=n)return false;
   unsigned char c1=s[i],c2=s[i+1],c3=s[i+2];
   if((c1&0xc0)!=0x80||(c2&0xc0)!=0x80||(c3&0xc0)!=0x80)return false;
   if(c==0xf0&&c1<0x90)return false;
   if(c==0xf4&&c1>=0x90)return false;
   i+=3;continue;
  }
  return false;
 }
 return true;
}
static void LoadSideText(const TCHAR* fileName,int resourceId) {
 g_infoText.clear();TCHAR path[MAX_PATH]={};BuildSideFilePath(fileName,path,MAX_PATH);FILE* f=NULL;
 std::vector<char> bytes;size_t got=0;
 if(_tfopen_s(&f,path,_T("rb"))==0&&f) {
  fseek(f,0,SEEK_END);long size=ftell(f);fseek(f,0,SEEK_SET);
  if(size>0&&size<=2*1024*1024){bytes.assign((size_t)size+1,0);got=fread(bytes.data(),1,(size_t)size,f);}
  fclose(f);
 } else {
  HRSRC resource=FindResource(NULL,MAKEINTRESOURCE(resourceId),RT_RCDATA);
  if(resource) {
   HGLOBAL loaded=LoadResource(NULL,resource);DWORD size=SizeofResource(NULL,resource);
   const char* data=loaded?(const char*)LockResource(loaded):NULL;
   if(data&&size>0&&size<=2*1024*1024){bytes.assign(data,data+size);bytes.push_back(0);got=size;}
  }
 }
 if(!got)return;
 size_t offset=(got>=3&&(unsigned char)bytes[0]==0xef&&(unsigned char)bytes[1]==0xbb&&(unsigned char)bytes[2]==0xbf)?3:0;
 const char* textBytes=bytes.data()+offset;size_t textSize=got-offset;
 UINT cp=IsValidUtf8((const unsigned char*)textBytes,textSize)?CP_UTF8:CP_ACP;
 if(textSize>(size_t)INT_MAX)return;
#ifdef UNICODE
 int n=MultiByteToWideChar(cp,0,textBytes,(int)textSize,NULL,0);
 if(n>0){g_infoText.resize((size_t)n+1);MultiByteToWideChar(cp,0,textBytes,(int)textSize,g_infoText.data(),n);g_infoText[n]=0;}
#else
 int wn=MultiByteToWideChar(cp,0,textBytes,(int)textSize,NULL,0);
 if(wn>0){
  std::vector<WCHAR>w((size_t)wn+1,0);
  MultiByteToWideChar(cp,0,textBytes,(int)textSize,w.data(),wn);
  int n=WideCharToMultiByte(CP_ACP,0,w.data(),wn,NULL,0,NULL,NULL);
  if(n>0){g_infoText.resize((size_t)n+1);WideCharToMultiByte(CP_ACP,0,w.data(),wn,g_infoText.data(),n,NULL,NULL);g_infoText[n]=0;}
 }
#endif
}
static UINT AuthorsGifFrameDelay(UINT index) {
    if(index<g_gifFrameDelaysMs.size())return g_gifFrameDelaysMs[index];
    return 100;
}
static void CloseAuthorsGif(){if(g_infoWnd)KillTimer(g_infoWnd,2);delete g_authorsGif;g_authorsGif=NULL;if(g_authorsGifStream){g_authorsGifStream->Release();g_authorsGifStream=NULL;}g_gifFrameCount=0;g_gifFrameIndex=0;g_gifFrameDelaysMs.clear();}
static void LoadAuthorsGif(HWND hwnd) {
 CloseAuthorsGif();TCHAR path[MAX_PATH]={};BuildSideFilePath(_T("auteurs.GIF"),path,MAX_PATH);
#ifdef UNICODE
 const WCHAR* wp=path;
#else
 WCHAR wp[MAX_PATH]={};MultiByteToWideChar(CP_ACP,0,path,-1,wp,MAX_PATH);
#endif
 DWORD attr=GetFileAttributes(path);
  if(attr!=INVALID_FILE_ATTRIBUTES && !(attr&FILE_ATTRIBUTE_DIRECTORY)) {
   g_authorsGif=Gdiplus::Image::FromFile(wp,FALSE);
  } else {
   HRSRC resource=FindResource(NULL,MAKEINTRESOURCE(IDR_AUTHORS_GIF),RT_RCDATA);
   if(resource) {
    HGLOBAL loaded=LoadResource(NULL,resource);DWORD size=SizeofResource(NULL,resource);
    const void* data=loaded?LockResource(loaded):NULL;
    if(data&&size) {
     HGLOBAL memory=GlobalAlloc(GMEM_MOVEABLE,size);
     if(memory) {
      void* dest=GlobalLock(memory);
      if(dest){memcpy(dest,data,size);GlobalUnlock(memory);if(CreateStreamOnHGlobal(memory,TRUE,&g_authorsGifStream)==S_OK)g_authorsGif=new Gdiplus::Bitmap(g_authorsGifStream,FALSE);else GlobalFree(memory);}
      else GlobalFree(memory);
     }
    }
   }
  }
  if(!g_authorsGif||g_authorsGif->GetLastStatus()!=Gdiplus::Ok){delete g_authorsGif;g_authorsGif=NULL;if(g_authorsGifStream){g_authorsGifStream->Release();g_authorsGifStream=NULL;}return;}
 UINT n=g_authorsGif->GetFrameDimensionsCount();if(!n)return;std::vector<GUID> ids(n);g_authorsGif->GetFrameDimensionsList(ids.data(),n);
 for(UINT i=0;i<n;++i)if(ids[i]==Gdiplus::FrameDimensionTime){g_gifFrameDimension=ids[i];break;}
 g_gifFrameCount=g_authorsGif->GetFrameCount(&g_gifFrameDimension);
  g_gifFrameDelaysMs.assign(g_gifFrameCount,100);
  UINT propertySize=g_authorsGif->GetPropertyItemSize(0x5100);
  if(propertySize>0) {
   std::vector<BYTE> propertyBuffer(propertySize);
   if(g_authorsGif->GetPropertyItem(0x5100,propertySize,(Gdiplus::PropertyItem*)propertyBuffer.data())==Gdiplus::Ok) {
    Gdiplus::PropertyItem* item=(Gdiplus::PropertyItem*)propertyBuffer.data();
    ULONG* delays=(ULONG*)item->value;
    UINT count=item->length/(UINT)sizeof(ULONG);
    for(UINT i=0;i<g_gifFrameCount&&i<count;++i) {
     UINT delayMs=(UINT)delays[i]*10;
     if(delayMs<10)delayMs=10;
     g_gifFrameDelaysMs[i]=delayMs;
    }
   }
  }
  if(g_gifFrameCount>1)SetTimer(hwnd,2,AuthorsGifFrameDelay(0),NULL);
}
static void ShowSideContent(HWND owner,int content) {
  if(g_infoWnd && IsWindowVisible(g_infoWnd) && g_infoContent==content){CloseAuthorsGif();ShowWindow(g_infoWnd,SW_HIDE);return;}
  if(!g_infoWnd){RECT wr;GetWindowRect(owner,&wr);int ix=wr.right;if(ix+g_infoWindowWidth>GetSystemMetrics(SM_CXSCREEN))ix=wr.left-g_infoWindowWidth;
  g_infoWnd=CreateWindowEx(WS_EX_TOOLWINDOW,_T("MIDI_PIXEL_INFO_WINDOW"),_T(""),WS_POPUP|WS_BORDER,ix,wr.top,g_infoWindowWidth,wr.bottom-wr.top,owner,NULL,(HINSTANCE)GetWindowLongPtr(owner,GWLP_HINSTANCE),NULL);}
 if(!g_infoWnd)return;CloseAuthorsGif();g_infoContent=content;g_infoScroll=0;
 if(content==1)LoadSideText(_T("MIDIPIXELS.TXT"),IDR_MIDIPIXELS_TEXT);else if(content==2)LoadSideText(_T("history.TXT"),IDR_HISTORY_TEXT);else if(content==3){
  LoadAuthorsGif(g_infoWnd);LoadSideText(_T("auteurs.txt"),IDR_AUTHORS_TEXT);
  if(g_infoText.empty()) {
   const TCHAR fallback[]=
    _T("MIDIPIXELS\r\n")
    _T("Created through vibe-coding by Claude-Samuel Levine, late November 2026, in Etretat.\r\n")
    _T("Website: www.cslevine.com\r\n\r\n")
    _T("C++ source generated by ChatGPT (Lia).");
   g_infoText.assign(fallback,fallback+_tcslen(fallback));
  }
 }
 InvalidateRect(g_infoWnd,NULL,TRUE);ShowWindow(g_infoWnd,SW_SHOWNOACTIVATE);UpdateWindow(g_infoWnd);
}

static void DrawJustifiedLine(HDC dc,const TCHAR* line,int length,const RECT& area,int y,bool justify) {
    if(length<=0)return;
    SIZE extent={};GetTextExtentPoint32(dc,line,length,&extent);
    int spaces=0;for(int i=0;i<length;++i)if(line[i]==_T(' '))++spaces;
    if(justify&&spaces>0&&extent.cx<area.right-area.left)
        SetTextJustification(dc,(area.right-area.left)-extent.cx,spaces);
    else SetTextJustification(dc,0,0);
    TextOut(dc,area.left,y,line,length);
    SetTextJustification(dc,0,0);
}
static int DrawJustifiedText(HDC dc,const TCHAR* text,const RECT& area,int lineHeight) {
    if(!text||!lineHeight)return 0;
    TEXTMETRIC tm={};GetTextMetrics(dc,&tm);
    int charWidth=tm.tmAveCharWidth>0?tm.tmAveCharWidth:8;
    int y=area.top;
    const TCHAR* p=text;
    while(*p&&y<area.bottom) {
        const TCHAR* paragraphStart=p;
        while(*p&&*p!=_T('\r')&&*p!=_T('\n'))++p;
        std::basic_string<TCHAR> paragraph(paragraphStart,p);
        while(!paragraph.empty()&&(paragraph.back()==_T(' ')||paragraph.back()==_T('\t')))paragraph.pop_back();
        size_t first=0;
        int indentColumns=0;
        while(first<paragraph.size()&&(paragraph[first]==_T(' ')||paragraph[first]==_T('\t'))) {
            if(paragraph[first]==_T('\t'))indentColumns=((indentColumns/4)+1)*4;
            else ++indentColumns;
            ++first;
        }
        if(first==paragraph.size()) {
            y+=lineHeight+3;
        } else {
            RECT paragraphArea=area;
            paragraphArea.left+=indentColumns*charWidth;
            if(paragraphArea.left>=paragraphArea.right-charWidth)paragraphArea.left=paragraphArea.right-charWidth;
            std::vector<std::basic_string<TCHAR>> words;
            size_t pos=first;
            while(pos<paragraph.size()) {
                while(pos<paragraph.size()&&(paragraph[pos]==_T(' ')||paragraph[pos]==_T('\t')))++pos;
                size_t wordStart=pos;
                while(pos<paragraph.size()&&paragraph[pos]!=_T(' ')&&paragraph[pos]!=_T('\t'))++pos;
                if(pos>wordStart)words.push_back(paragraph.substr(wordStart,pos-wordStart));
            }
            std::basic_string<TCHAR> line;
            for(size_t i=0;i<words.size();++i) {
                std::basic_string<TCHAR> candidate=line.empty()?words[i]:line+_T(" ")+words[i];
                SIZE extent={};GetTextExtentPoint32(dc,candidate.c_str(),(int)candidate.size(),&extent);
                if(!line.empty()&&extent.cx>paragraphArea.right-paragraphArea.left) {
                    DrawJustifiedLine(dc,line.c_str(),(int)line.size(),paragraphArea,y,true);
                    y+=lineHeight;line=words[i];
                } else line.swap(candidate);
            }
            if(!line.empty()&&y<area.bottom) {
                DrawJustifiedLine(dc,line.c_str(),(int)line.size(),paragraphArea,y,false);
                y+=lineHeight+3;
            }
        }
        if(*p==_T('\r')){++p;if(*p==_T('\n'))++p;}
        else if(*p==_T('\n'))++p;
    }
    return y-area.top;
}
static HFONT CreateDocumentFont(int size) {
    return CreateFont(size,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,FIXED_PITCH|FF_MODERN,_T("Courier New"));
}
static LRESULT CALLBACK InfoWindowProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) {
    switch(msg) {
    case WM_ERASEBKGND: return 1;
    case WM_TIMER: {if(wp==2&&g_authorsGif&&g_gifFrameCount>1){g_gifFrameIndex=(g_gifFrameIndex+1)%g_gifFrameCount;/* Keep GDI+ GIF frame composition/disposal semantics: transparent pixels may reveal preceding frames. */g_authorsGif->SelectActiveFrame(&g_gifFrameDimension,g_gifFrameIndex);SetTimer(hwnd,2,AuthorsGifFrameDelay(g_gifFrameIndex),NULL);RECT imageRect;GetClientRect(hwnd,&imageRect);imageRect.left=10;imageRect.top=43;imageRect.bottom=43+(imageRect.bottom-55)*2/5;imageRect.right-=10;InvalidateRect(hwnd,&imageRect,FALSE);return 0;}break;}
    case WM_MOUSEWHEEL: {
        RECT client; GetClientRect(hwnd,&client);
        int maxScroll=(int)(g_infoText.size()*18)-(client.bottom-48);
        if(g_infoContent==0) {
            size_t chars=0;
            for(size_t i=0;i<g_midiText.size();++i)
                if(g_midiText[i].label=="Text"||g_midiText[i].label=="Lyric"||g_midiText[i].label=="Cue")chars+=g_midiText[i].text.size();
            maxScroll=(int)(chars/38)*14+100-(client.bottom-48);
        }
        if(maxScroll<0)maxScroll=0;
        g_infoScroll-=GET_WHEEL_DELTA_WPARAM(wp)/WHEEL_DELTA*57;
        if(g_infoScroll<0)g_infoScroll=0;if(g_infoScroll>maxScroll)g_infoScroll=maxScroll;
        if(g_infoContent==3) {
            RECT textRect;GetClientRect(hwnd,&textRect);
            textRect.top=43+(textRect.bottom-55)*2/5+14;
            textRect.left=14;textRect.right-=14;textRect.bottom-=12;
            InvalidateRect(hwnd,&textRect,FALSE);
        } else InvalidateRect(hwnd,NULL,FALSE);
        return 0;
    }
    case WM_LBUTTONDOWN: {
        RECT r; GetClientRect(hwnd,&r);
        if(LOWORD(lp)>=r.right-38 && HIWORD(lp)<30){CloseAuthorsGif();ShowWindow(hwnd,SW_HIDE);return 0;}
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc=BeginPaint(hwnd,&ps); RECT r; GetClientRect(hwnd,&r);
        FillRectColor(dc,r,g_uiBackground);
        const TCHAR* panelTitle=g_infoContent==1?_T("MIDIPIXELS"):g_infoContent==2?_T("HISTORY"):g_infoContent==3?_T("AUTHORS"):_T("MIDI INFO");
        Txt(dc,14,10,panelTitle,RGB(235,235,242),12,FW_SEMIBOLD);
        Txt(dc,r.right-25,8,_T("X"),RGB(235,235,242),14,FW_BOLD);
        HPEN pen=CreatePen(PS_SOLID,1,g_uiLines); HPEN old=(HPEN)SelectObject(dc,pen);
        MoveToEx(dc,10,31,NULL); LineTo(dc,r.right-10,31); SelectObject(dc,old); DeleteObject(pen);
        if(g_infoContent==3){
            int gifBottom=43+(r.bottom-55)*2/5;
            RECT imageRect={10,43,r.right-10,gifBottom};
            RECT textRect={14,gifBottom+14,r.right-14,r.bottom-12};
            if(g_authorsGif){
                Gdiplus::Graphics gr(dc);gr.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
                UINT iw=g_authorsGif->GetWidth(),ih=g_authorsGif->GetHeight();
                if(iw&&ih&&imageRect.right>imageRect.left&&imageRect.bottom>imageRect.top){
                    double z=(double)(imageRect.right-imageRect.left-8)/iw;
                    double zh=(double)(imageRect.bottom-imageRect.top-8)/ih;
                    if(zh<z)z=zh;
                    int dw=(int)(iw*z),dh=(int)(ih*z);
                    gr.DrawImage(g_authorsGif,(r.right-dw)/2,(imageRect.top+imageRect.bottom-dh)/2,dw,dh);
                }
            } else Txt(dc,14,48,_T("Place auteurs.GIF in the infos folder"),RGB(205,210,225),10);
            HPEN ap=CreatePen(PS_SOLID,1,g_uiLines);HPEN ao=(HPEN)SelectObject(dc,ap);
            MoveToEx(dc,10,gifBottom,NULL);LineTo(dc,r.right-10,gifBottom);
            SelectObject(dc,ao);DeleteObject(ap);
            if(g_infoText.empty())Txt(dc,14,textRect.top,_T("auteurs.txt not found in the infos folder."),RGB(205,210,225),10);
            else {
                RECT tr=textRect;tr.top-=g_infoScroll;
                SetTextColor(dc,RGB(225,225,232));SetBkMode(dc,TRANSPARENT);
                int clipState=SaveDC(dc);
                IntersectClipRect(dc,textRect.left,textRect.top,textRect.right,textRect.bottom);
                HFONT authorFont=CreateDocumentFont(g_textesFontSize);
                HFONT oldAuthorFont=(HFONT)SelectObject(dc,authorFont);
                TEXTMETRIC tm={};GetTextMetrics(dc,&tm);
                DrawJustifiedText(dc,g_infoText.data(),tr,tm.tmHeight+2);
                SelectObject(dc,oldAuthorFont);DeleteObject(authorFont);
                RestoreDC(dc,clipState);
            }
            EndPaint(hwnd,&ps);return 0;
        }
        if(g_infoContent==1||g_infoContent==2){
            if(g_infoText.empty())Txt(dc,14,48,_T("Text file not found in the infos folder."),RGB(205,210,225),10);
            else {
                RECT tr={14,43-g_infoScroll,r.right-14,r.bottom-12};
                SetTextColor(dc,RGB(225,225,232));SetBkMode(dc,TRANSPARENT);
                int clipState=SaveDC(dc);IntersectClipRect(dc,14,43,r.right-14,r.bottom-12);
                HFONT docFont=CreateDocumentFont(g_textesFontSize);HFONT oldDocFont=(HFONT)SelectObject(dc,docFont);
                TEXTMETRIC tm={};GetTextMetrics(dc,&tm);
                DrawJustifiedText(dc,g_infoText.data(),tr,tm.tmHeight+2);
                SelectObject(dc,oldDocFont);DeleteObject(docFont);RestoreDC(dc,clipState);
            }
            EndPaint(hwnd,&ps);return 0;
        }
        int y=43;
        const char* labels[3]={"Text","Lyric","Cue"};
        const TCHAR* headings[3]={_T("TEXT"),_T("0x05  LYRICS"),_T("0x07  CUE POINT")};
        for(int section=0;section<3;++section) {
            bool hasItems=false;
            for(size_t i=0;i<g_midiText.size();++i)if(g_midiText[i].label==labels[section]){hasItems=true;break;}
            if(!hasItems)continue;
            Txt(dc,14,y-g_infoScroll,headings[section],RGB(160,170,195),9,FW_BOLD); y+=18;
            for(size_t i=0;i<g_midiText.size();++i)if(g_midiText[i].label==labels[section]) {
                TCHAR buf[4096]={};
#ifdef UNICODE
                MultiByteToWideChar(CP_ACP,0,g_midiText[i].text.c_str(),-1,buf,4096);
#else
                lstrcpynA(buf,g_midiText[i].text.c_str(),4096);
#endif
                RECT draw={14,y-g_infoScroll,r.right-14,r.bottom-12};
                SetTextColor(dc,RGB(225,225,232));SetBkMode(dc,TRANSPARENT);
                HFONT infoFont=CreateDocumentFont(g_textesFontSize);HFONT oldInfoFont=(HFONT)SelectObject(dc,infoFont);
                TEXTMETRIC tm={};GetTextMetrics(dc,&tm);
                int clipState=SaveDC(dc);IntersectClipRect(dc,14,43,r.right-14,r.bottom-12);
                int used=DrawJustifiedText(dc,buf,draw,tm.tmHeight+2);
                RestoreDC(dc,clipState);SelectObject(dc,oldInfoFont);DeleteObject(infoFont);
                y+=used+7;
            }
            y+=5;
        }
        EndPaint(hwnd,&ps); return 0;
    }
    case WM_CLOSE: CloseAuthorsGif();ShowWindow(hwnd,SW_HIDE); return 0;
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
            else {RECT wr;GetWindowRect(hwnd,&wr);int ix=wr.right;if(ix+g_infoWindowWidth>GetSystemMetrics(SM_CXSCREEN))ix=wr.left-g_infoWindowWidth;SetWindowPos(g_infoWnd,NULL,ix,wr.top,g_infoWindowWidth,wr.bottom-wr.top,SWP_NOZORDER|SWP_NOACTIVATE);}
        }
        return 0;
    case WM_MOVE:
        if(g_infoWnd && IsWindowVisible(g_infoWnd)){RECT wr;GetWindowRect(hwnd,&wr);int ix=wr.right;if(ix+g_infoWindowWidth>GetSystemMetrics(SM_CXSCREEN))ix=wr.left-g_infoWindowWidth;SetWindowPos(g_infoWnd,NULL,ix,wr.top,g_infoWindowWidth,wr.bottom-wr.top,SWP_NOZORDER|SWP_NOACTIVATE);}
        return 0;
    case WM_TIMER: if(wp==1)AdvancePlayback(hwnd); return 0;
    case WM_MOUSEMOVE: {
        TRACKMOUSEEVENT tme={sizeof(tme),TME_LEAVE,hwnd,0};
        TrackMouseEvent(&tme);
        RECT client;GetClientRect(hwnd,&client);
        bool over=(!g_miniMode && HIWORD(lp)<27 && LOWORD(lp)>=client.right-120);
        if(over!=g_windowButtonsHover) {
            g_windowButtonsHover=over;
            RECT topStrip={client.right-125,0,client.right,30};
            InvalidateRect(hwnd,&topStrip,FALSE);
        }
        return 0;
    }
    case WM_MOUSELEAVE: {
        if(g_windowButtonsHover) {
            g_windowButtonsHover=false;
            RECT client;GetClientRect(hwnd,&client);
            RECT topStrip={client.right-125,0,client.right,30};
            InvalidateRect(hwnd,&topStrip,FALSE);
        }
        return 0;
    }
    case WM_DROPFILES: {
        HDROP drop=(HDROP)wp; TCHAR path[MAX_PATH]={};
        if(DragQueryFile(drop,0,path,MAX_PATH))OpenMidiPath(hwnd,path,true);
        DragFinish(drop); return 0;
    }
    case WM_LBUTTONDOWN: {
        RECT client; GetClientRect(hwnd, &client);
        if(g_miniMode){
            int x=LOWORD(lp),y=HIWORD(lp),controlsY=g_progressHeight+9;
            if(g_loaded&&y>=0&&y<g_progressHeight){SeekPlayback(hwnd,x);return 0;}
            int miniTop=g_progressHeight+(MINI_TRANSPORT_HEIGHT-MINI_SCREEN_H)/2;
            RECT miniHit={MINI_SCREEN_X,miniTop,MINI_SCREEN_X+MINI_SCREEN_W,miniTop+MINI_SCREEN_H};
            if(x>=miniHit.left&&x<miniHit.right&&y>=miniHit.top&&y<miniHit.bottom){ToggleMiniMode(hwnd);return 0;}
            if(y>=g_progressHeight){
                if(x>=client.right-85&&x<client.right-30){StartPlayback();InvalidateDynamic(hwnd);return 0;}
                if(x>=client.right-150&&x<client.right-90){StopPlayback();InvalidateDynamic(hwnd);return 0;}
                if(x>=25&&x<125){ShowMidiOutputSettings(hwnd);return 0;}
                if(x>=client.right/2-45&&x<client.right/2+35){LoadDialog(hwnd);return 0;}
                if(x>=client.right/2+35&&x<client.right/2+125){ShowSideContent(hwnd,0);return 0;}
            }
            ReleaseCapture();SendMessage(hwnd,WM_NCLBUTTONDOWN,HTCAPTION,0);return 0;
        }
        int x = LOWORD(lp), y = HIWORD(lp), barY = client.bottom - 77;
        if (g_loaded && y >= barY && y < barY + g_progressHeight) {
            SeekPlayback(hwnd, x);
            return 0;
        }
        if(y>=4&&y<24){
            const int buttonW=21,buttonGap=3,closeX=client.right-27,maxX=closeX-buttonW-buttonGap,minX=maxX-buttonW-buttonGap,miniX=minX-buttonW-buttonGap;
            if(x>=miniX&&x<miniX+buttonW){ToggleMiniMode(hwnd);return 0;}
            if(x>=closeX&&x<closeX+buttonW){SendMessage(hwnd,WM_CLOSE,0,0);return 0;}
            if(x>=maxX&&x<maxX+buttonW){ToggleFullscreen(hwnd);return 0;}
            if(x>=minX&&x<minX+buttonW){ShowWindow(hwnd,SW_MINIMIZE);return 0;}
        }
        if (y >= barY + g_progressHeight) {
            bool handled=false;
            if(y>=client.bottom-31 && x>=client.right/2-55 && x<client.right/2+48){ShowSideContent(hwnd,1);return 0;}
            if(y>=client.bottom-31 && x>=client.right/2+48 && x<client.right/2+112){ShowSideContent(hwnd,2);return 0;}
            if(y>=client.bottom-31 && x>=client.right-145 && x<client.right-35){ShowSideContent(hwnd,3);return 0;}
            if (x >= client.right-85 && x < client.right-30) { StartPlayback(); handled=true; }
            else if (x >= client.right-150 && x < client.right-90) { StopPlayback(); handled=true; }
            else if(x>=25&&x<125&&y>=barY+g_progressHeight+22&&y<barY+g_progressHeight+43){g_trackMode=!g_trackMode;SaveInstrumentDisplaySettings();DestroyBackBuffer();InvalidateRect(hwnd,NULL,FALSE);InvalidateDynamic(hwnd);handled=true;}
            else if (x >= 25 && x < 125) { ShowMidiOutputSettings(hwnd); handled=true; }
            else if (x >= client.right/2-45 && x < client.right/2+35) { LoadDialog(hwnd); handled=true; }
            else if (x >= client.right/2+35 && x < client.right/2+125) {ShowSideContent(hwnd,0);handled=true;}
            if(!handled) {
                ReleaseCapture();
                SendMessage(hwnd,WM_NCLBUTTONDOWN,HTCAPTION,0);
            }
            InvalidateDynamic(hwnd);
        }
        return 0;
    }
    case WM_KEYDOWN:
        if(wp==VK_RETURN && (lp & (1LL<<24))) { StartPlayback(); InvalidateDynamic(hwnd); return 0; }
        if(wp==VK_NUMPAD0) {
            if(g_playing) { g_playing=false; g_lastMs=0; AllNotesOff(); g_numpadZeroPaused=true; }
            else if(g_numpadZeroPaused) { StopPlayback(); g_numpadZeroPaused=false; }
            else g_numpadZeroPaused=true;
            InvalidateDynamic(hwnd); return 0;
        }
        if(wp==VK_PRIOR) { SeekMarker(hwnd,1); return 0; }
        if(wp==VK_NEXT) { SeekMarker(hwnd,-1); return 0; }
        if(wp==g_keyMidiPanic && (!g_keyMidiPanicShift || (GetKeyState(VK_SHIFT)&0x8000))) { MidiPanic(); InvalidateDynamic(hwnd); return 0; }
        if(wp==g_keyMiniMode){ToggleMiniMode(hwnd);return 0;}
        if(wp==g_keySettings){ShowMidiOutputSettings(hwnd);return 0;}
        if(wp==g_keyDisplayMode){g_trackMode=!g_trackMode;SaveInstrumentDisplaySettings();DestroyBackBuffer();InvalidateRect(hwnd,NULL,FALSE);InvalidateDynamic(hwnd);return 0;}
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
    case WM_CLOSE: KillTimer(hwnd,1);if(g_infoWnd)CloseAuthorsGif();SaveInstrumentDisplaySettings();SaveWindow(hwnd);if(g_settingsWnd)DestroyWindow(g_settingsWnd);if(g_infoWnd)DestroyWindow(g_infoWnd);CloseMidiOutput();DestroyBackBuffer();DestroyWindow(hwnd);return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}
// Keep a saved window rectangle usable when the primary display resolution changes.
// External-monitor placement is intentionally not handled here.
static void FitStartupWindowToWorkArea(int& x,int& y,int& w,int& h) {
    RECT work={};
    if(!SystemParametersInfo(SPI_GETWORKAREA,0,&work,0)) return;
    const int areaW=work.right-work.left;
    const int areaH=work.bottom-work.top;
    if(areaW<1||areaH<1) return;
    if(w>areaW-2) w=areaW-2;
    if(h>areaH-2) h=areaH-2;
    if(x<work.left || x+w>work.right) x=work.left+1;
    if(y<work.top || y+h>work.bottom) y=work.top+1;
    if(x+w>work.right) w=work.right-x-1;
    if(y+h>work.bottom) h=work.bottom-y-1;
    if(w<1) w=1;
    if(h<1) h=1;
}

int APIENTRY WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int show) {
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;if(Gdiplus::GdiplusStartup(&g_gdiplusToken,&gdiplusStartupInput,NULL)!=Gdiplus::Ok)return 1;
    TCHAR exe[MAX_PATH]; GetModuleFileName(NULL, exe, MAX_PATH);
    lstrcpyn(g_iniPath, exe, MAX_PATH);
    TCHAR* slash = _tcsrchr(g_iniPath, _T('\\')); if (slash) *(slash + 1) = 0;
    _tcscat_s(g_iniPath, MAX_PATH, _T("SETTINGS.INI"));

    LoadColorPalette();
    LoadInterfaceColors();
    LoadInstrumentDisplaySettings();
    LoadKeyboardSettings();
    LoadProgramNames();
    LoadMidiOutputSettings();
    int x = GetIniIntAnySection(_T("windowX"),100);
    int y = GetIniIntAnySection(_T("windowY"),100);
    int w = GetIniIntAnySection(_T("windowWidth"),WIN_W);
    int h = GetIniIntAnySection(_T("windowHeight"),WIN_H);
    g_startFullscreen=GetIniIntAnySection(_T("WindowsState"),0)!=0;
    if (w < 500) w = WIN_W; if (h < 350) h = WIN_H;
    FitStartupWindowToWorkArea(x,y,w,h);
    const TCHAR CLASS_NAME[] = _T("MIDI_PIXEL_FIRST_WINDOW");
    WNDCLASS wc = {}; wc.lpfnWndProc = WindowProc; wc.hInstance = instance; wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    if (!RegisterClass(&wc)) return 1;
    WNDCLASS swc={};swc.lpfnWndProc=SettingsWindowProc;swc.hInstance=instance;swc.lpszClassName=_T("MIDI_PIXEL_SETTINGS_WINDOW");swc.hCursor=LoadCursor(NULL,IDC_ARROW);swc.hbrBackground=GetSysColorBrush(COLOR_3DFACE);if(!RegisterClass(&swc))return 1;
    WNDCLASS iwc={}; iwc.lpfnWndProc=InfoWindowProc; iwc.hInstance=instance; iwc.lpszClassName=_T("MIDI_PIXEL_INFO_WINDOW");
    iwc.hCursor=LoadCursor(NULL,IDC_ARROW); iwc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
    if(!RegisterClass(&iwc))return 1;
    TCHAR title[128]; _stprintf_s(title, _T("MIDI PIXEL %s"), VERSION);
    HWND hwnd = CreateWindowEx(0, CLASS_NAME, title, WS_POPUP | WS_THICKFRAME,
        x, y, w, h, NULL, NULL, instance, NULL);
    if (!hwnd) return 2;
    OpenMidiOutput(hwnd);
    DragAcceptFiles(hwnd,TRUE);
    ShowWindow(hwnd, show); UpdateWindow(hwnd);
    if(g_startFullscreen)ToggleFullscreen(hwnd);
    SetTimer(hwnd, 1, 10, NULL);
    int argc=0;
    LPWSTR* argv=CommandLineToArgvW(GetCommandLineW(),&argc);
    if(argv) {
        bool opened=false;
        // Some file managers pass an unquoted %1 path. Reassemble consecutive
        // command-line tokens until they form an existing file path.
        for(int first=1;first<argc && !opened;++first) {
            WCHAR candidate[32768]={};
            for(int last=first;last<argc;++last) {
                size_t used=wcslen(candidate);
                if(used && used+1<sizeof(candidate)/sizeof(candidate[0])) {
                    candidate[used]=L' '; candidate[used+1]=0;
                }
                used=wcslen(candidate);
                size_t remaining=sizeof(candidate)/sizeof(candidate[0])-used-1;
                wcsncat_s(candidate,sizeof(candidate)/sizeof(candidate[0]),argv[last],remaining);
                if(GetFileAttributesW(candidate)!=INVALID_FILE_ATTRIBUTES) {
                    TCHAR initialPath[MAX_PATH]={};
#ifdef UNICODE
                    lstrcpyn(initialPath,candidate,MAX_PATH);
#else
                    WideCharToMultiByte(CP_ACP,0,candidate,-1,initialPath,MAX_PATH,NULL,NULL);
#endif
                    OpenMidiPath(hwnd,initialPath,true);
                    opened=true;
                    break;
                }
            }
        }
        LocalFree(argv);
    }
    MSG msg; while (GetMessage(&msg, NULL, 0, 0) > 0) { TranslateMessage(&msg); DispatchMessage(&msg); }
    CloseAuthorsGif();if(g_gdiplusToken)Gdiplus::GdiplusShutdown(g_gdiplusToken);
    return (int)msg.wParam;
}
