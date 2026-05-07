// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#ifdef _WIN32

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>       // MFC support for Internet Explorer 4 Common Controls
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>         // MFC support for Windows Common Controls
#endif

#include <afxcontrolbars.h> // MFC support for ribbons and control bars

#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif

#else

#include <iostream>
#include <string>
#include <thread>
#include <type_traits>

#define _T(value) value

constexpr unsigned int MB_OK = 0;
constexpr unsigned int MB_ICONERROR = 0x00000010;

using CString = std::string;

inline int AfxMessageBox(const char *message, unsigned int = 0)
{
	std::cerr << message << std::endl;
	return 0;
}

inline int AfxMessageBox(const std::string &message, unsigned int = 0)
{
	std::cerr << message << std::endl;
	return 0;
}

using HANDLE = void *;

inline HANDLE GetCurrentThread()
{
	return nullptr;
}

inline unsigned long long SetThreadAffinityMask(HANDLE, long long)
{
	return 0;
}

template <typename A, typename B>
inline typename std::common_type<A, B>::type min(A a, B b)
{
	typedef typename std::common_type<A, B>::type Result;
	return a < b ? static_cast<Result>(a) : static_cast<Result>(b);
}

template <typename A, typename B>
inline typename std::common_type<A, B>::type max(A a, B b)
{
	typedef typename std::common_type<A, B>::type Result;
	return a > b ? static_cast<Result>(a) : static_cast<Result>(b);
}

struct SYSTEM_INFO {
	unsigned int dwNumberOfProcessors;
};

inline void GetSystemInfo(SYSTEM_INFO *systemInfo)
{
	systemInfo->dwNumberOfProcessors = std::thread::hardware_concurrency();
	if (systemInfo->dwNumberOfProcessors == 0) {
		systemInfo->dwNumberOfProcessors = 1;
	}
}

#endif
