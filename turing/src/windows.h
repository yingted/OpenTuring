#ifndef __STUB_WINDOWS_H__
#define __STUB_WINDOWS_H__

#include <stdarg.h>

#define NULL		((void*) 0)
#define TRUE		1
#define FALSE		0

typedef void *HINSTANCE;
typedef void *HFONT;
typedef void *HINSTANCE;
typedef void *HMENU;
typedef void *HBITMAP;
typedef void *HPALETTE;
typedef void *PIC;

typedef int		BOOL;
typedef unsigned char	UCHAR;
typedef unsigned char	BYTE;
typedef unsigned short	WORD;
typedef unsigned short	USHORT;
typedef unsigned int	DWORD;
typedef unsigned int	UINT;

typedef UINT		WPARAM;
typedef long		LPARAM;

typedef long LONG;

typedef void		*COLOR;
typedef void		*DEVCONTEXT;
typedef void		*DIR;
typedef void		*FILESPEC;
typedef void		*FONT;
typedef void		*INSTANCE;
typedef void		*MENU;
typedef void		*MYBITMAP;
typedef void		*PALETTE;
typedef void		*PIC;
typedef void		*PICBUFFER;
typedef void		*SPRITELIST;
typedef void		*REGION;
typedef void		*TEXT;
typedef void            *HDC;
typedef void		*TW_OOT_EVENT;
typedef void		*WIND;
typedef void            *HANDLE;
#define HWND WIND

typedef void *LPVOID;
typedef unsigned long SIZE_T;
typedef long LONG_PTR;
typedef LONG_PTR LRESULT;
typedef const char *LPCSTR;

#define __stdcall
#define CALLBACK

typedef struct _RECT {
  LONG left;
  LONG top;
  LONG right;
  LONG bottom;
} RECT, *PRECT;

typedef struct _FILETIME {
  DWORD dwLowDateTime;
  DWORD dwHighDateTime;
} FILETIME, *PFILETIME;

typedef struct _PROCESS_MEMORY_COUNTERS {
  DWORD  cb;
  DWORD  PageFaultCount;
  SIZE_T PeakWorkingSetSize;
  SIZE_T WorkingSetSize;
  SIZE_T QuotaPeakPagedPoolUsage;
  SIZE_T QuotaPagedPoolUsage;
  SIZE_T QuotaPeakNonPagedPoolUsage;
  SIZE_T QuotaNonPagedPoolUsage;
  SIZE_T PagefileUsage;
  SIZE_T PeakPagefileUsage;
} PROCESS_MEMORY_COUNTERS, *PPROCESS_MEMORY_COUNTERS;

typedef struct _SYSTEMTIME {
  WORD wYear;
  WORD wMonth;
  WORD wDayOfWeek;
  WORD wDay;
  WORD wHour;
  WORD wMinute;
  WORD wSecond;
  WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME;

#endif // __STUB_WINDOWS_H__
