///////////////////////////////////////////////////////////////////////  
//  LeafGeometry.h
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
#include "StructsSupport.h"
#include "StructsInfo.h"
#include "BillboardLeaf.h"
#include "SpeedTreeRT.h"
#include "IndexedGeometry.h"


// forward references
class CWindEngine;
class CTreeEngine;

// types
typedef CSpeedTreeRT::SGeometry::SLeaf::SCard SLeafCard;

// ps3 (can't define allocator inside class decl)
DefineAllocator(st_allocator_meshleaf);
typedef std::vector<CSpeedTreeRT::SGeometry::SLeaf::SMesh, st_allocator_meshleaf<CSpeedTreeRT::SGeometry::SLeaf::SMesh> > st_vector_meshleaf;


///////////////////////////////////////////////////////////////////////  
//  class CLeafGeometry declaration
//
//  This class is used as a geometry abstraction mechanism.  The leaves are
//  computed in the CBranch and CTreeEngine classes, but the geometry
//  is stored and accessed with CLeafGeometry.  It contains the code
//  necessary for formatting the leaf data for use within different rendering
//  APIs and/or coordinate systems.

class CLeafGeometry
{
public:
        CLeafGeometry(CWindEngine* pWindEngine);
        ~CLeafGeometry( );

        void                    Invalidate(void);
        void                    EnableManualLighting(bool bFlag)                    { m_bManualLighting = bFlag; }
        void                    EnableVertexWeighting(bool bFlag)                   { m_bVertexWeighting = bFlag; }
        void                    SetTreeEngine(CTreeEngine* pTreeEngine)             { m_pTreeEngine = pTreeEngine; }
        const float*            GetLeafBillboardTable(unsigned int& nEntryCount) const;
        void                    ComputeExtents(stRegion& cExtents) const;
        void                    GetTextureCoords(unsigned int nLeafMapIndex, float* pTexCoords) const;
        unsigned int            GetTriangleCount(unsigned int nLodLevel) const;
        void                    Transform(const stTransform& cTransform);
        int                     GetNumLodLevels(void) const                         { return m_nNumLods; }
        void                    Init(int nNumLeafLods, const st_vector_leaves* pvLeafLods, const SIdvLeafInfo& sLeafInfom, float fSizeIncreaseFactor);
        bool                    IsVertexWeightingEnabled(void) const                { return m_bVertexWeighting; }
        void                    SetTextureCoords(unsigned int nLeafMapIndex, const float* pTexCoords);
        void                    Update(CSpeedTreeRT::SGeometry::SLeaf& sGeometry, int nLodLevel, float fAzimuth, float fPitch, float fSizeIncreaseFactor);

        void                    DeleteGeometry(void);


        ///////////////////////////////////////////////////////////////////////  
        //  struct SLodGeometry declaration
        //
        //  This structure is used to store a single leaf LOD - an array of them
        //  is stored within the CLeafGeometry class.  The member variables change
        //  depending on the API/engine defined during compilation.

        struct SLodGeometry : public CSpeedTreeRT::SGeometry::SLeaf
        {
            SLodGeometry( );
            ~SLodGeometry( );

            void                    Invalidate(void)                                    { m_bValid = false; }
            bool                    IsValid(void) const                                 { return m_bValid; }
            void                    Validate(void)                                      { m_bValid = true; }
                                    
            bool                    m_bValid;               // used in CPU wind and billboard computations to eliminate redundant computations
            float*                  m_pOrigCenterCoords;    // used in CPU wind computations as reference point
            
            st_vector_meshleaf      m_vMeshes;
        };

private:
        void                    ComputeWindEffect(int nLodLevel);
        void                    InitLods(int nNumLeafLods, const st_vector_leaves* pvLeafLods, float fSizeIncreaseFactor);
        void                    InitClusters(SLodGeometry& sLod, float fSizeIncreaseFactor);

        bool                    m_bManualLighting;      // flag for manual lighting mode
        bool                    m_bVertexWeighting;     // flag for vertex weight mode

        // tree engine
        CTreeEngine*            m_pTreeEngine;          // pointer to the tree engine

        // wind information
        CWindEngine*            m_pWindEngine;          // pointer used to modify leaf geometry when WIND_CPU is used

        // compute tables
        int                     m_nNumRockingGroups;    // quantity of separate leaf rocking groups
        float*                  m_pTimeOffsets;         // time offsets for trig functions to keep groups from rocking in sync
        float**                 m_pLeafVertexes;        // points to the table of leaf rocking groups (all centered at origin)
        float*                  m_pLeafTexCoords;       // points to the table of texture coordinates for the leaf rocking groups
        float*                  m_pVertexProgramTable;  // used for vertex shaders to place leaf maps

        // texture information
        int                     m_nNumTextures;         // number of unique textures used to render leaves
        stVec3*                 m_pLeafDimensions;      // dimensions for each leaf type (m_nNumTexture long)
        stVec3*                 m_pLeafOrigins;         // origin for each leaf type (m_nNumTexture long)
        
        // LODs
        int                     m_nNumLods;             // total number of leaf LOD levels
        SLodGeometry*           m_pLods;                // pointer to m_nNumLods LOD levels
};

