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


///////////////////////////////////////////////////////////////////////  
//  Preprocessor

#pragma once
#include "stl_vector.h"
#include "SpeedTreeInstance.h"
#include <cfloat>
#include "IdvVectorMath.h"
#include <functional>


// forward references
class CIdvInterleavedBuffer;
class CIdvConstant;
class CIdvEffect;


///////////////////////////////////////////////////////////////////////  
//  SpeedTree memory management
//
//  Used to capture allocations in STL containers to report back to
//  SpeedTreeRT's allocation mechanism

DefineAllocator(st_allocator_vertbuf);


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine


class CSpeedTreeCullEngine
{
public:
        CSpeedTreeCullEngine( );
        ~CSpeedTreeCullEngine( );


        ///////////////////////////////////////////////////////////////////////  
        //  ECullStatus
        //
        //  Used to describe the cull status of an individual cell as something
        //  beyond culled or not culled.  Knowing that a cell is partially culled
        //  is important for optimizations down the road.

        enum ECullStatus
        {
            CULLED_NONE,            // the cell is fully inside the frustum
            CULLED_PARTIAL,         // the cell is partly in the frustum and partly out
            CULLED_COMPLETE         // the cell is fully outside of the frustum
        };


        ///////////////////////////////////////////////////////////////////////  
        //  ELodStatus
        //
        //  Used to describe the collective status of the trees in a cell.  An
        //  important part of the speed in this system is knowing that a cell is
        //  far enough away that the entire cell is composed of billboards.

        enum ELodStatus
        {
            LOD_ALL_BILLBOARDS,     // every tree in the cell is at LOD level 0.0
            LOD_MIX                 // the cell is within range such that there will
                                    // be a mixture of LOD levels
        };


        ///////////////////////////////////////////////////////////////////////  
        //  SCell
        //
        //  A forest is broken down into a grid of cells - somewhere similar to
        //  a quad tree with the exception that there is just one level.  Each
        //  cell has its own culling and LOD status that can, in some cases, be 
        //  used to bypass individual tree culling/LOD calculations.

        struct SCell
        {
            SCell( );

            int                     m_nRow;                 // the cell's row on the forest grid 
            int                     m_nCol;                 // the cell's column on the forest grid 
            float                   m_afExtents[6];         // [0-2] = min(x,y,z), [3-5] = max(x,y,z) [includes tree overhang]
            float                   m_afRoughExtents[6];    // [0-2] = min(x,y,z), [3-5] = max(x,y,z) [excludes tree overhang]
            float                   m_afCenter[3];          // average of the extents
            float                   m_fCullRadius;          // distance from the center to corner of cell (plus a
                                                            // small fudge factor to compensate for tree overhang)
            ECullStatus             m_eCullStatus;          // completely culled, partially culled, or not culled
            ELodStatus              m_eLodStatus;           // all billboards, or mixture (only useful if m_eCullStatus != CULLED_COMPLETE)

            st_vector_stinstance_p  m_vTrees;               // instances held in this cell, sorted by base tree type
            st_vector_int           m_vBaseIndices;         // same length as CSpeedTreeForest::m_vBaseTrees, stores index where each
                                                            // base type begins in m_vTrees
            int                     m_nNumBBs;              // used when batch rendering vertical (standard) billboards
            CIdvInterleavedBuffer*  m_pBillboardVB;         // if !NULL, used to batch render all billboards in this cell
        };


        ///////////////////////////////////////////////////////////////////////  
        //  SpeedTree memory management
        //
        //  Used to capture allocations in STL containers to report back to
        //  SpeedTreeRT's allocation mechanism

        DefineAllocator(st_allocator_cellp);
        typedef std::vector<SCell*, st_allocator_cellp<SCell*> > st_vector_cellp;

        DefineAllocator(st_allocator_vertbuf);
        typedef std::list<CIdvInterleavedBuffer*, st_allocator_vertbuf<CIdvInterleavedBuffer*> > st_list_vertbuf;

        DefineAllocator(st_allocator_cellmap);
        typedef std::map<CIdvInterleavedBuffer*, SCell*, std::less<CIdvInterleavedBuffer*>, st_allocator_cellmap<std::pair<CIdvInterleavedBuffer*, SCell*> > > st_map_cells;

        // setup
        bool                        Init(CSpeedTreeInstance* pTrees, int nNumTrees, int nNumBaseTrees, int nNumCellRows, int nNumCellCols, const idv::Region& cExtents = idv::Region());
        void                        SetProjection(float fAspectRatio, float fFieldOfView, float fNear, float fFar); // FOV in degrees
        void                        SetLodDistances(float fNear, float fFar)        { m_fNearLod = fNear; m_fFarLod = fFar; }

        // render loop
        void                        SetCamera(const float* pCameraPos, const float* pModelviewMatrix, const float* pProjection);
        void                        Cull(void);

        // results accessors
        CSpeedTreeInstance*         First3dTree(int nBaseIndex);
        CSpeedTreeInstance*         Next3dTree(void);   // NULL after last tree
        const CIdvInterleavedBuffer* FirstBillboardCell(int& nNumBBs);
        const CIdvInterleavedBuffer* NextBillboardCell(int& nNumBBs);

        // helpful / diagnostics
        void                        DrawOverlay(CIdvEffect* pEffect, CIdvConstant* pColor);
        void                        DrawActiveCells(CIdvEffect* pEffect, CIdvConstant* pColor);
        void                        GetFrustumCenter(float afCenter[3]);
        void                        GetFrustumExtents(float afMin[3], float afMax[3]);

private:
        SCell*                      Cell(int nRow, int nCol) const;     
        SCell*                      PosToCell(const float* pPos) const; 
        void                        DeleteCells(void);
        void                        ComputeExtents(const CSpeedTreeInstance* pTrees, int nNumTrees);
        void                        ComputeFrustum(const float* pProjection, const float* pModelview);
        ECullStatus                 CullStatus(SCell* pCell) const;
        bool                        CullStatus(const CSpeedTreeInstance* pTree) const;
        void                        ComputeCellLod(SCell* pCell) const;
        void                        ComputeTreesLod(SCell* pCell) const;
        void                        NextCell(void);
        void                        ProcessBillboards(void);
        void                        PopulateVertexBuffer_AllBBs(CIdvInterleavedBuffer* pVB, SCell* pCell);
        void                        PopulateVertexBuffer_PartialBBs(CIdvInterleavedBuffer* pVB, SCell* pCell);
        CIdvInterleavedBuffer*      NextRenderBuffer(void);

        // cell overlay rendering functions
        void                        Draw3dCellsViaBuffer(CIdvEffect* pEffect, CIdvConstant* pColor);
        void                        Draw2dBackground(CIdvEffect* pEffect, CIdvConstant* pColor);
        void                        Draw2dFlatGrid(CIdvEffect* pEffect, CIdvConstant* pColor);
        void                        Draw2dFrustum(CIdvEffect* pEffect, CIdvConstant* pColor);
        void                        Draw2dActiveCells(CIdvEffect* pEffect, CIdvConstant* pColor);
        void                        Draw2dMeshTrees(CIdvEffect* pEffect, CIdvConstant* pColor);
        void                        Draw2dBillboardTrees(CIdvEffect* pEffect, CIdvConstant* pColor);
        int                         CheckForOverflow(int nNumPoints);

        // cells
        SCell*                      m_pCells;                   // array of cells encompassing entire forest
        int                         m_nNumCellRows;             // number of rows of cells
        int                         m_nNumCellCols;             // number of columns of cells
        int                         m_nNumCells;                // rows * cols
        st_vector_cellp             m_vVisibleCells;            // vector of visible cell pointers

        // forest
        float                       m_afForestExtents[6];       // extents for entire forest, [0-2] = min(x,y,z), [3-5] = max(x,y,z)
        int                         m_nNumBaseTrees;            // same as CSpeedTreeForest::m_vBaseTrees, stores index where each
        int                         m_nCurrentBaseIndex;        // used by First/Next3dTree()
        int                         m_nCurrentTree;             // used by First/Next3dTree()
        int                         m_nCurrentCell;             // used by First/NextBillboardCell()
        st_map_cells::const_iterator m_iCurrentBillboardCell;   // used by First/NextBillboardCell()
        float                       m_fLongestTreeExtent;       // longest possible overhang of all instances

        // camera / projection
        float                       m_fAspectRatio;             // screen width / screen height
        float                       m_fFieldOfView;             // in degrees
        float                       m_fNearPlane;               // near clipping plane
        float                       m_fFarPlane;                // far clipping plane
        float                       m_fNearLod;                 // near SpeedTreeRT LOD value
        float                       m_fFarLod;                  // far SpeedTreeRT LOD value
        float                       m_afCameraPos[3];           // camera position, extracted from modelview matrix
        float                       m_afCameraRight[3];         // camera right axis, extracted from modelview matrix
        float                       m_afCameraUp[3];            // camera up axis, extracted from modelview matrix
        float                       m_afCameraOut[3];           // camera out axis, extracted from modelview matrix

        // derived frustum values
        float                       m_afFrustumPlanes[6][4];    // defines the (x,y,z,d) values for six plane defining the frustum
        float                       m_afFrustumMin[3];          // axis aligned frustum box (min, world-space)
        float                       m_afFrustumMax[3];          // axis aligned frustum box (max, world-space)
        idv::Vec3                   m_acFrustumPoints[5];       // stores five points that define a frustum (one point for camera instead of near plane)

        // billboard batching
        st_map_cells                m_mBBs;                     // used to distribute billboard rendering buffers across visible cells
        st_list_vertbuf             m_sBufferList;              // list of rendering buffers available for use to render billboards
        int                         m_nMaxTreesInOneCell;       // of all cells in the region, this is the largest
        unsigned char*              m_pStagingBuffer;           // buffer used to compile billboard lists to upload to render buffers

        // overlay
        CIdvInterleavedBuffer       m_cOverlayVB;
};


