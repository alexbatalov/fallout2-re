#ifndef WIN32_H
#define WIN32_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define DIRECTDRAW_VERSION 0x0300
#include <ddraw.h>

#define DIRECTINPUT_VERSION 0x0300
#include <dinput.h>
#include <mmreg.h>

#define DIRECTSOUND_VERSION 0x0300
#include <dsound.h>

#include <stdbool.h>

typedef HRESULT(__stdcall DirectDrawCreateProc)(GUID*, LPDIRECTDRAW*, IUnknown*);
typedef HRESULT(__stdcall DirectInputCreateAProc)(HINSTANCE, DWORD, LPDIRECTINPUTA*, IUnknown*);
typedef HRESULT(__stdcall DirectSoundCreateProc)(GUID*, LPDIRECTSOUND*, IUnknown*);

extern DirectDrawCreateProc* gDirectDrawCreateProc;
extern DirectInputCreateAProc* gDirectInputCreateAProc;
extern DirectSoundCreateProc* gDirectSoundCreateProc;
extern HWND gProgramWindow;
extern HINSTANCE gInstance;
extern LPSTR gCmdLine;
extern int gCmdShow;
extern bool gProgramIsActive;
extern HANDLE dword_51E448;
extern HMODULE gDDrawDLL;
extern HMODULE gDInputDLL;
extern HMODULE gDSoundDLL;

ATOM sub_4DE7F4(HINSTANCE hInstance);
bool sub_4DE864();
bool sub_4DE8D0();
void sub_4DE988(void);
void sub_4DE9F4(int sig);
LRESULT CALLBACK sub_4DE9FC(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif /* WIN32_H */
