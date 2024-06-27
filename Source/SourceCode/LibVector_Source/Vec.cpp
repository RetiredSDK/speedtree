///////////////////////////////////////////////////////////////////////  
//  Vec.cpp
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

// static variable definitions
float stVec::m_fTolerance = 0.1f;
float stVec3::m_fTolerance(0.1f);


///////////////////////////////////////////////////////////////////////  
//  stVec::stVec definition

stVec::stVec( )
{
    m_afData[0] = m_afData[1] = m_afData[2] = m_afData[3] = m_afData[4] = 0.0f;
    m_nSize = 3;
}


///////////////////////////////////////////////////////////////////////  
//  stVec::stVec definition
//

stVec::stVec(int nSize)
{
    m_afData[0] = m_afData[1] = m_afData[2] = m_afData[3] = m_afData[4] = 0.0f;
    if (nSize > 5)
        nSize = 5;
    m_nSize = nSize;
}


///////////////////////////////////////////////////////////////////////  
//  stVec::stVec definition

stVec::stVec(float fX, float fY)
{
    m_afData[0] = fX;
    m_afData[1] = fY;
    m_afData[2] = m_afData[3] = m_afData[4] = 0.0f;
    m_nSize = 2;
}


///////////////////////////////////////////////////////////////////////  
//  stVec::stVec definition

stVec::stVec(float fX, float fY, float fZ)
{
    m_afData[0] = fX;
    m_afData[1] = fY;
    m_afData[2] = fZ;
    m_afData[3] = m_afData[4] = 0.0;
    m_nSize = 3;
}


///////////////////////////////////////////////////////////////////////  
//  stVec::stVec definition

stVec::stVec(float fX, float fY, float fZ, float fW)
{
    m_afData[0] = fX;
    m_afData[1] = fY;
    m_afData[2] = fZ;
    m_afData[3] = fW;
    m_afData[4] = 0.0;
    m_nSize = 4;
}


///////////////////////////////////////////////////////////////////////  
//  stVec::stVec definition

stVec::stVec(float fX, float fY, float fZ, float fW, float fV)
{
    m_afData[0] = fX;
    m_afData[1] = fY;
    m_afData[2] = fZ;
    m_afData[3] = fW;
    m_afData[4] = fV;
    m_nSize = 5;
}


///////////////////////////////////////////////////////////////////////  
//  stVec::~stVec definition

stVec::~stVec( )
{
}


///////////////////////////////////////////////////////////////////////  
//  stVec::Magnitude definition

float stVec::Magnitude( ) const
{
    float fTemp = 0.0f;
    for (int i = 0; i < m_nSize; ++i)
        fTemp += m_afData[i] * m_afData[i];

    return sqrtf(fTemp);
}


///////////////////////////////////////////////////////////////////////  
//  stVec::MaxAxis definition

float stVec::MaxAxis( ) const
{
    float fMax = -FLT_MAX;
    for (int i = 0; i < m_nSize; ++i)
        if (m_afData[i] > fMax)
            fMax = m_afData[i];

    return fMax;
}


///////////////////////////////////////////////////////////////////////  
//  stVec::MinAxis definition

float stVec::MinAxis( ) const
{
    float fMin = FLT_MAX;
    for (int i = 0; i < m_nSize; ++i)
        if (m_afData[i] < fMin)
            fMin = m_afData[i];

    return fMin;
}


///////////////////////////////////////////////////////////////////////  
//  stVec::Normalize definition

void stVec::Normalize( )
{
    float fMag = Magnitude( );
    if (fMag != 0.0f)
        for (int i = 0; i < m_nSize; ++i)
            m_afData[i] /= fMag;
}


///////////////////////////////////////////////////////////////////////  
//  stVec::RoughlyEqual definition

bool stVec::RoughlyEqual(const stVec &cVec) const
{
    bool bValue = true;
    for (int i = 0; i < m_nSize; ++i)
        if (fabs(m_afData[i] - cVec.m_afData[i]) > m_fTolerance)
        {
            bValue = false;
            break;
        }

    return bValue;
}


///////////////////////////////////////////////////////////////////////  
//  stVec::AbsoluteValue definition

stVec stVec::AbsoluteValue( ) const
{
    stVec cTemp;
    for (int i = 0; i < m_nSize; ++i)
        if (m_afData[i] < 0.0f)
            cTemp.m_afData[i] = -m_afData[i];
        else
            cTemp.m_afData[i] = m_afData[i];

    return cTemp;
}


///////////////////////////////////////////////////////////////////////  
//  stVec::operator+ definition

stVec stVec::operator+(const stVec& cVec) const
{
    int nSize = -1;
    if (m_nSize < cVec.m_nSize)
        nSize = m_nSize;
    else
        nSize = cVec.m_nSize;

    stVec cTemp(nSize);
    for (int i = 0; i < m_nSize; ++i)
        cTemp.m_afData[i] = m_afData[i] + cVec.m_afData[i];

    return cTemp;
}


///////////////////////////////////////////////////////////////////////  
//  stVec::operator+= definition

stVec& stVec::operator+=(const stVec& cVec)
{
    *this = *this + cVec;

    return *this;
}


///////////////////////////////////////////////////////////////////////  
//  stVec::operator- definition

stVec stVec::operator-(const stVec& cVec) const
{
    int nSize = -1;
    if (m_nSize < cVec.m_nSize)
        nSize = m_nSize;
    else
        nSize = cVec.m_nSize;

    stVec cTemp(nSize);
    for (int i = 0; i < m_nSize; ++i)
        cTemp.m_afData[i] = m_afData[i] - cVec.m_afData[i];

    return cTemp;
}


///////////////////////////////////////////////////////////////////////  
//  stVec::operator-= definition

stVec& stVec::operator-=(const stVec& cVec)
{
    *this = *this - cVec;

    return *this;
}


///////////////////////////////////////////////////////////////////////  
//  stVec::operator- definition

stVec stVec::operator-( ) const
{
    stVec cTemp(m_nSize);
    for (int i = 0; i < m_nSize; ++i)
        cTemp.m_afData[i] = -m_afData[i];

    return cTemp;
}


///////////////////////////////////////////////////////////////////////  
//  stVec::operator^ definition

float stVec::operator^(const stVec& cVec) const
{
    float fTemp = 0.0f;
    for (int i = 0; i < m_nSize; ++i) 
        fTemp += m_afData[i] * cVec.m_afData[i];

    return fTemp;
}


///////////////////////////////////////////////////////////////////////  
//  stVec::operator* definition

stVec stVec::operator*(const stVec& cVec) const
{
    stVec cTemp(3);

    cTemp.m_afData[0] = m_afData[1] * cVec.m_afData[2] - m_afData[2] * cVec.m_afData[1];
    cTemp.m_afData[1] = m_afData[2] * cVec.m_afData[0] - m_afData[0] * cVec.m_afData[2];
    cTemp.m_afData[2] = m_afData[0] * cVec.m_afData[1] - m_afData[1] * cVec.m_afData[0];

    return cTemp;
}


///////////////////////////////////////////////////////////////////////  
//  stVec::operator*= definition

stVec& stVec::operator*=(const stVec& cVec)
{
    *this = *this * cVec;

    return *this;
}


///////////////////////////////////////////////////////////////////////  
//  stVec::operator* definition

stVec stVec::operator*(float fValue) const
{
    stVec cTemp(m_nSize);
    for (int i = 0; i < m_nSize; ++i)
        cTemp.m_afData[i] = m_afData[i] * fValue;

    return cTemp;
}


///////////////////////////////////////////////////////////////////////  
//  stVec::operator*= definition

stVec& stVec::operator*=(float fValue)
{
    *this = *this * fValue;

    return *this;
}


///////////////////////////////////////////////////////////////////////  
//  stVec::operator* definition

stVec stVec::operator*(const stTransform& cTransform) const
{
    stVec cTemp(m_nSize);
    for (int i = 0; i < m_nSize; ++i)
    {
        cTemp.m_afData[i] = 0.0;
        for (int j = 0; j < 4; ++j)
            if (j < m_nSize)
                cTemp.m_afData[i] += m_afData[j] * cTransform.m_afData[j][i];
            else if (j == 3)
                cTemp.m_afData[i] += cTransform.m_afData[j][i];
    }

    return cTemp;
}


///////////////////////////////////////////////////////////////////////  
//  stVec::operator/ definition

stVec stVec::operator/(float fValue) const
{
    stVec cTemp(m_nSize);

    for (int i = 0; i < m_nSize; ++i)
        cTemp.m_afData[i] = m_afData[i] / fValue;

    return cTemp;
}


///////////////////////////////////////////////////////////////////////  
//  stVec::operator/= definition

stVec& stVec::operator/=(float fValue)
{
    *this = *this / fValue;

    return *this;
}


///////////////////////////////////////////////////////////////////////  
//  stVec::operator== definition

bool stVec::operator==(const stVec& cVec) const
{
    bool bValue = true;

    for (int i = 0; i < m_nSize; ++i)
        if (m_afData[i] != cVec.m_afData[i])
        {
            bValue = false;
            break;
        }

    return bValue;
}


///////////////////////////////////////////////////////////////////////  
//  stVec::operator!= definition

bool stVec::operator!=(const stVec& cVec) const
{
    bool bValue = true;

    for (int i = 0; i < m_nSize; ++i)
        if (m_afData[i] != cVec.m_afData[i])
        {
            bValue = false;
            break;
        }

    return !bValue;
}


///////////////////////////////////////////////////////////////////////  
//  stVec::operator< definition

bool operator<(const stVec& stVecA, const stVec& stVecB)
{
    return (stVecA.m_afData[0] < stVecB.m_afData[0] ||
            stVecA.m_afData[1] < stVecB.m_afData[1] ||
            stVecA.m_afData[2] < stVecB.m_afData[2]);
}


///////////////////////////////////////////////////////////////////////  
//  stVec::operator< definition

bool stVec::operator<(const stRegion& cRegion) const
{
    if (m_nSize == 2)
        return ((m_afData[0] < cRegion.m_cMax[0] && m_afData[0] > cRegion.m_cMin[0]) && (m_afData[1] < cRegion.m_cMax[1] && m_afData[1] > cRegion.m_cMin[1]));
    else
        return ((m_afData[0] < cRegion.m_cMax[0] && m_afData[0] > cRegion.m_cMin[0]) && (m_afData[1] < cRegion.m_cMax[1] && m_afData[1] > cRegion.m_cMin[1]) && (m_afData[2] < cRegion.m_cMax[2] && m_afData[2] > cRegion.m_cMin[2]));
}


///////////////////////////////////////////////////////////////////////  
//  stVec::operator<= definition

bool stVec::operator<=(const stRegion& cRegion) const
{
    if (m_nSize == 2)
        return ((m_afData[0] <= cRegion.m_cMax[0] && m_afData[0] >= cRegion.m_cMin[0]) && (m_afData[1] <= cRegion.m_cMax[1] && m_afData[1] >= cRegion.m_cMin[1]));
    else
        return ((m_afData[0] <= cRegion.m_cMax[0] && m_afData[0] >= cRegion.m_cMin[0]) && (m_afData[1] <= cRegion.m_cMax[1] && m_afData[1] >= cRegion.m_cMin[1]) && (m_afData[2] <= cRegion.m_cMax[2] && m_afData[2] >= cRegion.m_cMin[2]));
}


///////////////////////////////////////////////////////////////////////  
//  stVec::Distance definition

float stVec::Distance(const stVec& cPoint) const
{
    if (m_nSize == 2)
        return sqrtf(
                    ((cPoint[0] - m_afData[0]) * (cPoint[0] - m_afData[0])) +
                    ((cPoint[1] - m_afData[1]) * (cPoint[1] - m_afData[1]))
                    );
    else
        return sqrtf(
                    ((cPoint[0] - m_afData[0]) * (cPoint[0] - m_afData[0])) +
                    ((cPoint[1] - m_afData[1]) * (cPoint[1] - m_afData[1])) +
                    ((cPoint[2] - m_afData[2]) * (cPoint[2] - m_afData[2]))
                    );
}


///////////////////////////////////////////////////////////////////////  
//  stVec::AngleBetween definition

float stVec::AngleBetween(const stVec& cVec) const
{
    return (acosf((*this ^ cVec) / (Magnitude( ) * cVec.Magnitude( ))));
}


///////////////////////////////////////////////////////////////////////  
//  stVec::operator< definition

bool stVec::operator<(const stVec& cVec) const
{
    if (m_afData[0] < cVec.m_afData[0])
        return true;
    else if (m_afData[0] > cVec.m_afData[0])
        return false;
    else
    {
        if (m_afData[1] < cVec.m_afData[1])
            return true;
        else if (m_afData[1] > cVec.m_afData[1])
            return false;
        else
            return m_afData[2] < cVec.m_afData[2];
    }
}


///////////////////////////////////////////////////////////////////////  
//  stVec::operator<= definition

bool stVec::operator<=(const stVec& cVec) const
{
    return (m_afData[0] <= cVec.m_afData[0]);
}


///////////////////////////////////////////////////////////////////////  
//  stVec::operator> definition

bool stVec::operator>(const stVec& cVec) const
{
    if (m_afData[0] > cVec.m_afData[0])
        return true;
    else if (m_afData[0] < cVec.m_afData[0])
        return false;
    else
    {
        if (m_afData[1] > cVec.m_afData[1])
            return true;
        else if (m_afData[1] < cVec.m_afData[1])
            return false;
        else
            return m_afData[2] > cVec.m_afData[2];
    }
}


///////////////////////////////////////////////////////////////////////  
//  stVec::operator>= definition

bool stVec::operator>=(const stVec& cVec) const
{
    return (m_afData[0] >= cVec.m_afData[0]);
}
