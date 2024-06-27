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

#include "IdvAutoCamera.h"
#include "UpVector.h"
#include <math.h>
#include <stdio.h>
#include "IdvRandom.h"


#ifdef UPVECTOR_POS_Y

#define X 0
#define Y 2
#define Z 1

#else

#define X 0
#define Y 1
#define Z 2

#endif

#define AZIMUTH 0
#define PITCH 1

///////////////////////////////////////////////////////////////////////  
//  Constants

const float c_fSpeedUpScale = 3.0f;
const float c_fSlowDownScale = 0.15f;


///////////////////////////////////////////////////////////////////////  
//  CAutoCamera::CAutoCamera

CAutoCamera::CAutoCamera( ) :
    m_fNextAdvance(0.0f),
    m_fCumulativeTime(0.0f),
    m_eType(GO),
    m_fCameraRadius(0.0f),
    m_fTargetRadius(0.0f)
{
    m_vecCameraPos[X] = 0.0f;
    m_vecCameraPos[Y] = -20.0f;
    m_vecCameraPos[Z] = 10.0f;

    // arbitrary camera position
    m_afCameraPos[X] = m_vecCameraPos[X];
    m_afCameraPos[Y] = m_vecCameraPos[Y];
    m_afCameraPos[Z] = m_vecCameraPos[Z];
}


///////////////////////////////////////////////////////////////////////  
//  CAutoCamera::~CAutoCamera
CAutoCamera::~CAutoCamera( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CAutoCamera::Advance

bool CAutoCamera::Advance(float fTime, const CSpeedTreeForest* pForest)
{
    bool bSwitch = false;

    m_fCumulativeTime += fTime;

    if (m_fCumulativeTime > m_fNextAdvance)
    {
        PickNewCamera(pForest);
        if (IdvRandom(0, 100) < 50)
            bSwitch = true;
    }

    switch (m_eType)
    {
    case GO:
        Go(fTime);
        break;
    case WANDERING_EYE:
        WanderingEye(fTime);
        break;
    case ORBIT:
        Orbit(fTime);
        break;
    case EXAMINE:
        Examine(fTime);
        break;
    default:
        break;
    }

    return bSwitch;
}


///////////////////////////////////////////////////////////////////////  
//  CAutoCamera::GetCameraTransform

const float* CAutoCamera::GetCameraTransform(void)
{
    m_afCameraPos[X] = m_vecCameraPos[X];
    m_afCameraPos[Y] = m_vecCameraPos[Y];
    m_afCameraPos[Z] = m_vecCameraPos[Z];

    m_cTransform.LoadIdentity( );

#ifdef UPVECTOR_POS_Y
    m_cTransform.LookAt(m_vecCameraPos, m_vecTargetPos, idv::Vec3(0.0f, 1.0f, 0.0f));
#else
    m_cTransform.LookAt(m_vecCameraPos, m_vecTargetPos, idv::Vec3(0.0f, 0.0f, 1.0f));
#endif

    return reinterpret_cast<float*>(m_cTransform.m_afData);
}


///////////////////////////////////////////////////////////////////////  
//  CAutoCamera::GetAzimuth

float CAutoCamera::GetAzimuth(void) const
{
    idv::Vec3 vecCamera = m_vecCameraPos;
    idv::Vec3 vecTarget = m_vecTargetPos;
    vecCamera[Z] = vecTarget[Z] = 0.0f;

    return vecCamera.AngleBetween(vecTarget);
}

///////////////////////////////////////////////////////////////////////  
//  CAutoCamera::PickNewCamera

void CAutoCamera::PickNewCamera(const CSpeedTreeForest* pForest)
{
    int nRandom = IdvRandom((int) GO, ((int) NUM_CAMERA_TYPES) + 2); // extras added to favor EXAMINE
    if (nRandom >= (int) EXAMINE)
        nRandom = (int) EXAMINE;
    m_eType = static_cast<ECameraTypes>(nRandom);

    switch (m_eType)
    {
    case GO:
        PickGo( );
        break;
    case WANDERING_EYE:
        PickWanderingEye( );
        break;
    case ORBIT:
        PickOrbit( );
        break;
    default:
        PickExamine(pForest);
        break;
    }

    m_fNextAdvance = IdvRandom(5.0f, 10.0f);
    m_fCumulativeTime = 0.0f;
}


///////////////////////////////////////////////////////////////////////  
//  CAutoCamera::PickGo

void CAutoCamera::PickGo(void)
{
    float fLongest = m_cExtents.LongestExtent( );
    float fZ = m_cExtents.Dimension(Z);

    float fAzimuth = IdvRandom(0.0f, idv::TwoPi);
    float fRadius = IdvRandom(0.0f, fLongest * 0.25f);
    m_vecCameraPos[X] = m_cExtents.Midpoint( )[X] + cosf(fAzimuth) * fRadius;
    m_vecCameraPos[Y] = m_cExtents.Midpoint( )[Y] + sinf(fAzimuth) * fRadius;
    m_vecCameraPos[Z] = m_cExtents.Midpoint( )[Z] + IdvRandom(-fZ * 0.4f, fZ * 1.5f);

    fAzimuth = IdvRandom(0.0f, idv::TwoPi);
    fRadius = IdvRandom(fZ * 1.0f, fZ * 3.0f);
    m_vecTargetPos[X] = m_vecCameraPos[X] + cosf(fAzimuth) * fRadius;
    m_vecTargetPos[Y] = m_vecCameraPos[Y] + sinf(fAzimuth) * fRadius;
    m_vecTargetPos[Z] = m_cExtents.Midpoint( )[Z] + IdvRandom(-fZ * 0.2f, fZ * 0.2f);

    const float c_fSpeedAdjust = 0.2f;
    m_vecCameraVelocity[X] = IdvRandom(-fZ * c_fSpeedAdjust, fZ * c_fSpeedAdjust);
    m_vecCameraVelocity[Y] = IdvRandom(-fZ * c_fSpeedAdjust, fZ * c_fSpeedAdjust);
    m_vecCameraVelocity[Z] = 0.0f;
}


///////////////////////////////////////////////////////////////////////  
//  CAutoCamera::Go

void CAutoCamera::Go(float fDeltaTime)
{
    m_vecCameraPos += m_vecCameraVelocity * fDeltaTime;
    m_vecTargetPos += m_vecCameraVelocity * fDeltaTime;
}


///////////////////////////////////////////////////////////////////////  
//  CAutoCamera::PickWanderingEye

void CAutoCamera::PickWanderingEye(void)
{
    float fLongest = m_cExtents.LongestExtent( );
    float fZ = m_cExtents.Dimension(Z);

    float fAzimuth = IdvRandom(0.0f, idv::TwoPi);
    float fRadius = IdvRandom(0.0f, fLongest * 0.25f);
    m_vecCameraPos[X] = m_cExtents.Midpoint( )[X] + cosf(fAzimuth) * fRadius;
    m_vecCameraPos[Y] = m_cExtents.Midpoint( )[Y] + sinf(fAzimuth) * fRadius;
    m_vecCameraPos[Z] = m_cExtents.Midpoint( )[Z] + IdvRandom(-fZ * 0.4f, fZ * 1.5f);

    fAzimuth = IdvRandom(0.0f, idv::TwoPi);
    fRadius = IdvRandom(fZ * 4.0f, fZ * 8.0f);
    m_vecTargetPos[X] = m_vecCameraPos[X] + cosf(fAzimuth) * fRadius;
    m_vecTargetPos[Y] = m_vecCameraPos[Y] + sinf(fAzimuth) * fRadius;
    m_vecTargetPos[Z] = m_cExtents.Midpoint( )[Z] + IdvRandom(-fZ * 0.2f, fZ * 0.2f);

    const float c_fCameraSpeedAdjust = 0.4f;
    m_vecCameraVelocity[X] = IdvRandom(-fZ * c_fCameraSpeedAdjust, fZ * c_fCameraSpeedAdjust);
    m_vecCameraVelocity[Y] = IdvRandom(-fZ * c_fCameraSpeedAdjust, fZ * c_fCameraSpeedAdjust);
    m_vecCameraVelocity[Z] = 0.0f;

    const float c_fTargetSpeedAdjust = 0.4f;
    m_vecTargetVelocity[X] = IdvRandom(-fZ * c_fTargetSpeedAdjust, fZ * c_fTargetSpeedAdjust);
    m_vecTargetVelocity[Y] = IdvRandom(-fZ * c_fTargetSpeedAdjust, fZ * c_fTargetSpeedAdjust);
    m_vecTargetVelocity[Z] = 0.0f;
}


///////////////////////////////////////////////////////////////////////  
//  CAutoCamera::WanderingEye

void CAutoCamera::WanderingEye(float fDeltaTime)
{
    m_vecCameraPos += m_vecCameraVelocity * fDeltaTime;
    m_vecTargetPos += m_vecTargetVelocity * fDeltaTime;
}


///////////////////////////////////////////////////////////////////////  
//  CAutoCamera::PickOrbit

void CAutoCamera::PickOrbit(void)
{
    float fLongest = m_cExtents.LongestExtent( );
    float fZ = m_cExtents.Dimension(Z);

    float fAzimuth = IdvRandom(0.0f, idv::TwoPi);
    float fRadius = IdvRandom(0.0f, fLongest * 0.25f);
    m_vecTargetPos[X] = m_cExtents.Midpoint( )[X] + cosf(fAzimuth) * fRadius;
    m_vecTargetPos[Y] = m_cExtents.Midpoint( )[Y] + sinf(fAzimuth) * fRadius;
    m_vecTargetPos[Z] = m_cExtents.Midpoint( )[Z] + IdvRandom(-fZ * 0.4f, fZ * 0.4f);
    m_fTargetRadius = 0.0f;

    m_vecCameraData[AZIMUTH] = IdvRandom(0.0f, idv::TwoPi); // initial azimuth
    const float c_fPitchLimit = idv::QuarterPi * 0.25f;
    m_vecCameraData[PITCH] = IdvRandom(-c_fPitchLimit, c_fPitchLimit); // initial pitch
    m_fCameraRadius = IdvRandom(fZ * 0.5f, fZ * 0.5f);

    const float c_fAzimuthalVelocityLimit = 0.2f;
    m_vecCameraVelocity[AZIMUTH] = IdvRandom(-c_fAzimuthalVelocityLimit, c_fAzimuthalVelocityLimit);
    const float c_fPitchVelocityLimit = 0.05f;
    m_vecCameraVelocity[PITCH] = IdvRandom(-c_fPitchVelocityLimit, c_fPitchVelocityLimit);
}


///////////////////////////////////////////////////////////////////////  
//  CAutoCamera::Orbit

void CAutoCamera::Orbit(float)
{
    float fAzimuth = m_vecCameraData[AZIMUTH] + m_vecCameraVelocity[AZIMUTH] * m_fCumulativeTime;
    float fPitch = m_vecCameraData[PITCH] + m_vecCameraVelocity[PITCH] * m_fCumulativeTime;

    m_vecCameraPos[X] = m_vecTargetPos[X] + m_fCameraRadius * cosf(fAzimuth) * cosf(fPitch);
    m_vecCameraPos[Y] = m_vecTargetPos[Y] + m_fCameraRadius * sinf(fAzimuth) * cosf(fPitch);
    m_vecCameraPos[Z] = m_vecTargetPos[Z] + m_fCameraRadius * sinf(fPitch);
}


///////////////////////////////////////////////////////////////////////  
//  CAutoCamera::PickExamine

void CAutoCamera::PickExamine(const CSpeedTreeForest* pForest)
{
    // pick a random tree and get its extents
    const st_vector_stinstance& vInstances = pForest->GetAllTrees( );
    int nTreeIndex = IdvRandom(0, static_cast<int>(vInstances.size( )));
    float afTreeExtents[6];
    vInstances.at(nTreeIndex).InstanceOf( )->GetBoundingBox(afTreeExtents);

    idv::Region cTreeExtents;
    cTreeExtents.m_cMin.Set(afTreeExtents[0], afTreeExtents[1], afTreeExtents[2]);
    cTreeExtents.m_cMax.Set(afTreeExtents[3], afTreeExtents[4], afTreeExtents[5]);

    // remember the center of the target tree
    m_vecTargetData = cTreeExtents.Midpoint( );
    m_vecTargetData[Z] = cTreeExtents.m_cMax[Z] * 0.5f;

    m_vecTargetData[X] += vInstances.at(nTreeIndex).Pos( )[X];
    m_vecTargetData[Y] += vInstances.at(nTreeIndex).Pos( )[Y];
    m_vecTargetData[Z] += vInstances.at(nTreeIndex).Pos( )[Z];

    // setup the motion constants
    const float c_fMaxSpeed = 0.2f;
    m_vecTargetVelocity[0] = IdvRandom(-c_fMaxSpeed, c_fMaxSpeed);
    m_vecTargetVelocity[1] = IdvRandom(-c_fMaxSpeed, c_fMaxSpeed);
    m_vecTargetVelocity[2] = IdvRandom(-c_fMaxSpeed, c_fMaxSpeed);
    m_vecCameraVelocity[0] = IdvRandom(-c_fMaxSpeed, c_fMaxSpeed);
    m_vecCameraVelocity[1] = IdvRandom(-c_fMaxSpeed, c_fMaxSpeed);
    m_vecCameraVelocity[2] = IdvRandom(-c_fMaxSpeed, c_fMaxSpeed);

    // compute radii
    float fLongest = cTreeExtents.LongestExtent( );
    m_fCameraRadius = fLongest;
    m_fTargetRadius = fLongest * 0.05f;
}


///////////////////////////////////////////////////////////////////////  
//  CAutoCamera::Examine

void CAutoCamera::Examine(float)
{
    m_vecCameraPos = m_vecTargetPos = m_vecTargetData;

    // move the camera
    float fLongRadius = m_fCameraRadius * ((sinf(m_fCumulativeTime * m_vecCameraVelocity[0]) * 0.6f) + 1.0f);
    m_vecCameraPos[X] += fLongRadius * cosf(m_fCumulativeTime * m_vecCameraVelocity[1]);
    m_vecCameraPos[Y] += fLongRadius * sinf(m_fCumulativeTime * m_vecCameraVelocity[1]);
    m_vecCameraPos[Z] += m_vecTargetData[Z] * 0.8f * cosf(m_fCumulativeTime * m_vecCameraVelocity[2]);

    // move the target
    float fShortRadius = m_fTargetRadius * ((sinf(m_fCumulativeTime * m_vecTargetVelocity[0]) * 0.25f) + 1.0f);
    m_vecTargetPos[X] += fShortRadius * cosf(m_fCumulativeTime * m_vecTargetVelocity[1]);
    m_vecTargetPos[Y] += fShortRadius * sinf(m_fCumulativeTime * m_vecTargetVelocity[1]);
    m_vecTargetPos[Z] += m_vecTargetData[Z] * 0.2f * cosf(m_fCumulativeTime * m_vecTargetVelocity[2]);
}
