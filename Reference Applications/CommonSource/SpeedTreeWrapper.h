///////////////////////////////////////////////////////////////////////  
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
//      Web: http://www.idvinc.com

#pragma once
#include "SpeedTreeRT.h"
#include "SpeedTreeBillboard.h"
#include "IdvTexture.h"
#include "IdvMaterial.h"
#include "IdvGeometryBuffer.h"
#include "stl_vector.h"
#include "SpeedTreeMemory.h"
#define SPEEDTREE_FRAMEWORK


// forward references
class CSpeedTreeInstance;


///////////////////////////////////////////////////////////////////////  
//  class CSpeedTreeWrapper declaration

class CSpeedTreeWrapper : public CSpeedTreeRT
{
public:
                                            CSpeedTreeWrapper( );
                                            ~CSpeedTreeWrapper( );

        // construction
        st_string                           LoadSptFile(const char* pFilename, int nNumWindMatrices, unsigned int nSeed = 1, float fSize = -1.0f, float fSizeVariance = -1.0f);
        void                                MakeInstance(CSpeedTreeInstance* pInstance);
        float                               CullRadius(void) const                      { return m_fCullRadius; }
        const st_string&                    GetName(void) const                         { return m_strName; }

        // textures
        const CIdvTexture&                  GetBranchMap(ETextureLayers eLayer) const   { st_assert(eLayer >= 0 && eLayer < CSpeedTreeRT::TL_SHADOW); return m_texBranchMaps[eLayer]; }

        // lighting
        const CIdvMaterial&                 GetBranchMaterial(void) const               { return m_cBranchMaterial; }
        const CIdvMaterial&                 GetFrondMaterial(void) const                { return m_cFrondMaterial; }
        const CIdvMaterial&                 GetLeafMaterial(void) const                 { return m_cLeafMaterial; }
        const SLightShaderParams&           GetLightScales(void) const                  { return m_sLightScalars; }

        // wind
        float                               GetLeafRockScalar(void) const               { return m_fLeafRockScalar; }
        float                               GetLeafRustleScalar(void) const             { return m_fLeafRustleScalar; }

        // geometry
        struct SDrawData
        {
            SDrawData( ) :
                // branches
                m_nBranchVertexDataStart(-1),
                m_pBranchIndexDataStart(NULL),
                m_pBranchStripLengths(NULL),
                // fronds
                m_nFrondVertexDataStart(-1),
                m_pFrondIndexDataStart(NULL),
                m_pFrondStripLengths(NULL),
                // leaves
                m_nLeafNumLods(0),
                m_pLeafCardVertexDataStart(NULL),
                m_pLeafCardVertexDataLengths(NULL),
                m_nLeafMeshVertexDataStart(-1),
                m_pLeafMeshIndexDataStart(NULL),
                m_pLeafMeshTriListLengths(NULL),
                // billboards
                m_pTexCoords360(NULL),
                m_nNum360Billboards(0)
            {
            }
            ~SDrawData( )
            {
                st_delete_array<int>(m_pBranchIndexDataStart, "CSpeedTreeWrapper::~SDrawData, m_pBranchIndexDataStart");
                st_delete_array<int>(m_pBranchStripLengths, "CSpeedTreeWrapper::~SDrawData, m_pBranchStripLengths");
                st_delete_array<int>(m_pFrondIndexDataStart, "CSpeedTreeWrapper::~SDrawData, m_pFrondIndexDataStart");
                st_delete_array<int>(m_pFrondStripLengths, "CSpeedTreeWrapper::~SDrawData, m_pFrondStripLengths");
                st_delete_array<int>(m_pLeafCardVertexDataStart, "CSpeedTreeWrapper::~SDrawData, m_pLeafCardVertexDataStart");
                st_delete_array<int>(m_pLeafCardVertexDataLengths, "CSpeedTreeWrapper::~SDrawData, m_pLeafCardVertexDataLengths");
                st_delete_array<int>(m_pLeafMeshIndexDataStart, "CSpeedTreeWrapper::~SDrawData, m_pLeafMeshIndexDataStart");
                st_delete_array<int>(m_pLeafMeshTriListLengths, "CSpeedTreeWrapper::~SDrawData, m_pLeafMeshTriListLengths");
                st_delete_array<float>(m_pTexCoords360, "CSpeedTreeWrapper::~SDrawData, m_pTexCoords360");
            }

            // branches
            int                             m_nBranchVertexDataStart;
            int*                            m_pBranchIndexDataStart;
            int*                            m_pBranchStripLengths;

            // fronds
            int                             m_nFrondVertexDataStart;
            int*                            m_pFrondIndexDataStart;
            int*                            m_pFrondStripLengths;

            // leaves
            int                             m_nLeafNumLods;
            // cards
            int*                            m_pLeafCardVertexDataStart;
            int*                            m_pLeafCardVertexDataLengths;
            // meshes
            int                             m_nLeafMeshVertexDataStart;
            int*                            m_pLeafMeshIndexDataStart;
            int*                            m_pLeafMeshTriListLengths;

            // billboards
            float*                          m_pTexCoords360;
            int                             m_nNum360Billboards;
        };

        void                                SetupBranchGeometry(CIdvTightlyPackedBuffer& cBuffer, const CSpeedWind& cWind);
        void                                SetupFrondGeometry(CIdvTightlyPackedBuffer& cBuffer, const CSpeedWind& cWind);
        void                                SetupLeafCardGeometry(CIdvTightlyPackedBuffer& cGeometry, const CSpeedWind& cWind);
        void                                SetupLeafMeshGeometry(CIdvTightlyPackedBuffer& cGeometry, const CSpeedWind& cWind);
        void                                SetupBillboardGeometry(int nTexCoordsTableOffset);

        const SDrawData&                    GetDrawData(void) const                     { return m_sDrawData; }
        SSpeedTreeBillboard&                GetVertBillboardBuffer(void)                { return m_cVertBillboardVertices; }
#ifdef HORZ_BILLBOARDS
        SSpeedTreeBillboard&                GetHorzBillboardBuffer(void)                { return m_cHorzBillboardVertices; }
        const SGeometry::SHorzBillboard&    GetHorzBillboard(void) const                { return m_sHorzBillboard; }
#endif

        int                                 SortIndex(void) const                       { return m_nSortIndex; }
        void                                SetSortIndex(int nIndex)                    { m_nSortIndex = nIndex; }

        bool                                HasBranchGeometry(void) const               { return m_bHasBranchGeometry; }
        bool                                HasFrondGeometry(void) const                { return m_bHasFrondGeometry; }
        bool                                HasLeafCardGeometry(void) const             { return m_bHasLeafCardGeometry; }
        bool                                HasLeafMeshGeometry(void) const             { return m_bHasLeafMeshGeometry; }
        bool                                BranchesReceiveShadows(void) const          { return m_bBranchesReceiveShadows; }
        bool                                FrondsReceiveShadows(void) const            { return m_bFrondsReceiveShadows; }

#ifdef INTERMEDIATE_FRONDBRANCH_FADING
        float                               BranchFadeDistance(void) const              { return m_fBranchFadeDistance; }
        float                               FrondFadeDistance(void) const               { return m_fFrondFadeDistance; }
#endif

private:
        void                                ComputeCullRadius(void);
        void                                LoadBranchTextures(const st_string& strTexturePath);
        void                                FillMissingTexcoords(CIdvTightlyPackedBuffer& cBuffer, const st_string& strLayerName, int nLayer, int nNumCoords, int nNumVerts);

        st_string                           m_strName;                  // filename of the SPT used to create the wrapper class

        // textures
        CIdvTexture                         m_texBranchMaps[CSpeedTreeRT::TL_NUM_TEX_LAYERS]; // texture for each layer

        // lighting materials (simple diffuse, ambient, etc. colors)
        CIdvMaterial                        m_cBranchMaterial;          // lighting material for branch geometry (diffuse & ambient)
        CIdvMaterial                        m_cFrondMaterial;           // lighting material for frond geometry (diffuse & ambient)
        CIdvMaterial                        m_cLeafMaterial;            // lighting material for leaf geometry (diffuse & ambient)
        SLightShaderParams                  m_sLightScalars;            // values controlled in SpeedTreeCAD for adjusting overall brightness

        // geometry
        SDrawData                           m_sDrawData;                // reference points for unified forest VBs and IBs for this wrapper object
        float                               m_fCullRadius;              // spherical culling algorithms are used - this radius will encompass the tree
        SSpeedTreeBillboard                 m_cVertBillboardVertices;   // used as a template for updating vertical (standard) billboards of this type tree
#ifdef HORZ_BILLBOARDS
        SSpeedTreeBillboard                 m_cHorzBillboardVertices;   // used as a template for updating horizontal billboards of this type tree
        SGeometry::SHorzBillboard           m_sHorzBillboard;           // copy of the static horizontal billboard data from RT
#endif
        bool                                m_bHasBranchGeometry;       // used to avoid expensive context switches if possible
        bool                                m_bHasFrondGeometry;        // used to avoid expensive context switches if possible
        bool                                m_bHasLeafCardGeometry;     // used to avoid expensive context switches if possible
        bool                                m_bHasLeafMeshGeometry;     // used to avoid expensive context switches if possible
        bool                                m_bBranchesReceiveShadows;  // used to avoid shadow texture bleed onto branches that don't receive shadows
        bool                                m_bFrondsReceiveShadows;    // used to avoid shadow texture bleed onto fronds that don't receive shadows
#ifdef INTERMEDIATE_FRONDBRANCH_FADING
        float                               m_fBranchFadeDistance;
        float                               m_fFrondFadeDistance;
#endif

        // wind
        float                               m_fLeafRockScalar;          // one rock/rustle table is uploaded for all trees, these values help
        float                               m_fLeafRustleScalar;        // scale the table up or down to how the tree was tuned in CAD

        int                                 m_nSortIndex;               // helps sort instances by base tree type to avoid redundant context switches
};


///////////////////////////////////////////////////////////////////////  
//  SpeedTree memory management
//
//  Used to capture allocations in STL containers to report back to
//  SpeedTreeRT's allocation mechanism

DefineAllocator(st_allocator_stwrapper_p);
typedef std::vector<CSpeedTreeWrapper*, st_allocator_stwrapper_p<CSpeedTreeWrapper*> > st_vector_stwrapper_p;

