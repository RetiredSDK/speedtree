///////////////////////////////////////////////////////////////////////  
//
//  *** INTERACTIVE DATA VISUALIZATION (IDV) PROPRIETARY INFORMATION ***
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Interactive Data Visualization and may
//  not be copied or disclosed except in accordance with the terms of
//  that agreement.
//
//      Copyright (c) 2003-2007 IDV, Inc.
//      All Rights Reserved.
//
//      IDV, Inc.
//      Web: http://www.idvinc.com


///////////////////////////////////////////////////////////////////////  
//  Include Files

#include "IdvConsoleNavigation.h"
#include "IdvGlobal.h"
#include <math.h>


///////////////////////////////////////////////////////////////////////  
//  CNavigation::CNavigation

CNavigation::CNavigation( ) :
    m_fAzimuth(126.518),
    m_fPitch(-93.176f)
{
    // arbitrary camera position
    m_afCameraPos[0] = 0.0f;
    m_afCameraPos[1] = 0.0f;
    m_afCameraPos[2] = 0.0f;
}


///////////////////////////////////////////////////////////////////////  
//  CNavigation::GetCameraTransform

const float* CNavigation::GetCameraTransform(void)
{
    m_cTransform.LoadIdentity( );

#ifdef UPVECTOR_POS_Y
    m_cTransform.Rotate(m_fPitch + 90.0f, 'x');
    m_cTransform.Rotate(90.0f - m_fAzimuth, 'y');
#else // POS_Z
    m_cTransform.Rotate(m_fPitch, 'x');
    m_cTransform.Rotate(m_fAzimuth, 'z');
#endif
    m_cTransform.Translate(-m_afCameraPos[0], -m_afCameraPos[1], -m_afCameraPos[2]);

    return reinterpret_cast<float*>(m_cTransform.m_afData);
}


///////////////////////////////////////////////////////////////////////  
//  CNavigation::MoveFB

void CNavigation::MoveFB(float fDist)
{
    float fAmt = fDist * cosf(idv::DegToRad(m_fPitch + 90.0f));

#ifdef UPVECTOR_POS_Y
    m_afCameraPos[0] -= fAmt * sinf(idv::DegToRad(m_fAzimuth - 90.0f));
    m_afCameraPos[2] -= fAmt * cosf(idv::DegToRad(m_fAzimuth - 90.0f));
    m_afCameraPos[1] += fDist * sinf(idv::DegToRad(m_fPitch -90.0f));
#else
    m_afCameraPos[0] += fAmt * sinf(idv::DegToRad(m_fAzimuth));
    m_afCameraPos[1] += fAmt * cosf(idv::DegToRad(m_fAzimuth));
    m_afCameraPos[2] -= fDist * sinf(idv::DegToRad(m_fPitch + 90.0f));
#endif
}


///////////////////////////////////////////////////////////////////////  
//  CNavigation::MoveLR

void CNavigation::MoveLR(float fDist)
{
#ifdef UPVECTOR_POS_Y
    m_afCameraPos[0] += fDist * sinf(idv::DegToRad(m_fAzimuth));
    m_afCameraPos[2] += fDist * cosf(idv::DegToRad(m_fAzimuth));
#else
    m_afCameraPos[0] -= fDist * sinf(idv::DegToRad(m_fAzimuth - 90.0f));
    m_afCameraPos[1] -= fDist * cosf(idv::DegToRad(m_fAzimuth - 90.0f));
#endif
}


///////////////////////////////////////////////////////////////////////  
//  CNavigation::TurnLR

void CNavigation::TurnLR(float fDist)
{
#ifdef UPVECTOR_POS_Y
    m_fAzimuth -= fDist;
#else
    m_fAzimuth += fDist;
#endif
}


///////////////////////////////////////////////////////////////////////  
//  CNavigation::TurnUD

void CNavigation::TurnUD(float fDist)
{
    m_fPitch -= fDist;
    m_fPitch = st_min(st_max(m_fPitch, -175.0f), -5.0f);
}


