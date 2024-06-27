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

#pragma once

///////////////////////////////////////////////////////////////////////  
//  Include Files

#include "IdvNavigationBase.h"


///////////////////////////////////////////////////////////////////////  
//  CMouseNavigation
//
//  Geared mostly for terrain-style navigation, not object inspection

class CMouseNavigation : public CNavigationBase  
{
public: 
        CMouseNavigation( );

        bool            Advance(float fFrameTime, const CSpeedTreeForest* pForest);
        float           GetAzimuth(void) const              { return 90.0f - m_fAzimuth; }
        const float*    GetCameraTransform(void);
        void            MouseButton(EMouseButton eButton, bool bPressed, int nX, int nY);
        void            MouseMotion(int nX, int nY);

private:
        // orientation
        float           m_fAzimuth;
        float           m_fPitch;

        // mouse mechanics
        EMouseButton    m_eMouseButton;
        bool            m_abMousePressed[BUTTON_COUNT];
        int             m_nBaseX;
        int             m_nBaseY;
        int             m_nDeltaX;
        int             m_nDeltaY;
        int             m_nLastX;
        int             m_nLastY;
};

