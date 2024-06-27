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

///////////////////////////////////////////////////////////////////////  
//  Include Files

#include <cstdlib>


///////////////////////////////////////////////////////////////////////  
//  IdvRandom (int)
//
//  nMin <= (return value) <= nMax

inline int IdvRandom(int nMin, int nMax)
{
    int nDiff = nMax - nMin + 1;

    return nMin + (rand( ) % nDiff);
}


///////////////////////////////////////////////////////////////////////  
//  IdvRandom (float)
//
//  fMin <= (return value) <= fMax

inline float IdvRandom(float fMin, float fMax)
{
    float fUnit = float(rand( )) / RAND_MAX;
    float fDiff = fMax - fMin;

    return fMin + fUnit * fDiff;
}
