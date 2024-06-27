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
//  Assign3d

#ifdef UPVECTOR_POS_Y
inline void Assign3d(float* pDest, const float* pSource)
{
    pDest[0] = -pSource[0];
    pDest[1] = pSource[2];
    pDest[2] = pSource[1];
}
inline void Assign3d(float* pDest, float x, float y, float z)
{
    pDest[0] = -x;
    pDest[1] = z;
    pDest[2] = y;
}
inline const float* Convert3d(const float* pSource)
{
    static float afConvert[3];

    afConvert[0] = -pSource[0];
    afConvert[1] = pSource[2];
    afConvert[2] = pSource[1];

    return afConvert;
}
inline const float* Convert3d(float x, float y, float z)
{
    static float afConvert[3];

    afConvert[0] = -x;
    afConvert[1] = z;
    afConvert[2] = y;

    return afConvert;
}
#else // POS_Z
inline void Assign3d(float* pDest, const float* pSource)
{
    pDest[0] = pSource[0];
    pDest[1] = pSource[1];
    pDest[2] = pSource[2];
}
inline void Assign3d(float* pDest, float x, float y, float z)
{
    pDest[0] = x;
    pDest[1] = y;
    pDest[2] = z;
}
inline const float* Convert3d(const float* pSource)
{
    static float afConvert[3];

    afConvert[0] = pSource[0];
    afConvert[1] = pSource[1];
    afConvert[2] = pSource[2];

    return afConvert;
}
inline const float* Convert3d(float x, float y, float z)
{
    static float afConvert[3];

    afConvert[0] = x;
    afConvert[1] = y;
    afConvert[2] = z;

    return afConvert;
}
#endif

