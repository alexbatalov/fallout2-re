#ifndef FALLOUT_PLIB_GNW_WINMAIN_H_
#define FALLOUT_PLIB_GNW_WINMAIN_H_

#include <stdbool.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

extern HWND GNW95_hwnd;
extern HINSTANCE GNW95_hInstance;
extern LPSTR GNW95_lpszCmdLine;
extern int GNW95_nCmdShow;
extern bool GNW95_isActive;
extern HANDLE GNW95_mutex;
extern HMODULE GNW95_hDDrawLib;
extern HMODULE GNW95_hDInputLib;
extern HMODULE GNW95_hDSoundLib;

extern char GNW95_title[256];

BOOL InitClass(HINSTANCE hInstance);
BOOL InitInstance();
void SignalHandler(int signalID);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif /* FALLOUT_PLIB_GNW_WINMAIN_H_ */
