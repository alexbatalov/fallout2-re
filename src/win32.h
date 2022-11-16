#ifndef WIN32_H
#define WIN32_H

#include <stdbool.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "plib/gnw/gnw95dx.h"

extern HWND gProgramWindow;
extern HINSTANCE gInstance;
extern LPSTR gCmdLine;
extern int gCmdShow;
extern bool gProgramIsActive;
extern HANDLE _GNW95_mutex;
extern HMODULE gDDrawDLL;
extern HMODULE gDInputDLL;
extern HMODULE gDSoundDLL;

ATOM _InitClass(HINSTANCE hInstance);
bool _InitInstance();
bool _LoadDirectX();
void _UnloadDirectX(void);
void _SignalHandler(int sig);
LRESULT CALLBACK _WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif /* WIN32_H */
