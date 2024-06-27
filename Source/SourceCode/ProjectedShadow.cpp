///////////////////////////////////////////////////////////////////////  
//  ProjectedShadow.cpp
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
#pragma warning (disable : 4786) // VC6 stl files cause warnings
#endif
#include "ProjectedShadow.h"
#include "LibGlobals_Source/IdvGlobals.h"
#include "LibFilename_Source/IdvFilename.h"
#include "IndexedGeometry.h"
#include "UpVector.h"


///////////////////////////////////////////////////////////////////////  
//  CProjectedShadow::CProjectedShadow definition

CProjectedShadow::CProjectedShadow( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CProjectedShadow::Save definition
//

void CProjectedShadow::Save(CTreeFileAccess& cFile) const
{
    cFile.SaveToken(File_BeginShadowProjectionInfo);

    cFile.SaveToken(File_ShadowRightVector);
    cFile.SaveVector3(m_cRight);

    cFile.SaveToken(File_ShadowUpVector);
    cFile.SaveVector3(m_cUp);

    cFile.SaveToken(File_ShadowOutVector);
    cFile.SaveVector3(m_cOut);

    cFile.SaveToken(File_ShadowMap);
    cFile.SaveString(m_strSelfShadowMap);

    cFile.SaveToken(File_EndShadowProjectionInfo);
}


///////////////////////////////////////////////////////////////////////  
//  CProjectedShadow::Parse definition

void CProjectedShadow::Parse(CTreeFileAccess& cFile)
{
    int nToken = cFile.ParseToken( );
    do
    {
        switch (nToken)
        {
        case File_ShadowRightVector:
            Assign3d(m_cRight, cFile.ParseVector3( ));
            break;
        case File_ShadowUpVector:
            Assign3d(m_cUp, cFile.ParseVector3( ));
            break;
        case File_ShadowOutVector:
            Assign3d(m_cOut, cFile.ParseVector3( ));
            break;
        case File_ShadowMap:
            {
                cFile.ParseString(m_strSelfShadowMap);
                m_strSelfShadowMap = IdvNoPath(m_strSelfShadowMap);
            }
            break;
        default:
            throw(IdvFileError(IdvFormatString("malformed frond info (token %d)", nToken)));
        }
        nToken = cFile.ParseToken( );
    } while (nToken != File_EndShadowProjectionInfo);
}


///////////////////////////////////////////////////////////////////////  
//  CProjectedShadow::ComputeTexCoords definition

void CProjectedShadow::ComputeTexCoords(CIndexedGeometry* pGeometry, stVec3 cCenter, float fRadius, float* pShadowCoords) const
{
    // compute axis end points
    stVec3 cStartS = cCenter - m_cRight * fRadius, cEndS = cCenter + m_cRight * fRadius;
    stVec3 cStartT = cCenter - m_cUp * fRadius, cEndT = cCenter + m_cUp * fRadius;

    // compute the texcoords
    pGeometry->ResetVertexCounter( );
    int nVertexCount = pGeometry->GetVertexCount( ); 
    for (int i = 0; i < nVertexCount; ++i)
    {
        float afTexCoords[2];

        // get the vertex
        //lint -save -e661 -e662
        const float* pVertex = pGeometry->GetVertexCoord(i);
        stVec3 cVertex(pVertex[0], pVertex[1], pVertex[2]);
        //line -restore

        // compute the s axis perpendicular bisector and s-coord
        stVec3 cBisectorS = ClosestPoint(cStartS, cEndS, cVertex);
        afTexCoords[0] = cStartS.Distance(cBisectorS) / (fRadius * 2.0f);
        if (pShadowCoords)
            afTexCoords[0] = VecInterpolate(pShadowCoords[2], pShadowCoords[0], afTexCoords[0]);

        // compute the t axis perpendicular bisector and t-coord
        stVec3 cBisectorT = ClosestPoint(cStartT, cEndT, cVertex);
        afTexCoords[1] = cStartT.Distance(cBisectorT) / (fRadius * 2.0f);
        if (pShadowCoords)
            afTexCoords[1] = VecInterpolate(pShadowCoords[5], pShadowCoords[1], afTexCoords[1]);

        // add the verex coord to the geometry
        pGeometry->AddVertexTexCoord(CSpeedTreeRT::TL_SHADOW, afTexCoords);
    }
}


///////////////////////////////////////////////////////////////////////  
//  CProjectedShadow::ClosestPoint definition

stVec3 CProjectedShadow::ClosestPoint(const stVec3& cStart, const stVec3& cEnd, const stVec3& cVertex) const
{
    if (cStart == cEnd)
        return cStart;

    stVec3 cDirection = cEnd - cStart;
    cDirection.Normalize( );
    stVec3 cTmp = cVertex - cStart;
    const float fT = cTmp.Dot(cDirection);

    return cStart + cDirection * fT;
}

