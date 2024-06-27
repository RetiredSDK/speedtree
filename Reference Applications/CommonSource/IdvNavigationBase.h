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

#include <string.h>
#include "IdvVectorMath.h"
#include "SpeedTreeForest.h"


///////////////////////////////////////////////////////////////////////  
//  CNavigationBase
//
//  Geared mostly for terrain-style navigation, not object inspection

class CNavigationBase  
{
public: 
        enum EMouseButton
        {
            BUTTON_NONE, BUTTON_LEFT, BUTTON_MIDDLE, BUTTON_RIGHT, BUTTON_COUNT
        };

        CNavigationBase(void);
virtual ~CNavigationBase(void);

virtual bool            Advance(float fFrameTime, const CSpeedTreeForest* pForest = NULL);
virtual float           GetAzimuth(void) const          { return 0.0f; }
virtual const float*    GetCameraPos(void) const        { return m_afCameraPos; }
virtual const float*    GetCameraTransform(void) = 0;
virtual void            SetCameraPos(const float* pPos) { memcpy(m_afCameraPos, pPos, 3 * sizeof(float)); }
virtual void            MouseButton(EMouseButton eButton, bool bPressed, int nX, int nY);
virtual void            MouseMotion(int nX, int nY);
virtual void            SetSpeedScales(float fTranslate = -1.0f, float fRotate = -1.0f);
virtual void            SetShiftKeyState(bool bDown)    { m_bShiftDown = bDown; }
virtual void            SetControlKeyState(bool bDown)  { m_bControlDown = bDown; }

protected:
        // orientation
        float           m_afCameraPos[4];
        idv::Transform  m_cTransform;
        
        // modifiers
        bool            m_bShiftDown;
        bool            m_bControlDown;

        // speeds
        float           m_fTranslateSpeed;
        float           m_fRotateSpeed;
};

