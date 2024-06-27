///////////////////////////////////////////////////////////////////////  
//  BillboardLeaf.h
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
#include "LibVector_Source/IdvVector.h"
#include "FileAccess.h"


///////////////////////////////////////////////////////////////////////  
//  class CBillboardLeaf declaration

class CBillboardLeaf
{
public:
        CBillboardLeaf(const stVec3& cPos, short nColorScale, int nAngleIndex, float fWindWeight1, float fWindWeight2, int nWindGroup1, int nWindGroup2);

        // inspectors
        CBillboardLeaf*     Clone(void) const;
        int                 GetAngle(void) const                                { return m_ucAngleIndex; }
        stVec3              GetColor(int nIndex) const;
        unsigned int        GetColorUInt(int nIndex) const;
        float               GetDimming(void) const                              { return float(m_ucColorScale) / 255.0f; }
        const stVec3&       GetNormal(int nIndex) const                         { return m_acNormals[nIndex]; }
        int                 GetTextureIndex(void) const                         { return m_ucTextureIndex / 2; }
        int                 GetAltTextureIndex(void) const                      { return m_ucTextureIndex; }
        float               GetWeight1(void) const                              { return m_fWindWeight1; }
        float               GetWeight2(void) const                              { return m_fWindWeight2; }
        unsigned char       GetWindGroup1(void) const                           { return static_cast<unsigned char>(m_nWindGroup1); }
        unsigned char       GetWindGroup2(void) const                           { return static_cast<unsigned char>(m_nWindGroup2); }
        const stVec3&       GetBinormal(int nIndex) const                       { return m_acBinormals[nIndex]; }
        const stVec3&       GetTangent(int nIndex) const                        { return m_acTangents[nIndex]; }
        const stVec3&       GetPosition(void) const                             { return m_cPosition; }

        // mutators
        void                AdjustStaticLighting(const stVec3& cTreeCenter, const stVec3& cLightPos, float fLightScalar);
        void                SetColor(const stVec3& cColor, int nIndex, bool bApplyDimming = true);
        void                SetNormal(const stVec3& cNormal, int nIndex)        { m_acNormals[nIndex] = cNormal; }
        void                SetTextureIndex(int nTextureIndex)                  { m_ucTextureIndex = static_cast<unsigned char>(nTextureIndex); }
        void                SetBinormal(const stVec3& cBinormal, int nIndex)    { m_acBinormals[nIndex] = cBinormal; }
        void                SetTangent(const stVec3& cTangent, int nIndex)      { m_acTangents[nIndex] = cTangent; }
        void                SetPosition(const stVec3& cPosition)                { m_cPosition = cPosition; }

        // file operations
        void                Save(CTreeFileAccess& cFile) const;
static  void                Parse(CTreeFileAccess& cFile, CBillboardLeaf& cLeaf);

        CBillboardLeaf( );                      // internal - used by Clone() and Parse() only
private:

        unsigned char       m_ucAngleIndex;     // which leaf angle group this leaf belongs to
        unsigned char       m_ucColorScale;     // 0 - 255 (dark to light)
        unsigned int        m_auiColors[4];
        stVec3              m_acNormals[4];
        stVec3              m_acTangents[4];
        stVec3              m_acBinormals[4];
        stVec3              m_cPosition;
        unsigned char       m_ucTextureIndex;   // which texture index this leaf belongs to
        float               m_fWindWeight1;     // primary wind weighting, 0.0 = rigid, 1.0 = full wind effect
        float               m_fWindWeight2;     // secondary wind weighting, 0.0 = rigid, 1.0 = full wind effect
        int                 m_nWindGroup1;      // which primary wind matrix affects this leaf
        int                 m_nWindGroup2;      // which secondary wind matrix affects this leaf
};

DefineAllocator(st_allocator_leaves);
typedef std::vector<CBillboardLeaf, st_allocator_leaves<CBillboardLeaf> > st_vector_leaves;

