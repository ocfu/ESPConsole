//
//__console.h
//
#pragma once

#ifdef NDEBUG
#warning "NDEBUG is defined"
#endif
#ifdef DEBUG
#warning "DEBUG is defined"
#endif

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#ifdef BUILD_ID
#pragma message("BUILD_ID is " STR(BUILD_ID))
#endif


#include <CxESPConsole.hpp>

extern const char* g_szBuildId PROGMEM;

#ifndef __SKIP_GLOBALS__
#define __SKIP_GLOBALS__

#pragma GCC push_options
#pragma GCC optimize ("O0")
#if defined (BUILD_ID) && defined (_VERSION)
#define _VERSION_ID _VERSION "(" STR(BUILD_ID) ")"
#elif defined (_VERSION)
#define _VERSION_ID _VERSION
#else
#define _VERSION_ID "-"
#endif
// for the identification of the binary (e.g. for archiving purposes)
#if defined (_NAME) && defined (_VERSION_ID)
    const char* g_szId PROGMEM = "$$id:" _NAME ":" _VERSION_ID;

#ifdef BUILD_ID
const char* g_szBuildId PROGMEM = STR(BUILD_ID);
#else
const char* g_szBuildId PROGMEM = "";
#endif

#endif
#include "../version.h"

#if defined (LIB_VERSION)
extern const char* g_szLib;
#endif
#ifndef _NAME
#define _NAME "App"
#endif



#pragma GCC pop_options
#endif /*__SKIP_GLOBALS__ */


void initESPConsole(const char* app = _NAME, const char* ver = _VERSION_ID);
