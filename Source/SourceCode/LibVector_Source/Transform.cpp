///////////////////////////////////////////////////////////////////////  
//  Transform.cpp
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
#include "../Debug.h"
using namespace std;
    

///////////////////////////////////////////////////////////////////////  
//  stTransform::stTransform definition

stTransform::stTransform( )
{
    LoadIdentity( );
}


///////////////////////////////////////////////////////////////////////  
//  stTransform::~stTransform definition

stTransform::~stTransform( )
{
    // do nothing
}


///////////////////////////////////////////////////////////////////////  
//  stTransform::Set definition

void stTransform::Set(float afData[4][4])
{
    memcpy(&m_afData[0][0], &afData[0][0], 16 * sizeof(float));
}
    

///////////////////////////////////////////////////////////////////////  
//  stTransform::LoadIdentity definition

void stTransform::LoadIdentity( )
{
    m_afData[0][0] = 1.0f;
    m_afData[0][1] = 0.0f;
    m_afData[0][2] = 0.0f;
    m_afData[0][3] = 0.0f;
    m_afData[1][0] = 0.0f;
    m_afData[1][1] = 1.0f;
    m_afData[1][2] = 0.0f;
    m_afData[1][3] = 0.0f;
    m_afData[2][0] = 0.0f;
    m_afData[2][1] = 0.0f;
    m_afData[2][2] = 1.0f;
    m_afData[2][3] = 0.0f;
    m_afData[3][0] = 0.0f;
    m_afData[3][1] = 0.0f;
    m_afData[3][2] = 0.0f;
    m_afData[3][3] = 1.0f;
}
    

///////////////////////////////////////////////////////////////////////  
//  stTransform::Translate definition

void stTransform::Translate(float fX, float fY, float fZ)
{
    stTransform  cTemp;

    cTemp.m_afData[3][0] = fX;
    cTemp.m_afData[3][1] = fY;
    cTemp.m_afData[3][2] = fZ;

    *this = cTemp * *this;
}
    

///////////////////////////////////////////////////////////////////////  
//  stTransform::Rotate definition

void stTransform::Rotate(float fAngle, char chAxis)
{
    stTransform cRotMatrix;

    float fCosine = VectorCosD(fAngle);
    float fSine = VectorSinD(fAngle);

    switch (chAxis)
    {
    case 'x': case 'X':
        cRotMatrix.m_afData[1][1] = fCosine;
        cRotMatrix.m_afData[1][2] = fSine;
        cRotMatrix.m_afData[2][1] = -fSine;
        cRotMatrix.m_afData[2][2] = fCosine;
        break;
    case 'y': case 'Y':
        cRotMatrix.m_afData[0][0] = fCosine;
        cRotMatrix.m_afData[0][2] = -fSine;
        cRotMatrix.m_afData[2][0] = fSine;
        cRotMatrix.m_afData[2][2] = fCosine;
        break;
    case 'z': case 'Z':
        cRotMatrix.m_afData[0][0] = fCosine;
        cRotMatrix.m_afData[1][0] = -fSine;
        cRotMatrix.m_afData[0][1] = fSine;
        cRotMatrix.m_afData[1][1] = fCosine;
        break;
    }

    *this = cRotMatrix * *this;
}
    

///////////////////////////////////////////////////////////////////////  
//  stTransform::RotateAxis definition

void stTransform::RotateAxis(float fAngle, stVec cAxis)
{
    cAxis.Normalize( );
    float s = VectorSinD(fAngle);
    float c = VectorCosD(fAngle);
    float t = 1.0f - c;

    float x = cAxis.m_afData[0];
    float y = cAxis.m_afData[1];
    float z = cAxis.m_afData[2];

    stTransform  cRotMatrix;
    cRotMatrix.m_afData[0][0] = t * x * x + c;
    cRotMatrix.m_afData[0][1] = t * x * y + s * z;
    cRotMatrix.m_afData[0][2] = t * x * z - s * y;
    cRotMatrix.m_afData[0][3] = 0.0;
    cRotMatrix.m_afData[1][0] = t * x * y - s * z;
    cRotMatrix.m_afData[1][1] = t * y * y + c;
    cRotMatrix.m_afData[1][2] = t * y * z + s * x;
    cRotMatrix.m_afData[1][3] = 0.0;
    cRotMatrix.m_afData[2][0] = t * x * z + s * y;
    cRotMatrix.m_afData[2][1] = t * y * z - s * x;
    cRotMatrix.m_afData[2][2] = t * z * z + c;
    cRotMatrix.m_afData[2][3] = 0.0;
    cRotMatrix.m_afData[3][0] = 0.0;
    cRotMatrix.m_afData[3][1] = 0.0;
    cRotMatrix.m_afData[3][2] = 0.0;
    cRotMatrix.m_afData[3][3] = 1.0;

    *this = cRotMatrix * *this;
}
    

///////////////////////////////////////////////////////////////////////  
//  stTransform::IsIdentity definition

bool stTransform::IsIdentity(void) const
{
    stTransform cId;

    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            if (cId.m_afData[i][j] != m_afData[i][j])
                return false;

    return true;
}
    

///////////////////////////////////////////////////////////////////////  
//  stTransform::Scale definition

void stTransform::Scale(float fX)
{
    stTransform  cTemp;

    cTemp.m_afData[0][0] = fX;
    cTemp.m_afData[1][1] = fX;
    cTemp.m_afData[2][2] = fX;

    *this = cTemp * *this;
}
    

///////////////////////////////////////////////////////////////////////  
//  stTransform::Scale definition

void stTransform::Scale(float fX, float fY, float fZ)
{
    stTransform  cTemp;

    cTemp.m_afData[0][0] = fX;
    cTemp.m_afData[1][1] = fY;
    cTemp.m_afData[2][2] = fZ;

    *this = cTemp * *this;
}


///////////////////////////////////////////////////////////////////////  
//  stTransform::operator* definition

stTransform  stTransform::operator*(const stTransform& cTransform) const
{
    stTransform  cTemp;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
        {
            cTemp.m_afData[i][j] = 0.0;
            for (int k = 0; k < 4; ++k)
                cTemp.m_afData[i][j] += m_afData[i][k] * cTransform.m_afData[k][j];
        }

    return cTemp;
}
    

///////////////////////////////////////////////////////////////////////  
//  stTransform::operator* definition

stVec stTransform::operator*(const stVec& cVec) const
{
    int nSize = cVec.GetSize( );
    stVec cTemp(nSize);

    for (int i = 0; i < nSize; ++i)
    {
        cTemp.m_afData[i] = 0.0f;
        for (int j = 0; j < 4; ++j)
            if (j < nSize)
                    cTemp.m_afData[i] += m_afData[i][j] * cVec.m_afData[j];
            else if (j == 3)
                    cTemp.m_afData[i] += m_afData[i][j];
    }

    return cTemp;
}
    

///////////////////////////////////////////////////////////////////////  
//  stTransform::operator* definition

stVec3 stTransform::operator*(const stVec3& cVec3) const
{
    stVec3 cTemp;

    for (int i = 0; i < 3; ++i)
    {
        cTemp.m_afData[i] = 0.0f;
        for (int j = 0; j < 4; ++j)
            if (j < 3)
                    cTemp.m_afData[i] += m_afData[i][j] * cVec3.m_afData[j];
            else if (j == 3)
                    cTemp.m_afData[i] += m_afData[i][j];
    }
    return cTemp;
}
    

///////////////////////////////////////////////////////////////////////  
//  stTransform::operator* definition

stRegion stTransform::operator*(const stRegion& cRegion) const
{
    stRegion cTemp;

    cTemp.m_cMin = *this * cRegion.m_cMin;
    cTemp.m_cMax = *this * cRegion.m_cMax;

    return cTemp;
}
    

///////////////////////////////////////////////////////////////////////  
//  stTransform::operator+ definition

stTransform stTransform::operator+(const stTransform& cTransform) const
{
    stTransform  cTemp;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            cTemp.m_afData[i][j] = cTransform.m_afData[i][j] + m_afData[i][j];

    return cTemp;
}
    

///////////////////////////////////////////////////////////////////////  
//  stTransform::operator- definition

stTransform stTransform::operator-(const stTransform& cTransform) const
{
    stTransform  cTemp;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            cTemp.m_afData[i][j] = m_afData[i][j] - cTransform.m_afData[i][j];

    return cTemp;
}
    

///////////////////////////////////////////////////////////////////////  
//  stTransform::LookAt definition

void stTransform::LookAt(const stVec& cEye, const stVec& cCenter, const stVec& cUp)
{
    stVec cF = cCenter - cEye;
    cF.Normalize( );

    stVec cUpPrime = cUp;
    cUpPrime.Normalize( );

    stVec cS = cF * cUpPrime;
    stVec cU = cS * cF;

    stTransform cTemp;
    cTemp.m_afData[0][0] = cS[0];
    cTemp.m_afData[1][0] = cS[1];
    cTemp.m_afData[2][0] = cS[2];

    cTemp.m_afData[0][1] = cU[0];
    cTemp.m_afData[1][1] = cU[1];
    cTemp.m_afData[2][1] = cU[2];

    cTemp.m_afData[0][2] = -cF[0];
    cTemp.m_afData[1][2] = -cF[1];
    cTemp.m_afData[2][2] = -cF[2];

    *this = cTemp * *this;

    Translate(-cEye.m_afData[0], -cEye.m_afData[1], -cEye.m_afData[2]);
}

