///////////////////////////////////////////////////////////////////////  
//  SimpleBillboard.cpp
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

#include "SimpleBillboard.h"
#include "UpVector.h"


// static variables
stVec3 CSimpleBillboard::m_acUnitBillboard[4];
stVec3 CSimpleBillboard::m_acNormals[4];
stVec3 CSimpleBillboard::m_acTangents[4];
stVec3 CSimpleBillboard::m_acBinormals[4];


///////////////////////////////////////////////////////////////////////  
//  CSimpleBillboard::CSimpleBillboard definition

CSimpleBillboard::CSimpleBillboard( ) :
    m_bValid(false)
{
    //lint -esym(1401, CSimpleBillboard::m_acTreeBillboard)  { it's static, so it's initialized above }
}


///////////////////////////////////////////////////////////////////////  
//  CSimpleBillboard::ComputeUnitBillboard definition

void CSimpleBillboard::ComputeUnitBillboard(float fAzimuth)
{
    // build rotation matrix
    stRotTransform cTrans;
#ifdef UPVECTOR_POS_Z
    cTrans.RotateZ(fAzimuth);
#endif
#ifdef UPVECTOR_NEG_Z
    cTrans.RotateZ(-fAzimuth);
#endif
#ifdef UPVECTOR_POS_Y
    cTrans.RotateY(fAzimuth);
#endif
#ifdef UPVECTOR_DIRECTX_RIGHT_HANDED_COORDINATE_SYSTEM
    cTrans.RotateZ(-fAzimuth);
#endif

    // compute rotated vertex attributes
    const stVec3 c_acCorners[4] = { stVec3(0.0f, 0.5f, 1.0f), stVec3(0.0f, -0.5f, 1.0f), stVec3(0.0f, -0.5f, 0.0f), stVec3(0.0f, 0.5f, 0.0f) };
    const stVec3 c_acNormals[4] = { stVec3(1.0f, 0.0f, 0.0f), stVec3(1.0f, 0.0f, 0.0f), stVec3(1.0f, 0.0f, 0.0f), stVec3(1.0f, 0.0f, 0.0f) };
    const stVec3 c_acTangents[4] = { stVec3(0.0f, 1.0f, 0.0f), stVec3(0.0f, 1.0f, 0.0f), stVec3(0.0f, 1.0f, 0.0f), stVec3(0.0f, 1.0f, 0.0f) };
    const stVec3 c_acBinormals[4] = { stVec3(0.0f, 0.0f, 1.0f), stVec3(0.0f, 0.0f, 1.0f), stVec3(0.0f, 0.0f, 1.0f), stVec3(0.0f, 0.0f, 1.0f) };
    stVec3 cUpAxisAdjusted;
    for (int i = 0; i < 4; ++i)
    {
        // coordinate
        Assign3d(cUpAxisAdjusted, c_acCorners[i]);
        m_acUnitBillboard[i] = cUpAxisAdjusted * cTrans;

        // normal
        Assign3d(cUpAxisAdjusted, c_acNormals[i]);
        m_acNormals[i] = cUpAxisAdjusted * cTrans;

        // binormal
        Assign3d(cUpAxisAdjusted, c_acBinormals[i]);
        m_acBinormals[i] = cUpAxisAdjusted * cTrans;

        // tangent
        Assign3d(cUpAxisAdjusted, c_acTangents[i]);
        m_acTangents[i] = cUpAxisAdjusted * cTrans;
    }
}


///////////////////////////////////////////////////////////////////////  
//  CSimpleBillboard::GetTransformedBillboard definition

void CSimpleBillboard::GetTransformedBillboard(float fWidth, float fHeight, CSpeedTreeRT::SGeometry::S360Billboard& sBB)
{
    if (!IsValid( ))
    {
#ifdef UPVECTOR_POS_Y
        float afScale[3] = { fWidth, fHeight, fWidth };
#else
        float afScale[3] = { fWidth, fWidth, fHeight };
#endif

        // scale coordinates
        m_acTreeBillboard[0] = afScale[0] * m_acUnitBillboard[0][0];
        m_acTreeBillboard[1] = afScale[1] * m_acUnitBillboard[0][1];
        m_acTreeBillboard[2] = afScale[2] * m_acUnitBillboard[0][2];
        m_acTreeBillboard[3] = afScale[0] * m_acUnitBillboard[1][0];
        m_acTreeBillboard[4] = afScale[1] * m_acUnitBillboard[1][1];
        m_acTreeBillboard[5] = afScale[2] * m_acUnitBillboard[1][2];
        m_acTreeBillboard[6] = afScale[0] * m_acUnitBillboard[2][0];
        m_acTreeBillboard[7] = afScale[1] * m_acUnitBillboard[2][1];
        m_acTreeBillboard[8] = afScale[2] * m_acUnitBillboard[2][2];
        m_acTreeBillboard[9] = afScale[0] * m_acUnitBillboard[3][0];
        m_acTreeBillboard[10] = afScale[1] * m_acUnitBillboard[3][1];
        m_acTreeBillboard[11] = afScale[2] * m_acUnitBillboard[3][2];

        Validate( );
    }

    sBB.m_pCoords = &m_acTreeBillboard[0];

    // point to other transformed vertex attributes
    sBB.m_pNormals = &m_acNormals[0][0];
    sBB.m_pBinormals = &m_acBinormals[0][0];
    sBB.m_pTangents = &m_acTangents[0][0];
}


///////////////////////////////////////////////////////////////////////  
//  CSimpleBillboard::GetLightingAttributes definition

void CSimpleBillboard::GetLightingAttributes(CSpeedTreeRT::SGeometry::S360Billboard& sBillboard)
{
    sBillboard.m_pNormals = &m_acNormals[0][0];
    sBillboard.m_pBinormals = &m_acBinormals[0][0];
    sBillboard.m_pTangents = &m_acTangents[0][0];
}




