///////////////////////////////////////////////////////////////////////  
//  Debug.h
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
//      http://www.idvinc.com

#pragma once
#ifdef WIN32
#pragma warning (disable : 4702)
#endif
#include <cstdlib>
#include "SpeedTreeMemory.h"


//  The "st" in st_assert stands for SpeedTree.  If an assertion fails, it is mostly likely an internal
//  error, although user errors can easilty cause assertion failures (passing NULL pointers).

#ifdef _DEBUG
    // assertion macro
    #define st_assert(a) \
        if (!(a))        \
        {                \
            fprintf(stderr, "Assertion failed\nLine %d, file: %s\n", __LINE__, __FILE__); \
            abort( ); \
        }
#else
    //  If in release mode, st_assert is defined as a no op.
    #define st_assert(x)
#endif


//  Most exposed CSpeedTreeRT functions have a try and catch around the code as a precautionary
//  measure.  This macro is used to catch both standard C++ exceptions (those derived from the
//  expception class) and non-standard exceptions, caught by the catch all (...).

#ifdef STRIP_EXCEPTION_HANDLING
#define try
#define SpeedTreeCatch(FuncName)
#define SpeedTreeCatchAll(FuncName)
#else
#define SpeedTreeCatch(FuncName) \
    catch (exception& cException) \
    { \
        SetError(IdvFormatString("%s - failed [%s]", FuncName, cException.what( )).c_str( )); \
    }

#define SpeedTreeCatchAll(FuncName) \
    catch (...) \
    { \
        SetError(IdvFormatString("%s - threw an unknown system exception", FuncName).c_str( )); \
    }
#endif

// Some platforms use max and min, others use __max and __min - we're using
// our own definitions
#define st_max(a,b)  (((a) > (b)) ? (a) : (b))
#define st_min(a,b)  (((a) < (b)) ? (a) : (b))

template <class T> inline T st_clamp(T tValue, T tMinValue, T tMaxValue)
{
    T tReturn = st_min(tValue, tMaxValue);
    return st_max(tReturn, tMinValue);
}


