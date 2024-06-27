///////////////////////////////////////////////////////////////////////  
//  IdvFastMath.h
//
//  *** INTERACTIVE DATA VISUALIZATION (IDV) PROPRIETARY INFORMATION ***
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Interactive Data Visualization and may
//  not be copied or disclosed except in accordance with the terms of
//  that agreement.
//
//      Copyright (c) 2003-2005 IDV, Inc.
//      All Rights Reserved.
//
//      IDV, Inc.
//      http://www.idvinc.com
        
#pragma once 
#ifdef PS3
#include <math.h>
#else
#include <cmath>
#endif


///////////////////////////////////////////////////////////////////////  
//  _idv_sqrt1 definition

inline float _idv_sqrt1(float fVal)
{
#ifdef PS3
    return sqrt(fVal);
#else
    int i = ((*reinterpret_cast<int*>(&fVal)) >> 1 ) + 0x1fc00000; 

    return *reinterpret_cast<float*>(&i);
#endif
}

