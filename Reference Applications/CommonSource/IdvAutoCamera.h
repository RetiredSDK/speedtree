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
//  CAutoCamera
//
//  Implements an occasionally changing automatically generated camera paths

class CAutoCamera : public CNavigationBase
{
public: 
        CAutoCamera(void);
virtual ~CAutoCamera(void);

virtual bool            Advance(float fFrameTime, const CSpeedTreeForest* pForest);
virtual const float*    GetCameraTransform(void);
virtual float           GetAzimuth(void) const;

        void            SetExtents(idv::Region cExtents) { m_cExtents = cExtents; }

private:
        idv::Vec3       m_vecCameraPos;
        idv::Vec3       m_vecCameraData;
        idv::Vec3       m_vecCameraVelocity;
        float           m_fCameraRadius;
        idv::Vec3       m_vecTargetPos;
        idv::Vec3       m_vecTargetData;
        idv::Vec3       m_vecTargetVelocity;
        float           m_fTargetRadius;

        idv::Region     m_cExtents;

        enum ECameraTypes
        {
            GO, WANDERING_EYE, ORBIT, EXAMINE, NUM_CAMERA_TYPES
        };

        ECameraTypes    m_eType;

        float           m_fCumulativeTime;
        float           m_fNextAdvance;

        void            PickNewCamera(const CSpeedTreeForest* pForest);

        void            PickGo(void);
        void            Go(float fDeltaTime);

        void            PickWanderingEye(void);
        void            WanderingEye(float fDeltaTime);

        void            PickOrbit(void);
        void            Orbit(float fDeltaTime);

        void            PickExamine(const CSpeedTreeForest* pForest);
        void            Examine(float fDeltaTime);
};

