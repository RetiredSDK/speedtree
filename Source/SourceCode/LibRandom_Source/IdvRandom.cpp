///////////////////////////////////////////////////////////////////////  
//  IdvRandom.cpp
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

#include "IdvRandom.h"
#include "RobertDavies_Random.h"
#ifdef WIN32
#include <ctime>
#endif
using namespace std;

 
///////////////////////////////////////////////////////////////////////  
//  struct SIdvRandomImpl declaration
//
//  This structure contains the member variables of stRandom in order
//  to hide the implementation details (namely the header file) of the Robert
//  Davies random library.

struct SIdvRandomImpl
{
        SIdvRandomImpl( )
        {
        }

static  Uniform     m_cUniform;         // Davies' uniform number generator
static  bool        m_bInit;            // Flag for initializing Davies' library
};


// static variables

Uniform SIdvRandomImpl::m_cUniform;
bool SIdvRandomImpl::m_bInit = false;


///////////////////////////////////////////////////////////////////////  
//  stRandom::stRandom definition
//

stRandom::stRandom( )
{
    if (!SIdvRandomImpl::m_bInit)
        Reseed( );
}


///////////////////////////////////////////////////////////////////////  
//  stRandom::~stRandom definition
//

stRandom::~stRandom( )
{
}


///////////////////////////////////////////////////////////////////////  
//  stRandom::GetUniform definition
//

float stRandom::GetUniform(float fMin, float fMax)
{
    return fMin + (fMax - fMin) * static_cast<float>(SIdvRandomImpl::m_cUniform.Next( ));
}


///////////////////////////////////////////////////////////////////////  
//  stRandom::GetUniform definition

void stRandom::Reseed(long lSeed)
{
    if (lSeed == -1L) // automatically calc a seed
    {
        long nNewSeed = 12345L;
        if (SIdvRandomImpl::m_bInit)
            nNewSeed = static_cast<long>(GetUniform(0.0f, float(INT_MAX)));
            
#ifdef WIN32
        long nTimeValue = static_cast<long>(time((time_t* ) 0));
#else
        long nTimeValue = 1;
#endif
        
        if (nNewSeed > nTimeValue)
            Random::Set(double(nTimeValue) / nNewSeed);
        else
            Random::Set(double(nNewSeed) / nTimeValue);
    }
    else
    {
        if (lSeed <= 1L)
            lSeed = 1L;
        Random::SetLong(lSeed);
    }

    SIdvRandomImpl::m_bInit = true;
}


///////////////////////////////////////////////////////////////////////  
//  stRandom::GetUniform definition
//

long stRandom::GetSeed(void) const
{
    return Random::GetSeed( );
}

