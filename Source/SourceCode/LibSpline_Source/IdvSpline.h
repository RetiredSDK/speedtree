///////////////////////////////////////////////////////////////////////  
//  IdvSpline.h
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
//#define USE_SPLINE_CACHING
#include "../LibVector_Source/IdvVector.h"
#include "../Debug.h"


// constants
const int c_nNumEvalPoints = 500;
const float c_fEvalPointSpacing = 1.0f / (c_nNumEvalPoints - 1.0f);


///////////////////////////////////////////////////////////////////////  
//  class stBezierSpline declaration

class stBezierSpline
{
public:
        stBezierSpline( );
        stBezierSpline(const st_string& strInput);
        ~stBezierSpline( );

        float                   Evaluate(float fPercent, bool bScaleVariance = false) const;
        float                   EvaluateWithoutVariance(float fPercent) const;
        void                    Parse(const st_string& strInput);
        st_string               Save(void) const;
        float                   ScaledVariance(float fPercent) const;
        float                   GetMin(void) const { return m_fMin; }
        float                   GetMax(void) const { return m_fMax; }
        bool                    IsInitialized(void) const { return m_nNumControlPoints > 0; }

static  void                    ClearCache(void);

private:
        void                    AddControlPoint(float afPoint[2], float afTangent[2], float fTangentLength);
        void                    CreateEvenlySpacedPoints(unsigned int nNumPoints);

        void                    SetNumControlPoints(int nNumPoints);

        float                   m_fVariance;                    // random amount of variance around true value
        float                   m_fMin;                         // minimum value returned when spline is evaluated
        float                   m_fMax;                         // maximum value returned when spline is evaluated

        // control points
        int                     m_nNumControlPoints;
        int                     m_nControlPointIndex;
        int                     m_nSplinePointIndex;

        // point pointers (point to either stack or heap copies)
        stVec*                  m_pControlPoints;               // x,y pairs of control points
        stVec*                  m_pControlPointTangents;        // x,y pairs of control point tangents
        float*                  m_pControlPointTangentLengths;  // control point tangent lengths
        stVec*                  m_pSplinePoints;

        // stack copies of points (used when less than c_nMaxStackPoints points)
        enum { c_nMaxStackPoints = 10 };

        stVec                   m_acControlPoints[c_nMaxStackPoints];
        stVec                   m_acControlPointTangents[c_nMaxStackPoints];
        float                   m_afControlPointTangentLengths[c_nMaxStackPoints];
        stVec                   m_acSplinePoints[1 + 3 * (c_nMaxStackPoints - 1)];

        // heap copies of points (used when greater than c_nMaxStackPoints points)
        st_vector_stVec         m_vControlPoints;               // x,y pairs of control points
        st_vector_stVec         m_vControlPointTangents;        // x,y pairs of control point tangents
        st_vector_float         m_vControlPointTangentLengths;  // control point tangent lengths
        st_vector_stVec         m_vSplinePoints;

        // always on the stack
        float                   m_acEvenlySpacedPoints[c_nNumEvalPoints][2];

static  int                     m_nRefCount;

#ifdef USE_SPLINE_CACHING
static  std::map<st_string, stBezierSpline*>* g_pSplineCache;
#endif
};
