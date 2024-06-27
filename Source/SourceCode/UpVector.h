///////////////////////////////////////////////////////////////////////  
//  UpVector.h
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

// if no up vector has been set, default to UPVECTOR_POS_Z
#if !defined UPVECTOR_POS_Z && !defined UPVECTOR_NEG_Z && !defined UPVECTOR_POS_Y && !defined UPVECTOR_NEG_Y && !defined UPVECTOR_POS_X && !defined UPVECTOR_NEG_X && !defined UPVECTOR_DIRECTX_RIGHT_HANDED_COORDINATE_SYSTEM
#define UPVECTOR_POS_Z
#endif


//  SpeedTree defaults to using a positive Z up vector and all of the branch
//  and leaf computations are done in this orientation.  When the geometry
//  is transferred to the classes CLeafGeometry and CIndexedGeometry, the
//  computed coordinates and normals are passed through the Assign3d() 
//  function to be potentially reoriented.
//
//  One and only one of the following seven symbols should be defined:
//
//      UPVECTOR_POS_Z
//      UPVECTOR_NEG_Z
//      UPVECTOR_POS_Y
//      UPVECTOR_NEG_Y
//      UPVECTOR_POS_X
//      UPVECTOR_NEG_X
//      UPVECTOR_DIRECTX_RIGHT_HANDED_COORDINATE_SYSTEM


///////////////////////////////////////////////////////////////////////  
//  function Assign3d definition (+Z up vector)

#ifdef UPVECTOR_POS_Z
inline void Assign3d(float* pDest, const float* pSource)
{
    pDest[0] = pSource[0];
    pDest[1] = pSource[1];
    pDest[2] = pSource[2];
}
#endif


///////////////////////////////////////////////////////////////////////  
//  function Assign3d definition (-Z up vector)

#ifdef UPVECTOR_NEG_Z
inline void Assign3d(float* pDest, const float* pSource)
{
    pDest[0] = -pSource[0];
    pDest[1] = pSource[1];
    pDest[2] = -pSource[2];
}
#endif


///////////////////////////////////////////////////////////////////////  
//  function Assign3d definition (+Y up vector)

#ifdef UPVECTOR_POS_Y
inline void Assign3d(float* pDest, const float* pSource)
{
    pDest[0] = -pSource[0];
    pDest[1] = pSource[2];
    pDest[2] = pSource[1];
}
#endif


///////////////////////////////////////////////////////////////////////  
//  function Assign3d definition (-Y up vector)

#ifdef UPVECTOR_NEG_Y
inline void Assign3d(float* pDest, const float* pSource)
{
    pDest[0] = pSource[0];
    pDest[1] = -pSource[2];
    pDest[2] = pSource[1];
}
#endif


///////////////////////////////////////////////////////////////////////  
//  function Assign3d definition (+X up vector)

#ifdef UPVECTOR_POS_X
inline void Assign3d(float* pDest, const float* pSource)
{
    pDest[0] = pSource[2];
    pDest[1] = pSource[1];
    pDest[2] = -pSource[0];
}
#endif


///////////////////////////////////////////////////////////////////////  
//  function Assign3d definition (-X up vector)

#ifdef UPVECTOR_NEG_X
inline void Assign3d(float* pDest, const float* pSource)
{
    pDest[0] = -pSource[2];
    pDest[1] = pSource[1];
    pDest[2] = pSource[0];
}
#endif


///////////////////////////////////////////////////////////////////////  
//  function Assign3d definition (X and Y swapped)

#ifdef UPVECTOR_DIRECTX_RIGHT_HANDED_COORDINATE_SYSTEM
inline void Assign3d(float* pDest, const float* pSource)
{
    pDest[0] = pSource[1];
    pDest[1] = pSource[0];
    pDest[2] = pSource[2];
}
#endif


///////////////////////////////////////////////////////////////////////  
//  function ComputeAzimuth definition

inline float ComputeAzimuth(const float* pCameraDir)
{
    // find the angle that point is on
    stVec3 cAdjustedDirection;
    Assign3d(cAdjustedDirection, pCameraDir);

    return VecRad2Deg(atan2f(cAdjustedDirection[1], cAdjustedDirection[0]));
}
