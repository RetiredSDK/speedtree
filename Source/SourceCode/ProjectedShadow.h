///////////////////////////////////////////////////////////////////////  
//  ProjectedShadow.h
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
#include "LibVector_Source/IdvFastMath.h"
#include "FileAccess.h"
#include "Debug.h"

// forward references
class CIndexedGeometry;


///////////////////////////////////////////////////////////////////////  
//  class CProjectedShadow declaration

class CProjectedShadow
{
public:
        CProjectedShadow( );

        void            Save(CTreeFileAccess& cFile) const;
        void            Parse(CTreeFileAccess& cFile);

        void            ComputeTexCoords(CIndexedGeometry* pGeometry, stVec3 cCenter, float fRadius, float* pShadowCoords = NULL) const;
        const char*     GetSelfShadowFilename(void) const       { return m_strSelfShadowMap.c_str( ); }

private:
        stVec3          ClosestPoint(const stVec3& cStart, const stVec3& cEnd, const stVec3& cVertex) const;

        stVec3          m_cRight;               // the "right" direction of the camera when the shadow image was taken by SpeedTreeCAD
        stVec3          m_cUp;                  // the "up" direction of the camera when the shadow image was taken by SpeedTreeCAD
        stVec3          m_cOut;                 // the "out" direction of the camera when the shadow image was taken by SpeedTreeCAD
        st_string       m_strSelfShadowMap;     // the name of the self-shadow texture map
};
