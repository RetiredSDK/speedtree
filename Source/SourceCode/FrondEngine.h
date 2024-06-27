///////////////////////////////////////////////////////////////////////  
//  FrondEngine.h
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
#include "SpeedTreeRT.h"
#include "LibGlobals_Source/IdvGlobals.h"
#include "LibFilename_Source/IdvFilename.h"
#include "FileAccess.h"
#include <vector>


///////////////////////////////////////////////////////////////////////  
//  struct SFrondVertex declaration

struct SFrondVertex
{
        SFrondVertex( ) :
            m_fCrossSectionWeight1(0.0f),
            m_fCrossSectionWeight2(0.0f),
            m_nWindGroup1(0),
            m_nWindGroup2(0)
        {
        }

        stVec3          m_cPos;                                     // xyz position of the vertex (computed as a branch)
        stRotTransform  m_cTrans;                                   // orientation of the vertex (computed as a branch)
        float           m_fCrossSectionWeight1;                     // primary wind weight of all vertices around this point (computed as a branch)
        float           m_fCrossSectionWeight2;                     // secondry wind weight of all vertices around this point (computed as a branch)
        int             m_nWindGroup1;                              // primary wind group that the frond belongs to
        int             m_nWindGroup2;                              // secondary wind group that the frond belongs to
};

DefineAllocator(st_allocator_frondvertex);
typedef std::vector<SFrondVertex, st_allocator_frondvertex<SFrondVertex> > st_vector_frondvertex;


///////////////////////////////////////////////////////////////////////  
//  struct SFrondGuide declaration

struct SFrondGuide
{
        SFrondGuide( ) :
            m_nNumVertices(0),
            m_nCurrentIndex(0),
            m_pVertices(NULL),
            m_fLength(0.0f),
            m_fRadius(0.0f),
            m_chFrondMapIndex(0),
            m_fOffsetAngle(0.0f),
            m_fSurfaceArea(0.0f),
            m_fFuzzySurfaceArea(0.0f),
            m_nVertexStartIndex(0),
            m_nVerticesPerGuideVertex(0)
        {
        }

        SFrondGuide(const SFrondGuide& cRight)
        {
            m_nNumVertices = cRight.m_nNumVertices;
            m_nCurrentIndex = cRight.m_nCurrentIndex;
            if (cRight.m_pVertices == cRight.m_asStackVertices)
            {
                memcpy(m_asStackVertices, cRight.m_asStackVertices, c_nMaxStackVertices * sizeof(SFrondVertex));
                m_pVertices = m_asStackVertices;
            }
            else if (cRight.m_pVertices != NULL)
            {
                m_vVertices = cRight.m_vVertices;
                m_pVertices = &m_vVertices[0];
            }
            else
                m_pVertices = NULL;
            m_fLength = cRight.m_fLength;
            m_fRadius = cRight.m_fRadius;
            m_chFrondMapIndex = cRight.m_chFrondMapIndex;
            m_fOffsetAngle = cRight.m_fOffsetAngle;
            m_fSurfaceArea = cRight.m_fSurfaceArea;
            m_fFuzzySurfaceArea = cRight.m_fFuzzySurfaceArea;
            m_nVertexStartIndex = cRight.m_nVertexStartIndex;
            m_nVerticesPerGuideVertex = cRight.m_nVerticesPerGuideVertex;
        }

        enum { c_nMaxStackVertices = 8 };

        int                         m_nNumVertices;
        int                         m_nCurrentIndex;
        SFrondVertex*               m_pVertices;
        SFrondVertex                m_asStackVertices[c_nMaxStackVertices];
        st_vector_frondvertex       m_vVertices;                    // all of the vertices in this guide (guides are frond "center lines")

        float                       m_fLength;                      // length of the guide
        float                       m_fRadius;                      // radius of the branch that generated this guide
        unsigned char               m_chFrondMapIndex;              // specifies which of the "n" frond maps this frond is associated with
        float                       m_fOffsetAngle;                 // angle by which frond is rotated around the "center line"
        float                       m_fSurfaceArea;                 // surface area of frond (factors heavily in lod)
        float                       m_fFuzzySurfaceArea;            // modified surface area to add randomness to frond disappearance due to lod
        unsigned int                m_nVertexStartIndex;            // location of the fronds position in the indexed geometry class
        unsigned int                m_nVerticesPerGuideVertex;      // number of vertices associated with each guide vertex
};

DefineAllocator(st_allocator_frondguide);
typedef std::vector<SFrondGuide, st_allocator_frondguide<SFrondGuide> > st_vector_frondguide;

DefineAllocator(st_allocator_frondguide_p);
typedef std::vector<SFrondGuide*, st_allocator_frondguide<SFrondGuide*> > st_vector_frondguide_p;

DefineAllocator(st_allocator_frondguide_p_vector);
typedef std::vector<st_vector_frondguide_p, st_allocator_frondguide_p_vector<st_vector_frondguide_p> > st_vector_frondguide_p_vector;


///////////////////////////////////////////////////////////////////////  
//  SFrondTexture

struct SFrondTexture
{
        SFrondTexture( ) :
            m_fAspectRatio(0.5f),
            m_fSizeScale(1.0f),
            m_fMinAngleOffset(0.0f),
            m_fMaxAngleOffset(0.0f)
        
        {
        }

        st_string       m_strFilename;                              // texture map filename
        float           m_fAspectRatio;                             // aspect ratio (widht/height) of the texture
        float           m_fSizeScale;                               // fronds are sized based on length of the guide, this value scales that size
        float           m_fMinAngleOffset;                          // minimum angle offset (from SpeedTreeCAD)
        float           m_fMaxAngleOffset;                          // maximum angle offset (from SpeedTreeCAD)
};

DefineAllocator(st_allocator_frondtexture);
typedef std::vector<SFrondTexture, st_allocator_frondtexture<SFrondTexture> > st_vector_frondtexture;


///////////////////////////////////////////////////////////////////////  
//  class CFrondEngine declaration

class CFrondEngine
{
public:
        enum EConditions
        {
            ENABLED, DISABLED, PRUNED
        };

        // construction/destruction
        CFrondEngine( );
        CFrondEngine(const CFrondEngine& cRight);
        ~CFrondEngine( );
                                                
        // general                              
        int                             GetLevel(void) const;
        bool                            Enabled(void) const;
        void                            Compute(CIndexedGeometry* pGeometry, CLightingEngine* pLightingEngine);
        void                            GetRules(float& fDistance, unsigned int& uiLevel, EConditions& eAbove, EConditions& eBelow) const;
        unsigned int                    GetSegmentOverride(void) const      { return m_uiSegmentOverride + 1; }
        bool                            IsUsingSegmentOverride(void) const  { return m_bUseSegmentOverride; }

                                                
        unsigned int                    GetTextureCount(void) const                                     { return (unsigned int) m_vTextures.size( ); }
        const char*                     GetTextureFilename(unsigned int nIndex) const                   { return m_vTextures[nIndex].m_strFilename.c_str( ); }
        void                            SetTextureFilename(unsigned int nIndex, const char* pFilename)  { m_vTextures[nIndex].m_strFilename = pFilename; }
        void                            SetTextureCoords(CIndexedGeometry* pGeometry, unsigned int nFrondMapIndex, const float* pTexCoords, bool bFlip);
                                                
        // frond guides                         
        void                            StartGuide(int nNumVertices);
        void                            AddGuideVertex(stVec3 cPos, stRotTransform cTrans, float fCrossSectionWeight1, float fCrossSectionWeight2, int nWindGroup1, int nWindGroup2);
        void                            EndGuide(float fLodSizeScalar);
                                                
        // spt parsing                          
        void                            Parse(CTreeFileAccess& cFile);
        void                            Save(CTreeFileAccess& cFile) const;
        void                            ParseSupplementalInfo(CTreeFileAccess& cFile);
        void                            SaveSupplementalInfo(CTreeFileAccess& cFile) const;

        // lod fading
        void                            ProhibitSegmentReduction(void);
                                                
private:                                        
        void                            BuildBladeVertices(SFrondGuide& sGuide);
        void                            ComputeBlade(unsigned int nLod, unsigned int nStart, SFrondGuide& sGuide);
        void                            BuildExtrusionVertices(SFrondGuide& sGuide);
        void                            ComputeExtrusion(unsigned int nLod, unsigned int nStart, SFrondGuide& sGuide);//, unsigned short* pStrip);
        void                            BuildProfileVectors(SFrondGuide& sGuide, st_vector_stVec3& vProfile, st_vector_stVec3& vNormals, st_vector_stVec3& vTangents);
        stBezierSpline*                 GetProfile(void) const              { return m_pProfile; }
        void                            BuildGuideLods(void);
        int                             ExtrusionStripLength(unsigned int uiLod, const SFrondGuide& sGuide);

        // general                              
        CIndexedGeometry*               m_pGeometry;                    // this is the class that holds all vertices, texture coordinates, triangles, etc.
        CLightingEngine*                m_pLightingEngine;              // class used to compute static lighting
                                                
        // frond guides                         
        st_vector_frondguide            m_vGuides;                      // vector of guides (guides represent the "center line" of a frond)
        st_vector_frondguide_p_vector   m_vGuideLods;                   // this vector holds gradually lower resolution versions of the guides

        // frond parameters in the .spt data
        enum EFrondType
        {
            FROND_BLADES, FROND_EXTRUSION, NUM_FROND_TYPES
        };

        EFrondType                      m_eFrondType;                   // type of frond (blade or extrusion)
        unsigned int                    m_nNumBlades;                   // number of blades
        stBezierSpline*                 m_pProfile;                     // shape of cross-section of extruded fronds
        unsigned int                    m_nProfileSegments;             // number of segments in the cross-section
        unsigned int                    m_nLevel;                       // branch level at which fronds become active
        bool                            m_bEnabled;                     // whether or not fronds are enabled
        st_vector_frondtexture          m_vTextures;                    // the CAD-specified frond maps
        unsigned int                    m_nNumLodLevels;                // number of discrete frond LODs
        float                           m_fMaxSurfaceAreaPercent;       // the percentage of the total frond area present at the highest detail
        float                           m_fMinSurfaceAreaPercent;       // the percentage of the total frond area present at the lowest 3D detail
        float                           m_fReductionFuzziness;          // controls how strictly size is used as the LOD judge (higher numbers = more random)
        float                           m_fLargeRetentionPercent;       // controls the percentage of "large" fronds that must remain
        unsigned int                    m_nMinLengthSegments;           // for LOD computation, this value represents the minimum number of lengthwise sections
        unsigned int                    m_nMinCrossSegments;            // for LOD computation, this value represents the minimum number of cross-sectional sections

        unsigned int                    m_uiDistanceLevel;              // controls many branch levels back to look for the cutoff
        float                           m_fDistancePercent;             // controls how far along the specified branch level the cutoff occurs
        EConditions                     m_eAboveCondition;              // controls frond generation above the cutoff
        EConditions                     m_eBelowCondition;              // controls frond generation below the cutoff
        unsigned int                    m_uiSegmentOverride;            // overrides branch settings to control frond length segments
        bool                            m_bUseSegmentOverride;          // toggles the use of the segment override

        bool                            m_bProhibitSegmentReduction;    // prevent lower LODs from having fewer segments so LOD fading works properly

        st_vector_float                 m_vLengths;
        st_vector_float_vector          m_vRunningLengths;
        st_vector_int                   m_vCurrentStrip;
static  int                             m_nRefCount;
static  st_vector_stVec3                m_vRightDirections;
static  st_vector_stVec3                m_vProfile;
static  st_vector_stVec3                m_vNormals;
static  st_vector_stVec3                m_vTangents;
static  st_vector_float                 m_vSideLengths;
};

