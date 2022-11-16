#include "plib/gnw/winmain.h"

#include <signal.h>

#include "plib/gnw/doscmdln.h"
#include "core.h"
#include "game/main.h"
#include "plib/gnw/gnw.h"

static BOOL LoadDirectX();
static void UnloadDirectX(void);

// 0x51E434
HWND GNW95_hwnd = NULL;

// 0x51E438
HINSTANCE GNW95_hInstance = NULL;

// 0x51E43C
LPSTR GNW95_lpszCmdLine = NULL;

// 0x51E440
int GNW95_nCmdShow = 0;

// 0x51E444
bool GNW95_isActive = false;

// GNW95MUTEX
HANDLE GNW95_mutex = NULL;

// 0x51E44C
HMODULE GNW95_hDDrawLib = NULL;

// 0x51E450
HMODULE GNW95_hDInputLib = NULL;

// 0x51E454
HMODULE GNW95_hDSoundLib = NULL;

// 0x6B23D0
char GNW95_title[256];

// 0x4DE700
int WINAPI WinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE hPrevInst, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    DOSCmdLine args;

    GNW95_mutex = CreateMutexA(0, TRUE, "GNW95MUTEX");
    if (GetLastError() == ERROR_SUCCESS) {
        ShowCursor(0);
        if (InitClass(hInst)) {
            if (InitInstance()) {
                if (LoadDirectX()) {
                    GNW95_hInstance = hInst;
                    GNW95_lpszCmdLine = lpCmdLine;
                    GNW95_nCmdShow = nCmdShow;
                    DOSCmdLineInit(&args);
                    if (DOSCmdLineCreate(&args, lpCmdLine)) {
                        signal(1, SignalHandler);
                        signal(3, SignalHandler);
                        signal(5, SignalHandler);
                        GNW95_isActive = true;
                        RealMain(args.numArgs, args.args);
                        DOSCmdLineDestroy(&args);
                        return 1;
                    }
                }
            }
        }
        CloseHandle(GNW95_mutex);
    }
    return 0;
}

// 0x4DE7F4
BOOL InitClass(HINSTANCE hInstance)
{
    WNDCLASSA wc;
    wc.style = 3;
    wc.lpfnWndProc = WindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = NULL;
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "GNW95 Class";

    return RegisterClassA(&wc);
}

// 0x4DE864
BOOL InitInstance()
{
    OSVERSIONINFOA osvi;
    bool result;

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);

#pragma warning(suppress : 4996 28159)
    if (!GetVersionExA(&osvi)) {
        return true;
    }

    result = true;

    if (osvi.dwPlatformId == 0 || osvi.dwPlatformId == 2 && osvi.dwMajorVersion < 4) {
        result = false;
    }

    if (!result) {
        MessageBoxA(NULL, "This program requires Windows 95 or Windows NT version 4.0 or greater.", "Wrong Windows Version", MB_ICONSTOP);
    }

    return result;
}

// 0x4DE8D0
static BOOL LoadDirectX()
{
    GNW95_hDDrawLib = LoadLibraryA("DDRAW.DLL");
    if (GNW95_hDDrawLib == NULL) {
        goto err;
    }

    GNW95_DirectDrawCreate = (PFNDDRAWCREATE)GetProcAddress(GNW95_hDDrawLib, "DirectDrawCreate");
    if (GNW95_DirectDrawCreate == NULL) {
        goto err;
    }

    GNW95_hDInputLib = LoadLibraryA("DINPUT.DLL");
    if (GNW95_hDInputLib == NULL) {
        goto err;
    }

    GNW95_DirectInputCreate = (PFNDINPUTCREATE)GetProcAddress(GNW95_hDInputLib, "DirectInputCreateA");
    if (GNW95_DirectInputCreate == NULL) {
        goto err;
    }

    GNW95_hDSoundLib = LoadLibraryA("DSOUND.DLL");
    if (GNW95_hDSoundLib == NULL) {
        goto err;
    }

    GNW95_DirectSoundCreate = (PFNDSOUNDCREATE)GetProcAddress(GNW95_hDSoundLib, "DirectSoundCreate");
    if (GNW95_DirectSoundCreate == NULL) {
        goto err;
    }

    atexit(UnloadDirectX);

    return TRUE;

err:
    UnloadDirectX();

    MessageBoxA(NULL, "This program requires Windows 95 with DirectX 3.0a or later or Windows NT version 4.0 with Service Pack 3 or greater.", "Could not load DirectX", MB_ICONSTOP);

    return FALSE;
}

// 0x4DE988
static void UnloadDirectX(void)
{
    if (GNW95_hDSoundLib != NULL) {
        FreeLibrary(GNW95_hDSoundLib);
        GNW95_hDSoundLib = NULL;
        GNW95_DirectDrawCreate = NULL;
    }

    if (GNW95_hDDrawLib != NULL) {
        FreeLibrary(GNW95_hDDrawLib);
        GNW95_hDDrawLib = NULL;
        GNW95_DirectSoundCreate = NULL;
    }

    if (GNW95_hDInputLib != NULL) {
        FreeLibrary(GNW95_hDInputLib);
        GNW95_hDInputLib = NULL;
        GNW95_DirectInputCreate = NULL;
    }
}

// 0x4DE9F4
void SignalHandler(int sig)
{
    windowManagerExit();
}

// 0x4DE9FC
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_DESTROY:
        exit(EXIT_SUCCESS);
    case WM_PAINT:
        if (1) {
            RECT updateRect;
            if (GetUpdateRect(hWnd, &updateRect, FALSE)) {
                Rect rect;
                rect.ulx = updateRect.left;
                rect.uly = updateRect.top;
                rect.lrx = updateRect.right - 1;
                rect.lry = updateRect.bottom - 1;
                windowRefreshAll(&rect);
            }
        }
        break;
    case WM_ERASEBKGND:
        return 1;
    case WM_SETCURSOR:
        if ((HWND)wParam == GNW95_hwnd) {
            SetCursor(NULL);
            return 1;
        }
        break;
    case WM_SYSCOMMAND:
        switch (wParam & 0xFFF0) {
        case SC_SCREENSAVE:
        case SC_MONITORPOWER:
            return 0;
        }
        break;
    case WM_ACTIVATEAPP:
        GNW95_isActive = wParam;
        if (wParam) {
            _GNW95_hook_input(1);
            windowRefreshAll(&_scr_size);
        } else {
            _GNW95_hook_input(0);
        }

        return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
