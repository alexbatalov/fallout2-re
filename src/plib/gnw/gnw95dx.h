#ifndef FALLOUT_PLIB_GNW_GNW95DX_H_
#define FALLOUT_PLIB_GNW_GNW95DX_H_

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define DIRECTDRAW_VERSION 0x0300
#include <ddraw.h>

#define DIRECTINPUT_VERSION 0x0300
#include <dinput.h>
#include <mmreg.h>

#define DIRECTSOUND_VERSION 0x0300
#include <dsound.h>

typedef HRESULT(__stdcall *PFNDDRAWCREATE)(GUID*, LPDIRECTDRAW*, IUnknown*);
typedef HRESULT(__stdcall *PFNDINPUTCREATE)(HINSTANCE, DWORD, LPDIRECTINPUTA*, IUnknown*);
typedef HRESULT(__stdcall *PFNDSOUNDCREATE)(GUID*, LPDIRECTSOUND*, IUnknown*);

extern PFNDDRAWCREATE GNW95_DirectDrawCreate;
extern PFNDINPUTCREATE GNW95_DirectInputCreate;
extern PFNDSOUNDCREATE GNW95_DirectSoundCreate;

#endif /* FALLOUT_PLIB_GNW_GNW95DX_H_ */
