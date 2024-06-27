///////////////////////////////////////////////////////////////////////  
//  IdvGlobals.h
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
#if defined(WIN32) || defined(_XBOX)
#pragma warning (disable : 4786 4996)
#ifdef STRIP_EXCEPTION_HANDLING
#pragma warning (disable : 4530)
#endif
#endif

#include <stdexcept>
#include <cstdarg>
#include <vector>
#include "../SpeedTreeMemory.h"
// ps3 (vsprintf)
#include <stdio.h>

#ifdef PS3
#include <errno.h>
#endif


//
// IDV exception classes, derived from standard C++ exception classes
//

///////////////////////////////////////////////////////////////////////  
//  class IdvFileError definition

class IdvFileError : public st_string
{
public:
        IdvFileError(const st_string& strDetails, bool bAppendSystemError = false) : 
            st_string(strDetails + " [" + (bAppendSystemError ? (st_string(strerror(errno))) : "") + "]") { }
};


///////////////////////////////////////////////////////////////////////  
//  class IdvSystemError definition

class IdvSystemError : public st_string
{
public:
        IdvSystemError(const st_string& strDetails, bool bAppendSystemError = false) : 
            st_string(strDetails + " [" + (bAppendSystemError ? (st_string(strerror(errno))) : "") + "]") { }
};


///////////////////////////////////////////////////////////////////////  
//  class IdvRuntimeError definition

class IdvRuntimeError : public st_string
{
public:
        IdvRuntimeError(const st_string& strDetails, bool bAppendSystemError = false) : 
            st_string(strDetails + " [" + (bAppendSystemError ? (st_string(strerror(errno))) : "") + "]") { }
};


///////////////////////////////////////////////////////////////////////  
//  IDV type definitions

const int c_nIdvMaxStringSize = 1024;
typedef char IdvTmpString[c_nIdvMaxStringSize];


///////////////////////////////////////////////////////////////////////  
//  IDV global function definitions

static inline st_string IdvFormatString(const char* pField, ...)
{
    va_list vlArgs;
    IdvTmpString szBuffer;
    
    va_start(vlArgs, pField);
    (void) vsprintf(szBuffer, pField, vlArgs);

    return szBuffer;
}

