///////////////////////////////////////////////////////////////////////  
//
//  *** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with 
//  the terms of that agreement.
//
//      Copyright (c) 2003-2007 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      Web: http://www.idvinc.com

#pragma once
#include <stdio.h>
#include <cstdlib>
#include <cstdarg>
#include "SpeedTreeMemory.h"
#if defined(WIN32) && !defined(_XBOX)
#include <windows.h>
#endif
#ifdef PS3
#include <errno.h>
#endif

#ifdef REPORT_TIMES
#include "IdvTimer.h"
#endif
//#define REPORT_TO_FILE


////////////////////////////////////////////////////////////
// IdvInterpolate

template <class T> inline T IdvInterpolate(const T& tStart, const T& tEnd, float fPercent)
{
    return static_cast<T>((tStart + (tEnd - tStart) * fPercent));
}


////////////////////////////////////////////////////////////
// IdvClamp

template <class T> inline T IdvClamp(T tValue, T tMinValue, T tMaxValue)
{
    T tReturn = st_min(tValue, tMaxValue);
    return st_max(tReturn, tMinValue);
}


///////////////////////////////////////////////////////////////////////  
//  IdvFileExists

inline bool IdvFileExists(const st_string& strFilename)
{
    FILE* pFile = fopen(strFilename.c_str( ), "r");
    bool bExists = (pFile != NULL);
    if (pFile)
        fclose(pFile);

    return bExists;
}


///////////////////////////////////////////////////////////////////////  
//  IdvFormatString

inline st_string IdvFormatString(const char* pField, ...)
{
    va_list vlArgs;
    char szBuffer[4096];

    va_start(vlArgs, pField);
    (void) vsprintf(szBuffer, pField, vlArgs);

    return szBuffer;
}


////////////////////////////////////////////////////////////
// IdvError

inline void IdvError(const char* pField, ...)
{
    va_list vlArgs;
    char szBuffer[4096];

    va_start(vlArgs, pField);
    (void) vsprintf(szBuffer, pField, vlArgs);

#if defined(WIN32) && !defined(_XBOX)
    (void) MessageBoxA(NULL, szBuffer, "SpeedTree ERROR", MB_ICONERROR | MB_OK);
#else
    fprintf(stderr, "\nSpeedTree ERROR: %s\n", szBuffer);
#endif
}


////////////////////////////////////////////////////////////
// IdvWarning

inline void IdvWarning(const char* pField, ...)
{
    va_list vlArgs;
    char szBuffer[4096];

    va_start(vlArgs, pField);
    (void) vsprintf(szBuffer, pField, vlArgs);

    // do something with the warning
    fprintf(stderr, "SpeedTree WARNING: %s\n", szBuffer);
}


////////////////////////////////////////////////////////////
// IdvInternal

inline void IdvInternal(const char* pField, ...)
{
    va_list vlArgs;
    char szBuffer[4096];

    va_start(vlArgs, pField);
    (void) vsprintf(szBuffer, pField, vlArgs);

    // do something with the warning
#if defined(WIN32) && !defined(_XBOX)
    (void) MessageBoxA(NULL, szBuffer, "SpeedTree INTERNAL ERROR", MB_ICONERROR | MB_OK);
#else
    fprintf(stderr, "SpeedTree INTERNAL ERROR: %s\n", szBuffer);
#endif
}


////////////////////////////////////////////////////////////
// IdvReport

inline void IdvReport(const char* pField, ...)
{
    va_list vlArgs;
    char szBuffer[4096];

    va_start(vlArgs, pField);
    (void) vsprintf(szBuffer, pField, vlArgs);

    // just print, but replace with whatever you need
    printf("\tInfo: %s\n", szBuffer);
#ifdef REPORT_TO_FILE
    FILE* pFile = fopen("game:\\idv_times.txt", "a");
    if (pFile)
    {
        fprintf(pFile, "%s\n", szBuffer);
        fclose(pFile);
    }
#endif
}


/////////////////////////////////////////////////////////////////////////////
//  IdvColorToUInt

inline unsigned int IdvColorToUInt(float fR, float fG, float fB, float fA = 1.0f)
{
    unsigned int uiRed = (unsigned int) (fR * 255.0f);
    unsigned int uiGreen = (unsigned int) (fG * 255.0f);
    unsigned int uiBlue = (unsigned int) (fB * 255.0f);
    unsigned int uiAlpha = (unsigned int) (fA * 255.0f);

    return ((uiRed << 0) + (uiGreen << 8) + (uiBlue << 16) + (uiAlpha << 24));
}


/////////////////////////////////////////////////////////////////////////////
//  IdvColorToUInt

inline unsigned int IdvColorToUInt(const float* pColor)
{
    return IdvColorToUInt(pColor[0], pColor[1], pColor[2], pColor[3]);
}


/////////////////////////////////////////////////////////////////////////////
//  IdvColorToFloats

inline void IdvColorToFloats(unsigned int uiColor, float* pColor)
{
    pColor[0] = ((uiColor & 0x000000FF) >> 0) / 255.0f;
    pColor[1] = ((uiColor & 0x0000FF00) >> 8) / 255.0f;
    pColor[2] = ((uiColor & 0x00FF0000) >> 16) / 255.0f;
    pColor[3] = ((uiColor & 0xFF000000) >> 24) / 255.0f;
}


////////////////////////////////////////////////////////////
// st_min & st_max
//
// Some platforms use max and min, others use st_max and st_min - we're using
// our own definitions

#define st_max(a,b)  (((a) > (b)) ? (a) : (b))
#define st_min(a,b)  (((a) < (b)) ? (a) : (b))


////////////////////////////////////////////////////////////
// st_assert

#ifdef _DEBUG
    // assertion macro
    #define st_assert(a) \
        if (!(a))        \
        {                \
            IdvInternal("Assertion failed\nLine %d, file: %s\n", __LINE__, __FILE__); \
            abort( ); \
        }
#else
    //  If in release mode, st_assert is defined as a no op.
    #define st_assert(x)
#endif

