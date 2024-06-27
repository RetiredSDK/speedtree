///////////////////////////////////////////////////////////////////////  
//  Camera.cpp
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

#include "Debug.h"
#include "TreeEngine.h"


// static variables

stVec3 stCamera::m_cCameraPos(0.0f, 0.0f, 0.0f);
stVec3 stCamera::m_cCameraDir(0.577f, 0.577f, 0.577f);


///////////////////////////////////////////////////////////////////////  
//  stCamera::stCamera function

stCamera::stCamera( ) : 
    m_cPosition(0.0f, 0.0f, 0.0f)
{
}


///////////////////////////////////////////////////////////////////////  
//  stCamera::stCamera function

stCamera::stCamera(const stCamera& cRightSide)
{
    *this = cRightSide;
}


///////////////////////////////////////////////////////////////////////  
//  stCamera::operator= function

const stCamera& stCamera::operator=(const stCamera& cRight)
{
    if (&cRight != this)
        m_cPosition = cRight.m_cPosition;

    return *this;
}
