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

static const TCHAR VERSION[] = _T("v0.203");
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
static unsigned char g_active[MIDI_CHANNELS][MIDI_NOTES] = {};
static std::vector<std::vector<unsigned char>> g_trackActive,g_trackPercussionActive;
static std::vector<std::vector<int>> g_trackVelocity,g_trackPercussionVelocity;
static std::vector<int> g_trackExpression,g_trackPan;
static COLORREF g_verticalPanCenterLine=RGB(110,100,75);
static bool g_trackMode=false;
static COLORREF g_percussionColor=RGB(230,110,190);
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
static int g_seekSeconds=5;
static int g_footerAppSize=12,g_footerAuthorSize=9,g_footerVersionSize=10;
static int g_trackTitleFontSize=17;
static COLORREF g_footerAppColor=RGB(30,30,45),g_footerAuthorColor=RGB(30,30,45),g_footerVersionColor=RGB(30,30,45);
static bool g_fullscreen=false;
static bool g_miniMode=false;
static RECT g_miniRestoreRect={};
static const int MINI_TRANSPORT_HEIGHT=54;
static const int MINI_SCREEN_X=168,MINI_SCREEN_W=130,MINI_SCREEN_H=34;
static bool g_startFullscreen=false;
static COLORREF g_windowButtonsColor=RGB(49,46,72);
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
    g_seekSeconds=GetIniIntAnySection(_T("SeekSeconds"),5);
    if(g_seekSeconds<1)g_seekSeconds=1; if(g_seekSeconds>600)g_seekSeconds=600;
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
    g_trackTitleFontSize=GetIniIntAnySection(_T("TrackTitleFontSize"),17);
    if(g_trackTitleFontSize<8)g_trackTitleFontSize=8;if(g_trackTitleFontSize>40)g_trackTitleFontSize=40;
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
    for(int ch=0;ch<MIDI_CHANNELS;++ch)SendMidiShort((unsigned char)(0xE0|ch),0,64);
    std::fill(g_trackExpression.begin(),g_trackExpression.end(),127);
    std::fill(g_trackPan.begin(),g_trackPan.end(),64);
    memset(g_velocity, 0, sizeof(g_velocity));
    for(size_t t=0;t<g_trackActive.size();++t){std::fill(g_trackActive[t].begin(),g_trackActive[t].end(),0);std::fill(g_trackPercussionActive[t].begin(),g_trackPercussionActive[t].end(),0);std::fill(g_trackVelocity[t].begin(),g_trackVelocity[t].end(),0);std::fill(g_trackPercussionVelocity[t].begin(),g_trackPercussionVelocity[t].end(),0);}
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
                    if(label && (metaType==0x01 || metaType==0x02 || metaType==0x06))
                        midiText.push_back({tick,0,label,MidiTextToAnsi(payload,len)});
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
    g_events.swap(events);
    g_tracks.swap(tracks);
    g_trackActive.assign(g_tracks.size(),std::vector<unsigned char>(MIDI_NOTES,0));
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
 int x=NotePixelX(note,clientWidth),y=DisplayRowY(row)-6;RECT r={x,y,x+4,y+6};return r;
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
        } else {
            RECT oldRect = g_trackMode?NoteRect(e.track,e.a,client.right):NoteRect(e.channel,e.a,client.right);
            if (e.type == 1) {
                SendMidiShort((unsigned char)(0x90 | e.channel), (unsigned char)e.a, (unsigned char)e.b);
                g_active[e.channel][e.a]=1;g_velocity[e.channel][e.a]=e.b;
                if(e.track>=0&&(size_t)e.track<g_trackActive.size()){if(e.channel==9){g_trackPercussionActive[e.track][e.a]=1;g_trackPercussionVelocity[e.track][e.a]=e.b;}else{g_trackActive[e.track][e.a]=1;g_trackVelocity[e.track][e.a]=e.b;}}
            } else {
                SendMidiShort((unsigned char)(0x80 | e.channel), (unsigned char)e.a, (unsigned char)e.b);
                g_active[e.channel][e.a]=0;g_velocity[e.channel][e.a]=0;
                if(e.track>=0&&(size_t)e.track<g_trackActive.size()){g_trackActive[e.track][e.a]=0;g_trackPercussionActive[e.track][e.a]=0;g_trackVelocity[e.track][e.a]=0;g_trackPercussionVelocity[e.track][e.a]=0;}
            }
            RECT newRect = g_trackMode?NoteRect(e.track,e.a,client.right):NoteRect(e.channel,e.a,client.right);
            if(g_miniMode) {
                RECT miniNotes={MINI_SCREEN_X,g_progressHeight+1,MINI_SCREEN_X+MINI_SCREEN_W,g_progressHeight+1+MINI_SCREEN_H};
                InvalidateRect(hwnd,&miniNotes,FALSE);
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
        else if (e.type == 6) SendMidiShort((unsigned char)(0xE0 | e.channel), (unsigned char)e.a, (unsigned char)e.b);
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
       RECT nr={84,y-7,timelineX-7,y+9};SetBkMode(dc,TRANSPARENT);SetTextColor(dc,label);HFONT nf=CreateFont(10,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_DONTCARE,_T("Segoe UI"));HFONT no=(HFONT)SelectObject(dc,nf);DrawText(dc,name,-1,&nr,DT_SINGLELINE|DT_VCENTER|DT_RIGHT|DT_END_ELLIPSIS|DT_NOPREFIX);SelectObject(dc,no);DeleteObject(nf);}
      if(g_trackMode&&g_loaded){RECT vr={24,y-2,48,y+2};FillRectColor(dc,vr,RGB(35,32,25));RECT pr={54,y-2,78,y+2};FillRectColor(dc,pr,RGB(35,32,25));}
      DrawLine(dc,timelineX,y+2,timelineR,line);
      for(int n=g_firstVisibleNote;n<=g_lastVisibleNote;++n)if(n%12==0){int mx=NotePixelX(n,width),half=g_lineThickness/2;RECT mr={mx,y+2-half,mx+4,y+2-half+g_lineThickness};FillRectColor(dc,mr,n==60?g_linesC3Color:g_linesCColor);}
    }
    if(g_trackMode&&g_loaded&&rows>0){int firstY=DisplayRowY(0),lastY=DisplayRowY(rows-1);HPEN panPen=CreatePen(PS_SOLID,1,g_verticalPanCenterLine);HPEN oldPan=(HPEN)SelectObject(dc,panPen);MoveToEx(dc,66,firstY-7,NULL);LineTo(dc,66,lastY+8);SelectObject(dc,oldPan);DeleteObject(panPen);}
    int headerY=rowTop+rows*rowH+4;if(!g_trackMode){Txt(dc,channelX,headerY,_T("CH"),label,9,FW_SEMIBOLD);Txt(dc,presetX,headerY,_T("PROGRAM"),label,9,FW_SEMIBOLD);}int infoY=headerY+12;
    TCHAR info[128];
    int barY = height - 77;
    if (g_loaded) {
        TCHAR displayTitle[MAX_PATH]; lstrcpyn(displayTitle,g_fileName,MAX_PATH);
        TCHAR* extension=_tcsrchr(displayTitle,_T('.')); if(extension)*extension=0;

        // Fixed three-line metadata block: current Marker, first Text Event, first Copyright.
        const int titleLineHeight=g_trackTitleFontSize+5;
        const int eventLineHeight=16;
        const int blockHeight=titleLineHeight+3*eventLineHeight;
        int blockY=barY-8-blockHeight;
        RECT textArea={timelineX,blockY,width-24,blockY+titleLineHeight};
        TxtCenteredMono(dc,textArea,displayTitle,RGB(235,235,242),g_trackTitleFontSize);
        int eventY=blockY+titleLineHeight;
        const MidiTextInfo* currentMarker=NULL;
        const MidiTextInfo* firstText=NULL;
        const MidiTextInfo* firstCopyright=NULL;
        for(size_t i=0;i<g_midiText.size();++i) {
            const MidiTextInfo& item=g_midiText[i];
            if(item.label=="Marker" && item.usec<=g_elapsedUsec) currentMarker=&item;
            else if(item.label=="Text" && !firstText) firstText=&item;
            else if(item.label=="Copyright" && !firstCopyright) firstCopyright=&item;
        }
        const MidiTextInfo* displayItems[3]={currentMarker,firstText,firstCopyright};
        for(int lineIndex=0;lineIndex<3;++lineIndex) {
            TCHAR eventText[1024]={};
            if(displayItems[lineIndex]) {
#ifdef UNICODE
                MultiByteToWideChar(CP_ACP,0,displayItems[lineIndex]->text.c_str(),-1,eventText,1024);
#else
                lstrcpynA(eventText,displayItems[lineIndex]->text.c_str(),1024);
#endif
            }
            RECT eventRect={timelineX,eventY,width-24,eventY+eventLineHeight};
            TxtCenteredMono(dc,eventRect,eventText,RGB(210,205,190),10);
            eventY+=eventLineHeight;
        }
        _stprintf_s(info, _T("%u tracks   /   %u ticks per quarter"), (unsigned int)g_tracks.size(), (unsigned int)g_division);
        Txt(dc, 24, barY - 55, info, label, 10);
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
        RECT mini={MINI_SCREEN_X,controlsY-8,MINI_SCREEN_X+MINI_SCREEN_W,controlsY-8+MINI_SCREEN_H};
        FillRectColor(dc,mini,RGB(25,23,38));DrawWindowButtonFrame(dc,mini,g_uiLines);
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
    int rows=DisplayRowCount();
    if(!g_trackMode){for(int ch=0;ch<MIDI_CHANNELS;++ch){int y=DisplayRowY(ch);TCHAR p[16]={};if(ch==9)lstrcpyn(p,_T("RYTHME"),16);else _stprintf_s(p,_T("%03d"),g_program[ch]+1);Txt(dc,72,y-5,g_loaded?p:_T("---"),RGB(160,170,195),10);
      if(ch!=9&&g_loaded&&g_programNames[g_program[ch]][0]){RECT nr={106,y-7,timelineX-7,y+9};TxtClipped(dc,nr,g_programNames[g_program[ch]],RGB(205,210,225),9);}
      if(!g_loaded)continue;for(int n=0;n<MIDI_NOTES;++n)if(g_active[ch][n]){RECT nr=NoteRect(ch,n,cr.right);if(nr.right<ps.rcPaint.left||nr.left>ps.rcPaint.right||nr.bottom<ps.rcPaint.top||nr.top>ps.rcPaint.bottom)continue;FillRectColor(dc,nr,ch==9?ColorForPercussion(g_velocity[ch][n]):ColorForNote(n,g_velocity[ch][n]));}}}
    else if(g_loaded){for(int tr=0;tr<rows;++tr){int y=DisplayRowY(tr);RECT vr={24,y-2,24+(24*g_trackExpression[tr])/127,y+2};if(vr.right>vr.left&&vr.right>ps.rcPaint.left&&vr.left<ps.rcPaint.right&&vr.bottom>ps.rcPaint.top&&vr.top<ps.rcPaint.bottom)FillRectColor(dc,vr,RGB(205,150,55));int px=54+(24*g_trackPan[tr])/127;RECT pan={px-1,y-5,px+2,y+6};if(pan.right>ps.rcPaint.left&&pan.left<ps.rcPaint.right&&pan.bottom>ps.rcPaint.top&&pan.top<ps.rcPaint.bottom)FillRectColor(dc,pan,RGB(175,190,205));for(int n=0;n<MIDI_NOTES;++n){RECT nr=NoteRect(tr,n,cr.right);if(nr.right<ps.rcPaint.left||nr.left>ps.rcPaint.right||nr.bottom<ps.rcPaint.top||nr.top>ps.rcPaint.bottom)continue;if(g_trackActive[tr][n])FillRectColor(dc,nr,ColorForNote(n,g_trackVelocity[tr][n]));if(g_trackPercussionActive[tr][n])FillRectColor(dc,nr,ColorForPercussion(g_trackPercussionVelocity[tr][n]));}}}
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
    const int buttonY=4,buttonW=21,buttonH=19,buttonGap=3;
    const int closeX=cr.right-27,maxX=closeX-buttonW-buttonGap,minX=maxX-buttonW-buttonGap,miniX=minX-buttonW-buttonGap;
    RECT button={minX,buttonY,minX+buttonW,buttonY+buttonH};FillRectColor(dc,button,g_windowButtonsColor);DrawWindowButtonFrame(dc,button,g_uiLines);TxtCenteredMono(dc,button,_T("_"),RGB(225,225,232),12,FW_BOLD);
    button={maxX,buttonY,maxX+buttonW,buttonY+buttonH};FillRectColor(dc,button,g_windowButtonsColor);DrawWindowButtonFrame(dc,button,g_uiLines);TxtCenteredMono(dc,button,_T("□"),RGB(225,225,232),11,FW_NORMAL);
    button={miniX,buttonY,miniX+buttonW,buttonY+buttonH};FillRectColor(dc,button,g_windowButtonsColor);DrawWindowButtonFrame(dc,button,g_uiLines);TxtCenteredMono(dc,button,_T("M"),RGB(225,225,232),11,FW_BOLD);
    button={closeX,buttonY,closeX+buttonW,buttonY+buttonH};FillRectColor(dc,button,g_windowButtonsColor);DrawWindowButtonFrame(dc,button,g_uiLines);TxtCenteredMono(dc,button,_T("X"),RGB(225,225,232),11,FW_BOLD);
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
static LRESULT CALLBACK SettingsWindowProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) {
    switch(msg){
    case WM_CREATE:{
        HFONT font=(HFONT)GetStockObject(DEFAULT_GUI_FONT);
        HWND title=CreateWindowEx(0,_T("STATIC"),_T("MIDI OUTPUT"),WS_CHILD|WS_VISIBLE,18,16,250,20,hwnd,NULL,NULL,NULL);
        g_mapperCheck=CreateWindowEx(0,_T("BUTTON"),_T("Use MIDI Mapper"),WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,20,44,170,24,hwnd,(HMENU)101,NULL,NULL);
        g_outputCombo=CreateWindowEx(0,_T("COMBOBOX"),_T(""),WS_CHILD|WS_VISIBLE|WS_VSCROLL|CBS_DROPDOWNLIST,20,78,330,180,hwnd,(HMENU)102,NULL,NULL);
        HWND apply=CreateWindowEx(0,_T("BUTTON"),_T("Apply"),WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,190,126,76,28,hwnd,(HMENU)103,NULL,NULL);
        HWND close=CreateWindowEx(0,_T("BUTTON"),_T("Close"),WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,274,126,76,28,hwnd,(HMENU)104,NULL,NULL);
        SendMessage(title,WM_SETFONT,(WPARAM)font,TRUE);SendMessage(g_mapperCheck,WM_SETFONT,(WPARAM)font,TRUE);SendMessage(g_outputCombo,WM_SETFONT,(WPARAM)font,TRUE);SendMessage(apply,WM_SETFONT,(WPARAM)font,TRUE);SendMessage(close,WM_SETFONT,(WPARAM)font,TRUE);
        SendMessage(g_mapperCheck,BM_SETCHECK,g_useMidiMapper?BST_CHECKED:BST_UNCHECKED,0);RefreshOutputCombo();return 0;
    }
    case WM_COMMAND:
        if(LOWORD(wp)==101&&HIWORD(wp)==BN_CLICKED){g_useMidiMapper=SendMessage(g_mapperCheck,BM_GETCHECK,0,0)==BST_CHECKED;EnableWindow(g_outputCombo,!g_useMidiMapper);return 0;}
        if(LOWORD(wp)==103){g_useMidiMapper=SendMessage(g_mapperCheck,BM_GETCHECK,0,0)==BST_CHECKED;if(!g_useMidiMapper){int row=(int)SendMessage(g_outputCombo,CB_GETCURSEL,0,0);if(row>=0)g_selectedMidiDevice=(UINT)SendMessage(g_outputCombo,CB_GETITEMDATA,row,0);}SaveMidiOutputSettings();OpenMidiOutput(hwnd);DestroyBackBuffer();if(GetParent(hwnd))InvalidateRect(GetParent(hwnd),NULL,FALSE);ShowWindow(hwnd,SW_HIDE);return 0;}
        if(LOWORD(wp)==104){ShowWindow(hwnd,SW_HIDE);return 0;}break;
    case WM_CTLCOLORSTATIC:case WM_CTLCOLORBTN:{HDC dc=(HDC)wp;SetBkColor(dc,g_uiBackground);SetTextColor(dc,RGB(235,235,242));static HBRUSH brush=NULL;if(!brush)brush=CreateSolidBrush(g_uiBackground);return (LRESULT)brush;}
    case WM_ERASEBKGND:{RECT r;GetClientRect(hwnd,&r);FillRectColor((HDC)wp,r,g_uiBackground);return 1;}
    case WM_CLOSE:ShowWindow(hwnd,SW_HIDE);return 0;
    }
    return DefWindowProc(hwnd,msg,wp,lp);
}
static void ShowMidiOutputSettings(HWND owner) {
    if(!g_settingsWnd)g_settingsWnd=CreateWindowEx(WS_EX_TOOLWINDOW,_T("MIDI_PIXEL_SETTINGS_WINDOW"),_T("Midi Player Device Settings"),WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_BORDER,CW_USEDEFAULT,CW_USEDEFAULT,390,205,owner,NULL,(HINSTANCE)GetWindowLongPtr(owner,GWLP_HINSTANCE),NULL);
    if(g_settingsWnd){RECT wr;GetWindowRect(owner,&wr);SetWindowPos(g_settingsWnd,HWND_TOP,wr.left+30,wr.top+30,390,205,SWP_SHOWWINDOW);SendMessage(g_mapperCheck,BM_SETCHECK,g_useMidiMapper?BST_CHECKED:BST_UNCHECKED,0);RefreshOutputCombo();ShowWindow(g_settingsWnd,SW_SHOW);UpdateWindow(g_settingsWnd);}
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
        if(g_miniMode){
            int x=LOWORD(lp),y=HIWORD(lp),controlsY=g_progressHeight+9;
            if(g_loaded&&y>=0&&y<g_progressHeight){SeekPlayback(hwnd,x);return 0;}
            RECT miniHit={MINI_SCREEN_X,controlsY-8,MINI_SCREEN_X+MINI_SCREEN_W,controlsY-8+MINI_SCREEN_H};
            if(x>=miniHit.left&&x<miniHit.right&&y>=miniHit.top&&y<miniHit.bottom){ToggleMiniMode(hwnd);return 0;}
            if(y>=g_progressHeight){
                if(x>=client.right-85&&x<client.right-30){StartPlayback();InvalidateDynamic(hwnd);return 0;}
                if(x>=client.right-150&&x<client.right-90){StopPlayback();InvalidateDynamic(hwnd);return 0;}
                if(x>=25&&x<125){ShowMidiOutputSettings(hwnd);return 0;}
                if(x>=client.right/2-45&&x<client.right/2+35){LoadDialog(hwnd);return 0;}
                if(x>=client.right/2+35&&x<client.right/2+125){
                    if(g_infoWnd&&IsWindowVisible(g_infoWnd))ShowWindow(g_infoWnd,SW_HIDE);
                    else if(g_infoWnd){InvalidateRect(g_infoWnd,NULL,TRUE);ShowWindow(g_infoWnd,SW_SHOWNOACTIVATE);}
                    return 0;
                }
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
            if (x >= client.right-85 && x < client.right-30) { StartPlayback(); handled=true; }
            else if (x >= client.right-150 && x < client.right-90) { StopPlayback(); handled=true; }
            else if(x>=25&&x<125&&y>=barY+g_progressHeight+22&&y<barY+g_progressHeight+43){g_trackMode=!g_trackMode;SaveInstrumentDisplaySettings();DestroyBackBuffer();InvalidateRect(hwnd,NULL,FALSE);InvalidateDynamic(hwnd);handled=true;}
            else if (x >= 25 && x < 125) { ShowMidiOutputSettings(hwnd); handled=true; }
            else if (x >= client.right/2-45 && x < client.right/2+35) { LoadDialog(hwnd); handled=true; }
            else if (x >= client.right/2+35 && x < client.right/2+125) {
                if(g_infoWnd && IsWindowVisible(g_infoWnd))ShowWindow(g_infoWnd,SW_HIDE);
                else {
                    if(!g_infoWnd) {
                        RECT wr;GetWindowRect(hwnd,&wr);int ix=wr.right+1;if(ix+350>GetSystemMetrics(SM_CXSCREEN))ix=wr.left-351;
                        g_infoWnd=CreateWindowEx(WS_EX_TOOLWINDOW,_T("MIDI_PIXEL_INFO_WINDOW"),_T(""),WS_POPUP|WS_BORDER,ix,wr.top,350,wr.bottom-wr.top,hwnd,NULL,(HINSTANCE)GetWindowLongPtr(hwnd,GWLP_HINSTANCE),NULL);
                    }
                    if(g_infoWnd){InvalidateRect(g_infoWnd,NULL,TRUE);ShowWindow(g_infoWnd,SW_SHOWNOACTIVATE);UpdateWindow(g_infoWnd);}
                }
                handled=true;
            }
            if(!handled) {
                ReleaseCapture();
                SendMessage(hwnd,WM_NCLBUTTONDOWN,HTCAPTION,0);
            }
            InvalidateDynamic(hwnd);
        }
        return 0;
    }
    case WM_KEYDOWN:
        if(g_miniMode&&wp=='M'){ToggleMiniMode(hwnd);return 0;}
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
    case WM_CLOSE: KillTimer(hwnd,1);SaveInstrumentDisplaySettings();SaveWindow(hwnd);if(g_settingsWnd)DestroyWindow(g_settingsWnd);CloseMidiOutput();DestroyBackBuffer();DestroyWindow(hwnd);return 0;
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
    LoadInstrumentDisplaySettings();
    LoadKeyboardSettings();
    LoadProgramNames();
    LoadMidiOutputSettings();
    int x = GetIniIntAnySection(_T("windowX"),100);
    int y = GetIniIntAnySection(_T("windowY"),100);
    int w = GetIniIntAnySection(_T("windowWidth"),WIN_W);
    int h = GetIniIntAnySection(_T("windowHeight"),WIN_H);
    g_startFullscreen=GetIniIntAnySection(_T("WindowsState"),0)!=0;
    if (w < 500 || w > 2000) w = WIN_W; if (h < 350 || h > 1400) h = WIN_H;
    const TCHAR CLASS_NAME[] = _T("MIDI_PIXEL_FIRST_WINDOW");
    WNDCLASS wc = {}; wc.lpfnWndProc = WindowProc; wc.hInstance = instance; wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    if (!RegisterClass(&wc)) return 1;
    WNDCLASS swc={};swc.lpfnWndProc=SettingsWindowProc;swc.hInstance=instance;swc.lpszClassName=_T("MIDI_PIXEL_SETTINGS_WINDOW");swc.hCursor=LoadCursor(NULL,IDC_ARROW);swc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);if(!RegisterClass(&swc))return 1;
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
    return (int)msg.wParam;
}
