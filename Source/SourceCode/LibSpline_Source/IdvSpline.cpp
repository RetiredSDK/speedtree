///////////////////////////////////////////////////////////////////////  
//  IdvSpline.cpp
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

#ifdef WIN32
#pragma warning (disable : 4786)
#endif

#include "../Debug.h"
#include "IdvSpline.h"
#include "../LibRandom_Source/IdvRandom.h"
#include <map>
#include <algorithm>


using namespace std;
#ifdef USE_SPLINE_CACHING
map<st_string, stBezierSpline*>* stBezierSpline::g_pSplineCache = NULL;
#endif
int stBezierSpline::m_nRefCount = 0;


///////////////////////////////////////////////////////////////////////  
//  stBezierSpline::stBezierSpline

stBezierSpline::stBezierSpline( ) :
    m_fVariance(0.0f),
    m_fMin(0.0f),
    m_fMax(1.0f),
    m_nNumControlPoints(0),
    m_nControlPointIndex(0),
    m_nSplinePointIndex(0),
    m_pControlPoints(NULL),
    m_pControlPointTangents(NULL),
    m_pControlPointTangentLengths(NULL),
    m_pSplinePoints(NULL)
{
    ++m_nRefCount;
}


///////////////////////////////////////////////////////////////////////  
//  stBezierSpline::stBezierSpline

stBezierSpline::stBezierSpline(const st_string& strInput)
{
#ifdef USE_SPLINE_CACHING
    if (!g_pSplineCache)
    {
        g_pSplineCache = new ("stBezierSpline::stBezierSpline, g_pSplineCache") map<st_string, stBezierSpline*>;
    }

    map<st_string, stBezierSpline*>::const_iterator iFind = g_pSplineCache->find(strInput);
    if (iFind == g_pSplineCache->end( ))
    {
        Parse(strInput);

        stBezierSpline* pCachedCopy = new ("stBezierSpline::stBezierSpline, pCachedCopy") stBezierSpline(*this);
        (*g_pSplineCache)[strInput] = pCachedCopy;
    }
    else
    {
        *this = *iFind->second;
    }
#else
    Parse(strInput);
#endif

    ++m_nRefCount;
}


///////////////////////////////////////////////////////////////////////  
//  stBezierSpline::stBezierSpline

stBezierSpline::~stBezierSpline( )
{
    st_assert(m_nRefCount >= 0);
}


///////////////////////////////////////////////////////////////////////  
//  stBezierSpline::ClearCache

void stBezierSpline::ClearCache(void)
{
#ifdef USE_SPLINE_CACHING
    if (g_pSplineCache)
    {
        for (map<st_string, stBezierSpline*>::iterator i = g_pSplineCache->begin( ); i != g_pSplineCache->end( ); ++i)
            st_delete<stBezierSpline>(i->second, "stBezierSpline::ClearCache, g_pSplineCache->second");

        g_pSplineCache->clear( );
        st_delete<map<st_string, stBezierSpline*> >(g_pSplineCache, "stBezierSpline::ClearCache, g_pSplineCache");
    }
#endif
}


///////////////////////////////////////////////////////////////////////  
//  stBezierSpline::Evaluate

float stBezierSpline::Evaluate(float fPercent, bool bScaleVariance) const
{
    float fValue = 0.0f;

    // new way, interpolated
    int nClosest = int(fPercent * float(c_nNumEvalPoints - 1));
    if (nClosest == c_nNumEvalPoints - 1)
    {
        fValue = m_acEvenlySpacedPoints[nClosest][1];
    }
    else
    {
        float fInterpPercent = (fPercent - (nClosest * c_fEvalPointSpacing)) / c_fEvalPointSpacing;
        fValue = VecInterpolate(m_acEvenlySpacedPoints[nClosest][1], m_acEvenlySpacedPoints[nClosest + 1][1], fInterpPercent);
    }

    float fNormalizedValue = fValue;
    fValue = fValue * (m_fMax - m_fMin) + m_fMin;

    static stRandom cRandom;
    if (bScaleVariance)
        fValue += fNormalizedValue * static_cast<float>(cRandom.GetUniform(-m_fVariance, m_fVariance));
    else
        fValue += static_cast<float>(cRandom.GetUniform(-m_fVariance, m_fVariance));
    
    return fValue;
}


///////////////////////////////////////////////////////////////////////  
//  stBezierSpline::EvaluateWithoutVariance

float stBezierSpline::EvaluateWithoutVariance(float fPercent) const
{
    float fValue = 0.0f;

    int nClosest = int(fPercent * float(c_nNumEvalPoints - 1));
    if (nClosest == c_nNumEvalPoints - 1)
        fValue = m_acEvenlySpacedPoints[nClosest][1];
    else
    {
        float fInterpPercent = (fPercent - (nClosest * c_fEvalPointSpacing)) / c_fEvalPointSpacing;
        fValue = VecInterpolate(m_acEvenlySpacedPoints[nClosest][1], m_acEvenlySpacedPoints[nClosest + 1][1], fInterpPercent);
    }

    fValue = fValue * (m_fMax - m_fMin) + m_fMin;
    
    return fValue;
}


/////////////////////////////////////////////////////////////////////////////
//  Function::IdvIsSpace

inline bool IdvIsSpace(char chChar)
{
    return (chChar == ' ' || chChar == '\t' || chChar == '\n' || chChar == '\r');
}


/////////////////////////////////////////////////////////////////////////////
//  Function::IdvIsAlpha

inline bool IdvIsAlpha(char chChar)
{
    return ((chChar >= 'a' && chChar <= 'z') ||
        (chChar >= 'A' && chChar <= 'Z'));
}


/////////////////////////////////////////////////////////////////////////////
//  Function::IdvIsDigit

inline bool IdvIsDigit(char chChar)
{
    return (chChar >= '0' && chChar <= '9');
}


/////////////////////////////////////////////////////////////////////////////
//  Function::IdvIsHexDigit

inline bool IdvIsHexDigit(char chChar)
{
    return (chChar >= '0' && chChar <= '9') ||
        (chChar >= 'A' && chChar <= 'F') ||
        (chChar >= 'a' && chChar <= 'f');
}


/////////////////////////////////////////////////////////////////////////////
//  Function::IdvAtof

template <class T> T IdvAtof(const char* pInput)
{
    unsigned int uiPos = 0;
    unsigned int uiSize = (unsigned int) strlen(pInput);
    T fReturn = 0.0f;
    bool bNegative = false;

    if (uiSize > 0)
    {
        char chCurrent = pInput[uiPos];
        while (uiPos != uiSize && IdvIsSpace(chCurrent))
            chCurrent = pInput[++uiPos];

        if (chCurrent == '-')
        {
            bNegative = true;
            chCurrent = pInput[++uiPos];
        }
        else if (chCurrent == '+')
            chCurrent = pInput[++uiPos];

        while (uiPos != uiSize && IdvIsDigit(chCurrent))
        {
            fReturn *= T(10.0);
            fReturn += (chCurrent - 48);
            chCurrent = pInput[++uiPos];
        }

        if (uiPos != uiSize && chCurrent == '.')
        {
            chCurrent = pInput[++uiPos];
            T fPlace = 0.1f;
            while (uiPos != uiSize && IdvIsDigit(chCurrent))
            {
                fReturn += (chCurrent - 48) * fPlace;
                fPlace *= (T)0.1;
                chCurrent = pInput[++uiPos];
            }
        }

        // should add 'd' and 'D' here since atof() supports them
        if (uiPos != uiSize && (chCurrent == 'e' || chCurrent == 'E'))
        {
            chCurrent = pInput[++uiPos];
            int iExponent = atoi(pInput + uiPos);
            fReturn *= powf(10.0f, float(iExponent));
        }
    }

    return (bNegative ? -fReturn : fReturn);
}


///////////////////////////////////////////////////////////////////////  
//  NextToken

static const char* NextToken(const char* pInput, char* pToken)
{
    while (IdvIsSpace(*pInput))
        pInput++;
    sscanf(pInput, "%s", pToken);

    return pInput + strlen(pToken);
}


///////////////////////////////////////////////////////////////////////  
//  stBezierSpline::Parse

void stBezierSpline::Parse(const st_string& strInput)
{
    m_nControlPointIndex = 0;
    m_nSplinePointIndex = 0;
    const char* pInput = strInput.c_str( );

    char szToken[256];
    pInput = NextToken(pInput, szToken);

    if (strcmp(szToken, "BezierSpline") == 0)
    {
        // get min, max, variance
        pInput = NextToken(pInput, szToken);
        m_fMin = static_cast<float>(IdvAtof<float>(szToken));
        pInput = NextToken(pInput, szToken);
        m_fMax = static_cast<float>(IdvAtof<float>(szToken));
        pInput = NextToken(pInput, szToken);
        m_fVariance = static_cast<float>(IdvAtof<float>(szToken));

        // match '{'
        pInput = NextToken(pInput, szToken);
        if (szToken[0] == '{')
        {
            // for each control point, get x,y control point, x,y tangent, and tangent length
            pInput = NextToken(pInput, szToken);
            int nNumControlPoints = atoi(szToken);
            SetNumControlPoints(nNumControlPoints);
            for (int i = 0; i < nNumControlPoints; ++i)
            {
                float afControlPoint[2];
                pInput = NextToken(pInput, szToken);
                afControlPoint[0] = static_cast<float>(IdvAtof<float>(szToken));
                pInput = NextToken(pInput, szToken);
                afControlPoint[1] = static_cast<float>(IdvAtof<float>(szToken));

                float afTangent[2];
                pInput = NextToken(pInput, szToken);
                afTangent[0] = static_cast<float>(IdvAtof<float>(szToken));
                pInput = NextToken(pInput, szToken);
                afTangent[1] = static_cast<float>(IdvAtof<float>(szToken));

                float fTangentLength;
                pInput = NextToken(pInput, szToken);
                fTangentLength = static_cast<float>(IdvAtof<float>(szToken));

                AddControlPoint(afControlPoint, afTangent, fTangentLength);
            }
            // the last token should be '}', but we ignore it
        }
    }
    CreateEvenlySpacedPoints(c_nNumEvalPoints);
}


///////////////////////////////////////////////////////////////////////  
//  stBezierSpline::Save

st_string stBezierSpline::Save(void) const
{
    st_string strOutput;

    strOutput = "BezierSpline ";
    strOutput += IdvFormatString("%g %g %g\n", m_fMin, m_fMax, m_fVariance);
    strOutput += "{\n";
        strOutput += IdvFormatString("\t%d\n", m_nNumControlPoints);
        for (int i = 0; i < m_nNumControlPoints; ++i)
            strOutput += IdvFormatString("\t%g %g %g %g %g\n", m_pControlPoints[i][0], m_pControlPoints[i][1], m_pControlPointTangents[i][0], m_pControlPointTangents[i][1], m_pControlPointTangentLengths[i]);
        strOutput += "\n";
    strOutput += "}\n";

    return strOutput;
}


///////////////////////////////////////////////////////////////////////  
//  stBezierSpline::ScaledVariance

float stBezierSpline::ScaledVariance(float fPercent) const
{
    float fValue = 0.0f;

    int nClosest = int(fPercent * float(c_nNumEvalPoints - 1) + 0.5f);
    fValue = m_acEvenlySpacedPoints[nClosest][1];

    static stRandom cRandom;
    fValue = static_cast<float>(cRandom.GetUniform(-m_fVariance * fValue, m_fVariance * fValue));
    
    return fValue;
}


///////////////////////////////////////////////////////////////////////  
//  stBezierSpline::AddControlPoint

void stBezierSpline::AddControlPoint(float afPoint[2], float afTangent[2], float fTangentLength)
{
    stVec cPoint(afPoint[0], afPoint[1]);
    stVec cTangent(afTangent[0], afTangent[1]);
    cTangent.Normalize( );
    
    if (m_nControlPointIndex > 0)
    {
        int nLastEntry = m_nControlPointIndex - 1;
        m_pSplinePoints[m_nSplinePointIndex++] = m_pControlPoints[nLastEntry] + (m_pControlPointTangents[nLastEntry] * m_pControlPointTangentLengths[nLastEntry]);
        m_pSplinePoints[m_nSplinePointIndex++] = cPoint - (cTangent * fTangentLength);
    }

    m_pControlPoints[m_nControlPointIndex] = cPoint;
    m_pSplinePoints[m_nSplinePointIndex++] = cPoint;
    m_pControlPointTangents[m_nControlPointIndex] = cTangent;
    m_pControlPointTangentLengths[m_nControlPointIndex] = fTangentLength;
    ++m_nControlPointIndex;
}


///////////////////////////////////////////////////////////////////////  
//  function SplineInterpolate

inline void SplineInterpolate(const stVec* pPoints, float x, float* pValue)
{
    float c12[2] = { VecInterpolate(pPoints[0][0], pPoints[1][0], x), VecInterpolate(pPoints[0][1], pPoints[1][1], x) };
    float c23[2] = { VecInterpolate(pPoints[1][0], pPoints[2][0], x), VecInterpolate(pPoints[1][1], pPoints[2][1], x) };
    float c34[2] = { VecInterpolate(pPoints[2][0], pPoints[3][0], x), VecInterpolate(pPoints[2][1], pPoints[3][1], x) };
    float c1223[2] = { VecInterpolate(c12[0], c23[0], x), VecInterpolate(c12[1], c23[1], x) };
    float c2334[2] = { VecInterpolate(c23[0], c34[0], x), VecInterpolate(c23[1], c34[1], x) };

    pValue[0] = VecInterpolate(c1223[0], c2334[0], x);
    pValue[1] = VecInterpolate(c1223[1], c2334[1], x);
}


///////////////////////////////////////////////////////////////////////  
//  stBezierSpline::CreateEvenlySpacedPoints

void stBezierSpline::CreateEvenlySpacedPoints(unsigned int nNumPoints)
{
    if (m_nNumControlPoints > 1)
    {
        float* pRawPoints = st_new_array<float>(nNumPoints * 2, "stBezierSpline::CreateEvenlySpacedPoints, pRawPoints");

        float fInterval = float(m_nNumControlPoints - 1) / (nNumPoints - 1);
        float fFraction = 0.0f;
        float* pValue = pRawPoints;
        int nLeftPoint = 0;
        unsigned int nPointsProcessed = 0;
        while (nLeftPoint < m_nNumControlPoints - 1 && nPointsProcessed < nNumPoints)
        {
            while (fFraction < 1.0f)
            {
                SplineInterpolate(m_pSplinePoints + nLeftPoint * 3, fFraction, pValue);
                fFraction += fInterval;
                pValue += 2;
                ++nPointsProcessed;
            }
            ++nLeftPoint;
            fFraction -= 1.0f;
        }
        if (nPointsProcessed < nNumPoints)
        {
            pValue[0] = m_pControlPoints[m_nNumControlPoints - 1][0];
            pValue[1] = m_pControlPoints[m_nNumControlPoints - 1][1];
            ++nPointsProcessed;
        }

        unsigned int nIndex = 0;
        float* pTestPoint = pRawPoints;
        m_acEvenlySpacedPoints[0][0] = m_pControlPoints[0][0];
        m_acEvenlySpacedPoints[0][1] = m_pControlPoints[0][1];
        for (unsigned int i = 1u; i < nNumPoints - 1; ++i)
        {
            float fPercent = float(i) / float(nNumPoints);
            for (unsigned int j = nIndex; j < nNumPoints - 1; ++j, pTestPoint += 2)
            {
                if (fPercent >= pTestPoint[0] && fPercent < pTestPoint[2])
                {
                    nIndex = j;
                    break;
                }
            }

            fPercent = (fPercent - pTestPoint[0]) / (pTestPoint[2] - pTestPoint[0]);
            m_acEvenlySpacedPoints[i][0] = fPercent;
            m_acEvenlySpacedPoints[i][1] = VecInterpolate(pTestPoint[1], pTestPoint[3], fPercent);
        }
        m_acEvenlySpacedPoints[nNumPoints - 1][0] = m_pControlPoints[m_nNumControlPoints - 1][0];
        m_acEvenlySpacedPoints[nNumPoints - 1][1] = m_pControlPoints[m_nNumControlPoints - 1][1];

        st_delete_array<float>(pRawPoints, "stBezierSpline::CreateEvenlySpacedPoints, pRawPoints");
    }
}


///////////////////////////////////////////////////////////////////////  
//  stBezierSpline::SetNumControlPoints

void stBezierSpline::SetNumControlPoints(int nNumPoints)
{
    m_nNumControlPoints = nNumPoints;

    if (nNumPoints <= c_nMaxStackPoints)
    {
        m_pControlPoints = m_acControlPoints;
        m_pControlPointTangents = m_acControlPointTangents;
        m_pControlPointTangentLengths = m_afControlPointTangentLengths;
        m_pSplinePoints = m_acSplinePoints;
    }
    else
    {
        m_vControlPoints.resize(nNumPoints);
        m_pControlPoints = &m_vControlPoints[0];

        m_vControlPointTangents.resize(nNumPoints);
        m_pControlPointTangents = &m_vControlPointTangents[0];

        m_vControlPointTangentLengths.resize(nNumPoints);
        m_pControlPointTangentLengths = &m_vControlPointTangentLengths[0];

        m_vSplinePoints.resize(1 + 3 * (nNumPoints - 1));
        m_pSplinePoints = &m_vSplinePoints[0];
    }
}

