///////////////////////////////////////////////////////////////////////  
//  CIdvTimer Class
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

#include <time.h>
#include "IdvTimer.h"
#include <cstdio>


///////////////////////////////////////////////////////////////////////  
//  CIdvTimer::CIdvTimer

CIdvTimer::CIdvTimer( )
{
    m_eTimerType = eClock;
#if defined(WIN32)
    if(QueryPerformanceFrequency(&m_liFreq))
        m_eTimerType = eHiRes;
    m_liStart.QuadPart = 0;
    m_liStop.QuadPart = 0;
#endif // WIN32
    m_iStart = 0;
    m_iStop = 0;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvTimer::IsHighResolutionAvailable

bool CIdvTimer::IsHighResolutionAvailable(void) const
{
    if(m_eTimerType == eHiRes) return true;
    return false;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvTimer::Start

void CIdvTimer::Start(void)
{
#if defined(WIN32)
    if(m_eTimerType == eHiRes)
    {
        m_liStart.QuadPart = 0;
        QueryPerformanceCounter(&m_liStart);
    }
    else
#endif // WIN32
    {
        m_iStart = clock( );
    }
}


///////////////////////////////////////////////////////////////////////  
//  CIdvTimer::Stop

void CIdvTimer::Stop(void)
{
#if defined(WIN32)
    if(m_eTimerType == eHiRes)
        QueryPerformanceCounter(&m_liStop);
    else
#endif // WIN32
        m_iStop = clock( );
}


///////////////////////////////////////////////////////////////////////  
//  CIdvTimer::GetMicroSec

double CIdvTimer::GetMicroSec(void) const
{
#if defined(WIN32)
    if(m_eTimerType == eHiRes)
        return ((m_liStop.QuadPart - m_liStart.QuadPart) * 1000000.0) / (double)m_liFreq.QuadPart;
    else
#endif // WIN32
        return (double)(m_iStop - m_iStart) / (double)CLOCKS_PER_SEC * 1000000.0;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvTimer::GetMilliSec

double CIdvTimer::GetMilliSec(void) const
{
#if defined(WIN32)
    if(m_eTimerType == eHiRes)
        return ((m_liStop.QuadPart - m_liStart.QuadPart) * 1000.0) / (double)m_liFreq.QuadPart;
    else
#endif // WIN32
        return ((double)(m_iStop - m_iStart) / (double)CLOCKS_PER_SEC) * 1000.0;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvTimer::GetSec

double CIdvTimer::GetSec(void) const
{
#if defined(WIN32)
    if(m_eTimerType == eHiRes)
    {
        return  ((double)(m_liStop.QuadPart - m_liStart.QuadPart)) / (double)m_liFreq.QuadPart;
    }
    else
#endif // WIN32
        return ((double)(m_iStop - m_iStart)) / (double)CLOCKS_PER_SEC;
}





