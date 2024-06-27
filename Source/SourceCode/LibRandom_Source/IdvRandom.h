///////////////////////////////////////////////////////////////////////  
//  IdvRandom.h
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
#include "../Debug.h"


#define BUILD_IDVRANDOM_SET
#include "../LibGlobals_Source/IdvGlobals.h"

struct SIdvRandomImpl;

///////////////////////////////////////////////////////////////////////  
//  class stRandom declaration

class BUILD_IDVRANDOM_SET stRandom 
{
public:
        stRandom( );
        ~stRandom( );

        float               GetUniform(float fMin, float fMax);
        void                Reseed(long lSeed = -1L);
        long                GetSeed(void) const;
};
