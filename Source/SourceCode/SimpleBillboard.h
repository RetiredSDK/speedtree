///////////////////////////////////////////////////////////////////////  
//  SimpleBillboard.h
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
#include "Debug.h"
#include "LibVector_Source/IdvVector.h"
#include "UpVector.h"
#include "SpeedTreeRT.h"


///////////////////////////////////////////////////////////////////////  
//  class CSimpleBillboard declaration

class CSimpleBillboard
{
public:
        CSimpleBillboard( );

static  void            ComputeUnitBillboard(float fAzimuth);
        void            GetTransformedBillboard(float fWidth, float fHeight, CSpeedTreeRT::SGeometry::S360Billboard& sBillboard);
static  void            GetLightingAttributes(CSpeedTreeRT::SGeometry::S360Billboard& sBillboard);

        void            Invalidate(void)                            { m_bValid = false; }
        bool            IsValid(void) const                         { return m_bValid; }
        void            Validate(void)                              { m_bValid = true; }

private:
        float           m_acTreeBillboard[12];      // used as a buffer for holding the scaled billboard coordinates
        //lint esym(1401, m_acTreeBillboard)  
static  stVec3          m_acUnitBillboard[4];       // unitized quad computed to face the camera - all various sized
                                                    // billboards are scaled from this one billboard
static  stVec3          m_acNormals[4];
static  stVec3          m_acBinormals[4];
static  stVec3          m_acTangents[4];

        bool            m_bValid;                   // used billboarding computations to eliminate redundant computations
};
