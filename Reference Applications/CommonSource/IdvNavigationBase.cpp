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


///////////////////////////////////////////////////////////////////////  
//  Include Files

#include "IdvNavigationBase.h"


///////////////////////////////////////////////////////////////////////  
//  CNavigationBase::CNavigationBase

CNavigationBase::CNavigationBase( ) :
    m_bShiftDown(false),
    m_bControlDown(false),
    m_fTranslateSpeed(0.08f),
    m_fRotateSpeed(0.15f)
{
    // arbitrary camera position
    m_afCameraPos[0] = 0.0f;
    m_afCameraPos[1] = 0.0f;
    m_afCameraPos[2] = 0.0f;
}


///////////////////////////////////////////////////////////////////////  
//  CNavigationBase::~CNavigationBase

CNavigationBase::~CNavigationBase( )
{
}

///////////////////////////////////////////////////////////////////////  
//  CNavigationBase::Advance
//
//  Returns feet / sec speed

bool CNavigationBase::Advance(float, const CSpeedTreeForest*)
{
    return false;
}


///////////////////////////////////////////////////////////////////////  
//  CNavigationBase::GetCameraTransform

const float* CNavigationBase::GetCameraTransform(void)
{
    m_cTransform.LoadIdentity( );

    return reinterpret_cast<float*>(m_cTransform.m_afData);
}


///////////////////////////////////////////////////////////////////////  
//  CNavigationBase::MouseButton

void CNavigationBase::MouseButton(EMouseButton, bool, int, int)
{
}


///////////////////////////////////////////////////////////////////////  
//  CNavigationBase::MouseMotion

void CNavigationBase::MouseMotion(int, int)
{
}


///////////////////////////////////////////////////////////////////////  
//  CNavigationBase::SetSpeedScales

void CNavigationBase::SetSpeedScales(float fTranslate, float fRotate)
{
    if (fTranslate != -1.0f)
        m_fTranslateSpeed = fTranslate;

    if (fRotate != -1.0f)
        m_fRotateSpeed = fRotate;
}

