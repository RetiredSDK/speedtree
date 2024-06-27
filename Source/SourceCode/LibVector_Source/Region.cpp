///////////////////////////////////////////////////////////////////////  
//  Region.cpp
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

#include "IdvVector.h"
using namespace std;


///////////////////////////////////////////////////////////////////////  
//  stRegion::stRegion definition

stRegion::stRegion( )
{
    Initialize( );
}


///////////////////////////////////////////////////////////////////////  
//  stRegion::operator+ definition

stRegion stRegion::operator+(const stRegion& cRegion) const
{
    stRegion cTemp = *this;

    for (int i = 0; i < 3; i++)
    {
        if (cRegion.m_cMin[i] < m_cMin[i])
            cTemp.m_cMin[i] = cRegion.m_cMin[i];
        if (cRegion.m_cMax[i] > m_cMax[i])
            cTemp.m_cMax[i] = cRegion.m_cMax[i];
    }

    return cTemp;
}


///////////////////////////////////////////////////////////////////////  
//  stRegion::operator+ definition

stRegion stRegion::operator+(const stVec& cVec) const
{
    stRegion cTemp = *this;

    cTemp.m_cMin = cTemp.m_cMin + cVec;
    cTemp.m_cMax = cTemp.m_cMax + cVec;

    return cTemp;
}


///////////////////////////////////////////////////////////////////////  
//  stRegion::operator^ definition

stRegion stRegion::operator^(const stVec3& cVec) const
{
    stRegion cTemp = *this;

    if (cVec[0] < cTemp.m_cMin[0])
        cTemp.m_cMin[0] = cVec[0];
    if (cVec[1] < cTemp.m_cMin[1])
        cTemp.m_cMin[1] = cVec[1];
    if (cVec[2] < cTemp.m_cMin[2])
        cTemp.m_cMin[2] = cVec[2];

    if (cVec[0] > cTemp.m_cMax[0])
        cTemp.m_cMax[0] = cVec[0];
    if (cVec[1] > cTemp.m_cMax[1])
        cTemp.m_cMax[1] = cVec[1];
    if (cVec[2] > cTemp.m_cMax[2])
        cTemp.m_cMax[2] = cVec[2];

    return cTemp;
}


///////////////////////////////////////////////////////////////////////  
//  stRegion::operator* definition

stRegion stRegion::operator*(float fScale) const
{
    stRegion cTemp = *this;
    stVec cMidpoint = Midpoint( );

    for (int i = 0; i < 3; i++)
    {
        float fDistance = (m_cMax[i] - m_cMin[i]) * 0.5F;
        cTemp.m_cMin[i] = cMidpoint[i] - fDistance * fScale;
        cTemp.m_cMax[i] = cMidpoint[i] + fDistance * fScale;
    }

    return cTemp;
}


///////////////////////////////////////////////////////////////////////  
//  stRegion::operator* definition

stRegion stRegion::operator*(const stTransform& cTransform) const
{
    stRegion cTemp;

    cTemp.m_cMin = m_cMin * cTransform;
    cTemp.m_cMax = m_cMax * cTransform;

    return cTemp;
}


///////////////////////////////////////////////////////////////////////  
//  stRegion::operator< definition

bool stRegion::operator<(const stRegion& cRegion) const
{
    bool bReturn = true;
    for (int i = 0; i < 3; i++)
        if (m_cMin[i] <= cRegion.m_cMin[i] || m_cMax[i] >= cRegion.m_cMax[i])
        {
            bReturn = false;
            break;
        }

    return bReturn;
}


///////////////////////////////////////////////////////////////////////  
//  stRegion::AverageExtent definition

float stRegion::AverageExtent( ) const
{
    float fSum = 0.0f;
    for (int i = 0; i < 3; i++)
        fSum += m_cMax[i] - m_cMin[i];

    return 0.33333333f * fSum;
}


///////////////////////////////////////////////////////////////////////  
//  stRegion::Initialized definition

void stRegion::Initialize( )
{
    m_cMin[0] = m_cMin[1] = m_cMin[2] = FLT_MAX;
    m_cMax[0] = m_cMax[1] = m_cMax[2] = -FLT_MAX;
}


///////////////////////////////////////////////////////////////////////  
//  stRegion::IsSet definition

bool stRegion::IsSet( ) const
{
    return (m_cMin[0] != FLT_MAX || 
            m_cMin[1] != FLT_MAX || 
            m_cMin[2] != FLT_MAX || 
            m_cMax[0] != -FLT_MAX || 
            m_cMax[1] != -FLT_MAX || 
            m_cMax[2] != -FLT_MAX);
}


///////////////////////////////////////////////////////////////////////  
//  stRegion::Scale definition

void stRegion::Scale(float fScale)
{
    m_cMin = m_cMin * fScale;
    m_cMax = m_cMax * fScale;
}

