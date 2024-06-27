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

#include "IdvMouseNavigation.h"
#include "UpVector.h"
#include <math.h>
#include <stdio.h>


///////////////////////////////////////////////////////////////////////  
//  Constants

const float c_fSpeedUpScale = 3.0f;
const float c_fSlowDownScale = 0.15f;


///////////////////////////////////////////////////////////////////////  
//  CMouseNavigation::CMouseNavigation

CMouseNavigation::CMouseNavigation( ) :
    m_fAzimuth(180.0f),
    m_fPitch(-90.0f),
    m_eMouseButton(BUTTON_NONE),
    m_nBaseX(0),
    m_nBaseY(0),
    m_nDeltaX(0),
    m_nDeltaY(0),
    m_nLastX(0),
    m_nLastY(0)
{
    // mouse pressed state
    for (int nButton = BUTTON_NONE; nButton < BUTTON_COUNT; ++nButton)
        m_abMousePressed[nButton] = false;
}


///////////////////////////////////////////////////////////////////////  
//  CMouseNavigation::Advance
//
//  Returns feet / sec speed

bool CMouseNavigation::Advance(float fFrameTime, const CSpeedTreeForest*)
{
    if (m_eMouseButton == BUTTON_LEFT)
    {
        const float c_fModifierScale = (m_bShiftDown ? c_fSpeedUpScale : 1.0f) * (m_bControlDown ? c_fSlowDownScale : 1.0f);

        // camera continues to move in direction set in MouseMotion()
#ifdef UPVECTOR_POS_Y
        m_afCameraPos[0] -= c_fModifierScale * m_fTranslateSpeed * fFrameTime * m_nDeltaY * cosf(idv::DegToRad(m_fAzimuth));
        m_afCameraPos[2] += c_fModifierScale * m_fTranslateSpeed * fFrameTime * m_nDeltaY * sinf(idv::DegToRad(m_fAzimuth));
#else // POS_Z
        m_afCameraPos[0] -= c_fModifierScale * m_fTranslateSpeed * fFrameTime * m_nDeltaY * cosf(idv::DegToRad(m_fAzimuth));
        m_afCameraPos[1] -= c_fModifierScale * m_fTranslateSpeed * fFrameTime * m_nDeltaY * sinf(idv::DegToRad(m_fAzimuth));
#endif
    }

    return false;
}


///////////////////////////////////////////////////////////////////////  
//  CMouseNavigation::MouseButton

void CMouseNavigation::MouseButton(EMouseButton eButton, bool bPressed, int nX, int nY)
{
    m_eMouseButton = eButton;
    m_abMousePressed[eButton] = bPressed;

    if (bPressed)
    {
        m_nDeltaX = 0;
        m_nDeltaY = 0;
        m_nBaseX = nX;
        m_nBaseY = nY;
    }

    m_nLastX = nX;
    m_nLastY = nY;
}


///////////////////////////////////////////////////////////////////////  
//  CMouseNavigation::MouseMotion

void CMouseNavigation::MouseMotion(int nX, int nY)
{
    const float c_fModifierScale = (m_bShiftDown ? c_fSpeedUpScale : 1.0f) * (m_bControlDown ? c_fSlowDownScale : 1.0f);
    const float c_fDeg2Rad = 57.2957795f;

    if (m_abMousePressed[BUTTON_LEFT] && m_abMousePressed[BUTTON_MIDDLE])
    {
        m_afCameraPos[0] -= (nX - m_nLastX) * cosf((m_fAzimuth + 90.0f) / c_fDeg2Rad);
        m_afCameraPos[1] -= (nX - m_nLastX) * sinf((m_fAzimuth + 90.0f) / c_fDeg2Rad);
    }
    else if (m_abMousePressed[BUTTON_MIDDLE])
    {
        m_fPitch += (nY - m_nLastY) * m_fRotateSpeed;
    }
    else if (m_abMousePressed[BUTTON_LEFT])
    {
        m_fAzimuth -= (nX - m_nLastX) * m_fRotateSpeed;
        m_nDeltaY = nY - m_nBaseY;
    }
    else if (m_abMousePressed[BUTTON_RIGHT])
    {
        const float c_fStrafeFactor = 0.5f;
#ifdef UPVECTOR_POS_Y
        m_afCameraPos[1] -= (nY - m_nLastY) * c_fModifierScale * m_fTranslateSpeed * 0.5f;
        m_afCameraPos[0] += (nX - m_nLastX) * c_fModifierScale * c_fStrafeFactor * cosf((90.0f - m_fAzimuth) / c_fDeg2Rad);
        m_afCameraPos[2] += (nX - m_nLastX) * c_fModifierScale * c_fStrafeFactor * sinf((90.0f - m_fAzimuth) / c_fDeg2Rad);
#else
        m_afCameraPos[2] -= (nY - m_nLastY) * c_fModifierScale * m_fTranslateSpeed * 0.5f;
        m_afCameraPos[0] -= (nX - m_nLastX) * c_fModifierScale * m_fTranslateSpeed * 0.5f * c_fStrafeFactor * cosf((m_fAzimuth + 90.0f) / c_fDeg2Rad);
        m_afCameraPos[1] -= (nX - m_nLastX) * c_fModifierScale * m_fTranslateSpeed * 0.5f * c_fStrafeFactor * sinf((m_fAzimuth + 90.0f) / c_fDeg2Rad);
#endif
    }

    m_nLastX = nX;
    m_nLastY = nY;
}


///////////////////////////////////////////////////////////////////////  
//  CMouseNavigation::GetCameraTransform

const float* CMouseNavigation::GetCameraTransform(void)
{
    m_cTransform.LoadIdentity( );

#ifdef UPVECTOR_POS_Y
    m_cTransform.Rotate(m_fPitch + 90.0f, 'x');
    m_cTransform.Rotate(90.0f - m_fAzimuth, 'y');
#else // POS_Z
    m_cTransform.Rotate(m_fPitch, 'x');
    m_cTransform.Rotate(90.0f - m_fAzimuth, 'z');
#endif
    m_cTransform.Translate(-m_afCameraPos[0], -m_afCameraPos[1], -m_afCameraPos[2]);

    return reinterpret_cast<float*>(m_cTransform.m_afData);
}


