///////////////////////////////////////////////////////////////////////  
//  CIdvTimer
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
#if defined(WIN32)
#include <windows.h>
#endif // WIN32


///////////////////////////////////////////////////////////////////////  
//  class CIdvTimer

class CIdvTimer
{
public:
        CIdvTimer( );

        double          GetSec(void) const;
        double          GetMilliSec(void) const;
        double          GetMicroSec(void) const;
        void            Stop(void);
        void            Start(void);
        bool            IsHighResolutionAvailable(void) const;

private:
        enum
        { 
            eUnknown, eHiRes, eClock
        } m_eTimerType;

#ifdef WIN32
        LARGE_INTEGER m_liFreq;      // ticks per second
        LARGE_INTEGER m_liStart, m_liStop;
#endif // WIN32

        int             m_iStart;
        int             m_iStop;
};

