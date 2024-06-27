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

#include <cstdlib>
#include "SpeedTreeInstance.h"
#include "SpeedTreeWrapper.h"
#include "IdvVectorMath.h"
#include <math.h>


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeInstance::CSpeedTreeInstance

CSpeedTreeInstance::CSpeedTreeInstance( ) :
    m_pInstanceOf(NULL),
    m_fUpAxisCenter(0.0f),
    m_fRotation(0.0f),
    m_fScale(1.0f),
    m_bCulled(false),
    m_fWindMatrixOffset(0.0f),
    m_fLod(1.0f),
    m_nBranchLodLevel(0),
    m_fBranchAlphaValue(84.0f / 255.0f),
    m_nFrondLodLevel(0),
    m_fFrondAlphaValue(84.0f / 255.0f),
    m_bBillboardIsActive(false),
    m_fBillboardFadeOut(0.0f)
{
    m_anLeafLodLevels[0] = m_anLeafLodLevels[1] = 0;
    m_afLeafAlphaValues[0] = m_afLeafAlphaValues[1] = 84.0f / 255.0f;

    m_afPos[0] = m_afPos[1] = m_afPos[2] = 0.0f;

    m_afRotationTrigValues[0] = sinf(m_fFrondAlphaValue);
    m_afRotationTrigValues[1] = cosf(m_fFrondAlphaValue);
    m_afRotationTrigValues[2] = -m_afRotationTrigValues[0];
    m_afRotationTrigValues[3] = 0.0f;
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeInstance::~CSpeedTreeInstance

CSpeedTreeInstance::~CSpeedTreeInstance( )
{
    m_pInstanceOf = NULL;
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeInstance::SetPos

void CSpeedTreeInstance::SetPos(const float* pPos)
{
    st_assert(pPos); 
    memcpy(m_afPos, pPos, 3 * sizeof(float));

    // need to also know the center of the tree along the up axis
    if (m_pInstanceOf)
    {
        float afBoundingBox[6];
        m_pInstanceOf->GetBoundingBox(afBoundingBox);
#ifdef UPVECTOR_POS_Y
        m_fUpAxisCenter = m_afPos[1] + m_fScale * 0.5f * (afBoundingBox[4] + afBoundingBox[1]);
#else
        m_fUpAxisCenter = m_afPos[2] + m_fScale * 0.5f * (afBoundingBox[5] + afBoundingBox[2]);
#endif
    }
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeInstance::SetRotation

void CSpeedTreeInstance::SetRotation(float fRotation)
{
    // can't use a negative rotation value (billboard shader optimization)
    if (fRotation < 0.0f)
        fRotation = fmod(fRotation, idv::TwoPi) + idv::TwoPi;

    m_fRotation = fRotation;
    m_afRotationTrigValues[0] = sinf(fRotation);    
    m_afRotationTrigValues[1] = cosf(fRotation);    
    m_afRotationTrigValues[2] = -m_afRotationTrigValues[0];
    m_afRotationTrigValues[3] = 0.0f;   
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeInstance::UpdateLod

void CSpeedTreeInstance::UpdateLod(float fLod)
{
    st_assert(m_pInstanceOf);

    m_fLod = fLod;
    if (fLod <= 0.0f)
    {
        m_fBillboardFadeOut = 1.0f;
        m_bBillboardIsActive = true;
    }
    else
    {
        CSpeedTreeRT::SLodValues sLod;

        m_pInstanceOf->GetLodValues(sLod, fLod);

        // branches
        m_nBranchLodLevel = char(sLod.m_nBranchActiveLod);
        m_fBranchAlphaValue = sLod.m_fBranchAlphaTestValue;
        st_assert(m_nBranchLodLevel > -1);

        // fronds
        m_nFrondLodLevel = char(sLod.m_nFrondActiveLod);
        m_fFrondAlphaValue = sLod.m_fFrondAlphaTestValue;
        st_assert(m_nFrondLodLevel > -1);

        // leaves
        if (sLod.m_anLeafActiveLods[0] > -1)
        {
            m_anLeafLodLevels[0] = char(sLod.m_anLeafActiveLods[0]);
            m_afLeafAlphaValues[0] = sLod.m_afLeafAlphaTestValues[0];
        }
        else
            m_anLeafLodLevels[0] = -1;

        if (sLod.m_anLeafActiveLods[1] > -1)
        {
            m_anLeafLodLevels[1] = char(sLod.m_anLeafActiveLods[1]);
            m_afLeafAlphaValues[1] = sLod.m_afLeafAlphaTestValues[1];
        }
        else
            m_anLeafLodLevels[1] = -1;

        // billboard
        m_fBillboardFadeOut = sLod.m_fBillboardFadeOut;
        m_bBillboardIsActive = m_fBillboardFadeOut > 0.0f;
    }
}

