#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#if defined(_WIN32) || defined(WIN32) 
#define CX_PLATFORM_WIN32
#endif

#if defined(__linux__)
#error "Linux platform not implemented yet"
//#define CX_PLATFORM_LINUX
#endif

#if defined(__APPLE__) || defined(MACOSX)
#error "MacOS platform not implemented yet"
//#define CX_PLATFORM_MACOSX
#endif

#if !defined(CX_PLATFORM_WIN32) && !defined(CX_PLATFORM_LINUX) && !defined(CX_PLATFORM_MACOSX)
#error "Unsupported platform"
#endif

#endif // _PLATFORM_H_
