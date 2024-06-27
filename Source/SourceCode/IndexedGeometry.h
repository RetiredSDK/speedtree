///////////////////////////////////////////////////////////////////////  
//  IndexedGeometry.h
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
#include "Debug.h"
#include <vector>
#include "SpeedTreeRT.h"
#include "LibVector_Source/IdvVector.h"


///////////////////////////////////////////////////////////////////////  
//  class CIndexedGeometry declaration
//
//  This class is used as a geometry abstraction mechanism.  The branches are
//  computed in the CBranch and CTreeEngine classes, but the geometry
//  is stored and accessed with CIndexedGeometry.  It contains the code
//  necessary for formatting the branch data for use within different rendering
//  APIs and/or coordinate systems.

class CIndexedGeometry
{
public:
        CIndexedGeometry(CWindEngine* pWindEngine, bool bRetainTexCoords = false);
        ~CIndexedGeometry( );

        ///////////////////////////////////////////////////////////////////////  
        //  Setting the geometry

        // general
        void                    EnableManualLighting(bool bFlag)                    { m_bManualLighting = bFlag; }
        void                    EnableVertexWeighting(bool bFlag)                   { m_bVertexWeighting = bFlag; }
        void                    CombineStrips(bool bToggleFaceOrdering = false);
        void                    ComputeExtents(stRegion& cExtents) const;
        bool                    IsVertexWeightingEnabled(void) const                { return m_bVertexWeighting; }
        void                    SetNumLodLevels(int nLevels);
        void                    SetWindMethod(CSpeedTreeRT::EWindMethod eMethod)    { m_eWindMethod = eMethod; }
        void                    Transform(const stTransform& cTransform);

        // indexing 
        void                    AddStrip(int nLodLevel, int* pStrip, int nStripLength);

        // vertex 
        void                    AddVertexColor(const float* pColor);                              // 4 floats: (r, g, b, a)
        void                    AddVertexCoord(const float* pCoord);                              // 3 floats: (x, y, z)
        void                    AddVertexNormal(const float* pNormal, bool bUpAxisAdjust = true); // 3 floats: (nx, ny, nz)
        void                    AddVertexWind1(float fWindWeight, unsigned char chWindMatrixIndex);
        void                    AddVertexWind2(float fWindWeight, unsigned char chWindMatrixIndex);
        void                    AddVertexTexCoord(CSpeedTreeRT::ETextureLayers eLayer, const float* pTexCoord, int nIndex = -1);
        void                    AddVertexTangent(const float* pTangent);
        void                    AddVertexBinormal(const float* pBinormal);
        void                    SetLodFadeHint(int nIndex, float fFadeHint);

        // setting & retrieving
        void                    AdvanceStripCounter(void)                           { m_nStripCounter++; }
        int                     GetStripCounter(void)                               { return m_nStripCounter; }
        void                    ResetStripCounter(void)                             { m_nStripCounter = 0; }
        void                    SetVertexCounter(int nCounter)                      { m_nNumVertices = nCounter; }
        int                     GetNumLodLevels(void)                               { return m_nNumLodLevels; }
        void                    Finish(void);

        // vertex counter controls
        void                    AdvanceVertexCounter(void)                          { m_nNumVertices++; }
        int                     GetVertexCounter(void)                              { return m_nNumVertices; }
        void                    ResetVertexCounter(void)                            { m_nNumVertices = 0; }


        ///////////////////////////////////////////////////////////////////////  
        //  Retrieving the geometry

        // indexing
        const int*              GetStrips(int nLodLevel) const;
        const int*              GetStripInfo(int nLodLevel, int& nNumStrips) const;
        int                     GetNumStrips(int nLodLevel) const;
        const int*              GetNumStripsPointer(void) const                     { return (const int*) m_pNumStrips; }
        const int**             GetStripLengthsPointer(void) const                  { return (const int**) m_pStripLengths; }
        const int***            GetStripsPointer(void) const                        { return (const int***) m_pStrips; }

        // tables
        const unsigned int*     GetVertexColors(void) const                         { return m_vColors.empty( ) ? NULL : &(m_vColors[0]); }
        const float*            GetVertexCoords(void) const                         { return m_vCoords.empty( ) ? NULL : &(m_vCoords[0]); }
        const float*            GetVertexNormals(void) const                        { return m_vNormals.empty( ) ? NULL : &(m_vNormals[0]); }
        const float*            GetVertexBinormals(void) const                      { return m_vBinormals.empty( ) ? NULL : &(m_vBinormals[0]); }
        const float*            GetVertexTangents(void) const                       { return m_vTangents.empty( ) ? NULL : &(m_vTangents[0]); }
        const float*            GetVertexWindWeights1(void) const                   { return m_vWindWeights1.empty( ) ? NULL : &(m_vWindWeights1[0]); }
        const unsigned char*    GetVertexWindMatrixIndices1(void) const             { return m_vWindMatrixIndices1.empty( ) ? NULL : &(m_vWindMatrixIndices1[0]); }
        const float*            GetVertexWindWeights2(void) const                   { return m_vWindWeights2.empty( ) ? NULL : &(m_vWindWeights2[0]); }
        const unsigned char*    GetVertexWindMatrixIndices2(void) const             { return m_vWindMatrixIndices2.empty( ) ? NULL : &(m_vWindMatrixIndices2[0]); }
        const float*            GetLodFadeHints(void) const                         { return m_vLodFadeHints.empty( ) ? NULL : &(m_vLodFadeHints[0]); }

        const float*            GetVertexTexCoords(CSpeedTreeRT::ETextureLayers eLayer) const { return m_avTexCoords[eLayer].empty( ) ? NULL : &(m_avTexCoords[eLayer][0]); }

        // vertex utility functions
        int                     GetTriangleCount(int nLodLevel) const;
        const float*            GetVertexCoord(unsigned int nVertexIndex) const;
        int                     GetVertexCount(void) const                          { return static_cast<int>(m_vCoords.size( ) / 3); }
        int                     GetVertexSize(void) const                           { return m_nVertexSize; }
        const unsigned char*    GetVertexTexIndices(void) const                     { return m_vTexIndices0.empty( ) ? NULL : &(m_vTexIndices0[0]); }
        const float*            GetVertexTexCoord(CSpeedTreeRT::ETextureLayers eLayer, unsigned int nVertexIndex) const;


        ///////////////////////////////////////////////////////////////////////  
        //  Misc

        bool                    ComputeWindEffect(int nLodLevel);
        void                    DeleteLodStrips(int nLodLevel);
        void                    Invalidate(void)                                    { m_bValid = false; }
        bool                    IsValid(void) const                                 { return m_bValid; }
        void                    Validate(void)                                      { m_bValid = true; }
        void                    ChangeTexCoord(unsigned char chChangedMapIndex, float* pNewTexCoords);

private:
        // utility functions
        unsigned int            ColorFloatsToUInt(const float* pColor) const;
        void                    DeleteIndexData(void);
        const float*            GetOrigVertexCoord(unsigned int nVertexIndex) const;
        int                     CompositeLength(int nLod, bool bToggleFaceOrdering) const;

        // general
        bool                    m_bRetainTexCoords;                 // used for remapping frond texture coordinates
        CWindEngine*            m_pWindEngine;                      // pointer used to modify branch geometry when WIND_CPU is used
        bool                    m_bVertexWeighting;                 // flag for vertex weight mode
        bool                    m_bManualLighting;                  // flag for manual lighting mode
        bool*                   m_pVertexWindComputed;              // for WIND_CPU - one entry per branch vertex to avoid redundant computations
        int                     m_nVertexSize;                      // size of vertex in bytes
        bool                    m_bValid;                           // is geometry valid for current frame
        CSpeedTreeRT::EWindMethod m_eWindMethod;                    // method used for wind effects
                                                                    
        // geometry                                                 
        int                     m_nNumLodLevels;                    // number of discrete branch LODs
        int                     m_nNumVertices;                     // current vertex - where values are stored when AddVertex*() functions are invoked
        int                     m_nNumVerticesLodLevel;             // which LOD level the m_nNumVertices is referencing
        int                     m_nStripCounter;                    // current strip - where strips are stored when AddStrip() is invoked
        st_vector_int           m_vTriangleCounts;                  // used to store the number of triangles per LOD level
        st_vector_int_vector    m_vStripInfo;
        st_vector_int_vector    m_vAllStrips;
        int*                    m_pNumStrips;
        int**                   m_pStripLengths;
        int***                  m_pStrips;                          // accessed as [lod][strip][vertex]

        // new geometry storage
        st_vector_uint          m_vColors;                          // table of RGBA colors, one color per entry
        st_vector_float         m_vCoords;                          // table of xyz values, three floats per coordinate
        st_vector_float         m_vOrigCoords;                      // table of xyz values used as a basis for CPU wind
        st_vector_float         m_vNormals;                         // table of 3D normal values, three floats per normal
        st_vector_float         m_vBinormals;                       // table of 3D binormal values, used for bump mapping, three floats per normal
        st_vector_float         m_vTangents;                        // table of 3D tangent values, used for bump mapping, three floats per normal
        st_vector_float         m_vOrigTexCoords0;                  // used for composite map manipulation in CAD, not needed by end user
        st_vector_byte          m_vTexIndices0;                     // used for composite map manipulation in CAD & MAX/MAYA, not needed by end user
        st_vector_float         m_vWindWeights1;                    // table of primary weights, values from from 0.0 for rigid to 1.0 for flexible
        st_vector_byte          m_vWindMatrixIndices1;              // table of primary wind matrix indices, ranging from 0 to num_wind_matrices
        st_vector_float         m_vWindWeights2;                    // table of secondary weights, values from from 0.0 for rigid to 1.0 for flexible
        st_vector_byte          m_vWindMatrixIndices2;              // table of secondary wind matrix indices, ranging from 0 to num_wind_matrices
        st_vector_float         m_vLodFadeHints;                    // table of intermediate fade hints [0,1], used to fade/fizzle branch/frond geometry 
                                                                    // before the billboard transition

        st_vector_float         m_avTexCoords[CSpeedTreeRT::TL_NUM_TEX_LAYERS];
};



///////////////////////////////////////////////////////////////////////  
//  Inline function definitions

inline int CIndexedGeometry::GetNumStrips(int nLodLevel) const
{
    st_assert(nLodLevel < int(m_vStripInfo.size( )));

    return (nLodLevel > -1) ? int(m_vStripInfo[nLodLevel].size( ) / 2) : 0;
}

