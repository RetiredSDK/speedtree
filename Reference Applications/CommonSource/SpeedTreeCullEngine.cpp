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

#include "SpeedTreeCullEngine.h"
#include "SpeedTreeBillboard.h"
#include "UpVector.h"
#include "IdvShaderManager.h"
#include <math.h>
using namespace std;

// constants
#define NO_INSTANCES_FOR_BASE -1
const int c_nMaxOverlayBufferVerts = 200000;


// Rendering billboards in cells that have a mix of 3d trees and billboards (LOD_MIX) is expensive.  
// Each billboard must be fed into a dynamic buffer that's built per-frame, as opposed to the
// all-billboard cells that are static while the cell remains visible.  When rendering
// the individual billboards that are part of the LOD_MIX cells, you can choose between
// culling each of them individually, or drawing them all via brute force, depending on 
// whether your application has more CPU or GPU to spare.
//
// To see the effect of this flag, run the reference application on a dense forest with 
// the flag on and then the flag off.

#define CULL_INDIVIDUAL_BILLBOARDS


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::SCell::SCell

CSpeedTreeCullEngine::SCell::SCell( ) :
    m_nRow(-1),
    m_nCol(-1),
    m_fCullRadius(FLT_MAX),
    m_eCullStatus(CULLED_COMPLETE),
    m_eLodStatus(LOD_MIX),
    m_nNumBBs(0),
    m_pBillboardVB(NULL)
{
    // fill with obviously invalid values to tag as uninitialized
    m_afExtents[0] = m_afExtents[1] = m_afExtents[2] = FLT_MAX;
    m_afExtents[3] = m_afExtents[4] = m_afExtents[5] = -FLT_MAX;
    m_afRoughExtents[0] = m_afRoughExtents[1] = m_afRoughExtents[2] = FLT_MAX;
    m_afRoughExtents[3] = m_afRoughExtents[4] = m_afRoughExtents[5] = -FLT_MAX;
    m_afCenter[0] = m_afCenter[1] = m_afCenter[2] = FLT_MAX;
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::CSpeedTreeCullEngine

CSpeedTreeCullEngine::CSpeedTreeCullEngine( ) :
    m_pCells(NULL),
    m_nNumCellRows(-1),
    m_nNumCellCols(-1),
    m_nNumCells(-1),
    m_nNumBaseTrees(-1),
    m_nCurrentBaseIndex(-1),
    m_nCurrentTree(0),
    m_nCurrentCell(0),
    m_fLongestTreeExtent(0.0f),
    m_fAspectRatio(4.0f / 3.0f),
    m_fFieldOfView(40.0f),
    m_fNearPlane(1.0f),
    m_fFarPlane(1000.0f),
    m_fNearLod(300.0f),
    m_fFarLod(1000.0f),
    m_nMaxTreesInOneCell(0),
    m_pStagingBuffer(NULL)
{
    m_iCurrentBillboardCell = m_mBBs.end( );

    m_afForestExtents[0] = m_afForestExtents[1] = m_afForestExtents[2] = FLT_MAX;
    m_afForestExtents[3] = m_afForestExtents[4] = m_afForestExtents[5] = -FLT_MAX;
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::~CSpeedTreeCullEngine

CSpeedTreeCullEngine::~CSpeedTreeCullEngine( )
{
    DeleteCells( );
    st_delete_array<unsigned char>(m_pStagingBuffer, "CSpeedTreeCullEngine::~CSpeedTreeCullEngine, m_pStagingBuffer");

    // delete map of render buffers
    for (st_map_cells::iterator iMap = m_mBBs.begin( ); iMap != m_mBBs.end( ); ++iMap)
    {
        CIdvInterleavedBuffer* pBuffer = iMap->first;
        st_delete<CIdvInterleavedBuffer>(pBuffer, "CSpeedTreeCullEngine::~CSpeedTreeCullEngine, map render buffer");
    }

    // delete any render buffers on the stack
    int nStackSize = int(m_sBufferList.size( ));
    for (int i = 0; i < nStackSize; ++i)
    {
        st_delete<CIdvInterleavedBuffer>(m_sBufferList.front( ), "CSpeedTreeCullEngine::~CSpeedTreeCullEngine, stack render buffer");
        m_sBufferList.pop_front( );
    }
}


///////////////////////////////////////////////////////////////////////  
//  CSortByBase

class CSortByBase
{
public:
    bool operator( )(const CSpeedTreeInstance* pLeft, const CSpeedTreeInstance* pRight)
    {
        return (pLeft->InstanceOf( )->SortIndex( ) < pRight->InstanceOf( )->SortIndex( ));
    }
};


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::Cell

inline CSpeedTreeCullEngine::SCell* CSpeedTreeCullEngine::Cell(int nRow, int nCol) const
{ 
    int nCellIndex = nRow * m_nNumCellCols + nCol;
    assert(nCellIndex < m_nNumCells);
    assert(m_pCells); 

    return m_pCells + nCellIndex;
}


///////////////////////////////////////////////////////////////////////  
//  Function: st_sign

inline float st_sign(float fValue)
{
    return fValue < 0.0f ? -1.0f : 1.0f;
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::Init

bool CSpeedTreeCullEngine::Init(CSpeedTreeInstance* pTrees, int nNumTrees, int nNumBaseTrees, int nNumCellRows, int nNumCellCols, const idv::Region& cExtents)
{
    bool bSuccess = false;

    // make sure we're working with reasonable values
    if (nNumCellRows > 1 && 
        nNumCellCols > 1 &&
        nNumTrees > 0 && 
        nNumBaseTrees > 0 &&
        pTrees != NULL)
    {
        m_nNumBaseTrees = nNumBaseTrees;

        // allocate cell grid
        m_nNumCellRows = nNumCellRows;
        m_nNumCellCols = nNumCellCols;
        m_nNumCells = m_nNumCellRows * m_nNumCellCols;
        m_pCells = st_new_array<SCell>(m_nNumCells, "CSpeedTreeCullEngine::Init, m_pCells");

        // compute extents of whole forest
        if (cExtents.IsSet( ))
        {
            for (int nAxis = 0; nAxis < 3; ++nAxis)
            {
                m_afForestExtents[nAxis] = cExtents.m_cMin[nAxis];
                m_afForestExtents[nAxis + 3] = cExtents.m_cMax[nAxis];
            }
        }
        else
            ComputeExtents(pTrees, nNumTrees);

        // cell dimensions
        const float c_fCellWidth = (m_afForestExtents[3] - m_afForestExtents[0]) / m_nNumCellCols;
#ifdef UPVECTOR_POS_Y
        const float c_fCellHeight = (m_afForestExtents[5] - m_afForestExtents[2]) / m_nNumCellRows;
#else
        const float c_fCellHeight = (m_afForestExtents[4] - m_afForestExtents[1]) / m_nNumCellRows;
#endif

        // define the cells based on the forest extents
#ifdef UPVECTOR_POS_Y
        float fY = m_afForestExtents[2];
#else
        float fY = m_afForestExtents[1];
#endif
        for (int nRow = 0; nRow < m_nNumCellRows; ++nRow)
        {
            float fX = m_afForestExtents[0];
            for (int nCol = 0; nCol < m_nNumCellCols; ++nCol)
            {
                SCell* pCell = Cell(nRow, nCol);

                // cell's row/col coords
                pCell->m_nRow = nRow;
                pCell->m_nCol = nCol;

                // cell's 2D extents
                pCell->m_afRoughExtents[0] = pCell->m_afExtents[0] = fX;
                pCell->m_afRoughExtents[3] = pCell->m_afExtents[3] = fX + c_fCellWidth;
#ifdef UPVECTOR_POS_Y
                pCell->m_afRoughExtents[2] = pCell->m_afExtents[2] = fY;
                pCell->m_afRoughExtents[5] = pCell->m_afExtents[5] = fY + c_fCellHeight;
#else
                pCell->m_afRoughExtents[1] = pCell->m_afExtents[1] = fY;
                pCell->m_afRoughExtents[4] = pCell->m_afExtents[4] = fY + c_fCellHeight;
#endif

                fX += c_fCellWidth;
            }

            fY += c_fCellHeight;
        }

        // populate the cells with trees
        for (int nTree = 0; nTree < nNumTrees; ++nTree)
        {
            CSpeedTreeInstance* pTree = pTrees + nTree;

            // put the tree in the appropriate cell
            SCell* pCell = PosToCell(pTree->Pos( ));
            pCell->m_vTrees.push_back(pTree);

            // adjust the cell's xy extents to adjust for potential overhang
            float afTreeExtents[6];
            pTree->InstanceOf( )->GetBoundingBox(afTreeExtents);

            // scale extents by instance's scale
            for (int i = 0; i < 6; ++i)
                afTreeExtents[i] *= pTree->Scale( );

            // because the instances are rotated, a tree that hangs far over to one side, rotated 180
            // degrees could cause culling errors.  to combat this, the extents are expanded even further
            float fLargest = st_max(fabs(afTreeExtents[0]), fabs(afTreeExtents[3]));
            afTreeExtents[0] = st_sign(afTreeExtents[0]) * fLargest;
            afTreeExtents[3] = st_sign(afTreeExtents[3]) * fLargest;
#ifdef UPVECTOR_POS_Y
            const int c_nAxis = 2;
#else
            const int c_nAxis = 1;
#endif
            fLargest = st_max(fabs(afTreeExtents[c_nAxis]), fabs(afTreeExtents[c_nAxis + 3]));
            afTreeExtents[c_nAxis] = st_sign(afTreeExtents[c_nAxis]) * fLargest;
            afTreeExtents[c_nAxis + 3] = st_sign(afTreeExtents[c_nAxis + 3]) * fLargest;

            // adjust the cell's extents based on the largest possible tree extents
            pCell->m_afExtents[0] = st_min(pCell->m_afExtents[0], pTree->Pos( )[0] + afTreeExtents[0]);
            pCell->m_afExtents[1] = st_min(pCell->m_afExtents[1], pTree->Pos( )[1] + afTreeExtents[1]);
            pCell->m_afExtents[2] = st_min(pCell->m_afExtents[2], pTree->Pos( )[2] + afTreeExtents[2]);
            pCell->m_afExtents[3] = st_max(pCell->m_afExtents[3], pTree->Pos( )[0] + afTreeExtents[3]);
            pCell->m_afExtents[4] = st_max(pCell->m_afExtents[4], pTree->Pos( )[1] + afTreeExtents[4]);
            pCell->m_afExtents[5] = st_max(pCell->m_afExtents[5], pTree->Pos( )[2] + afTreeExtents[5]);

            // we need to know the longest possible overhang for any tree in the scene.  this means
            // finding:
            //      1) the extents of the base model of the instance
            //      2) adjusting for the instance's scale
            //      3) adjusting for the instances's rotation (picture a root going all the way to 
            //         a corner of the extents box, but then getting rotated 45 degrees, pushing it
            //         out of the ortho box.

            m_fLongestTreeExtent = 0.0f;
#ifdef UPVECTOR_POS_Y
            // corner 0
            float fCornerLength = afTreeExtents[0] * afTreeExtents[0] + afTreeExtents[2] * afTreeExtents[2];
            m_fLongestTreeExtent = st_max(fCornerLength, m_fLongestTreeExtent);
            // corner 1
            fCornerLength = afTreeExtents[3] * afTreeExtents[3] + afTreeExtents[2] * afTreeExtents[2];
            m_fLongestTreeExtent = st_max(fCornerLength, m_fLongestTreeExtent);
            // corner 2
            fCornerLength = afTreeExtents[3] * afTreeExtents[3] + afTreeExtents[5] * afTreeExtents[5];
            m_fLongestTreeExtent = st_max(fCornerLength, m_fLongestTreeExtent);
            // corner 3
            fCornerLength = afTreeExtents[0] * afTreeExtents[0] + afTreeExtents[5] * afTreeExtents[5];
            m_fLongestTreeExtent = st_max(fCornerLength, m_fLongestTreeExtent);
#else
            // corner 0
            float fCornerLength = afTreeExtents[0] * afTreeExtents[0] + afTreeExtents[1] * afTreeExtents[1];
            m_fLongestTreeExtent = st_max(fCornerLength, m_fLongestTreeExtent);
            // corner 1
            fCornerLength = afTreeExtents[3] * afTreeExtents[3] + afTreeExtents[1] * afTreeExtents[1];
            m_fLongestTreeExtent = st_max(fCornerLength, m_fLongestTreeExtent);
            // corner 2
            fCornerLength = afTreeExtents[3] * afTreeExtents[3] + afTreeExtents[4] * afTreeExtents[4];
            m_fLongestTreeExtent = st_max(fCornerLength, m_fLongestTreeExtent);
            // corner 3
            fCornerLength = afTreeExtents[0] * afTreeExtents[0] + afTreeExtents[4] * afTreeExtents[4];
            m_fLongestTreeExtent = st_max(fCornerLength, m_fLongestTreeExtent);
#endif
            m_fLongestTreeExtent = sqrt(m_fLongestTreeExtent);
        }

        // with the cells now populated with trees, fill out the rest of the data
        m_nMaxTreesInOneCell = 0;
        for (int nCell = 0; nCell < m_nNumCells; ++nCell)
        {
            SCell* pCell = m_pCells + nCell;

            // init m_vBaseIndices vector
            pCell->m_vBaseIndices.resize(m_nNumBaseTrees);
            for (int nBase = 0; nBase < m_nNumBaseTrees; ++nBase)
                pCell->m_vBaseIndices[nBase] = NO_INSTANCES_FOR_BASE;

            // sort trees by base type to reduce context switches later
            sort(pCell->m_vTrees.begin( ), pCell->m_vTrees.end( ), CSortByBase( ));
            int nCurrentIndex = NO_INSTANCES_FOR_BASE;
            int nNumTrees = int(pCell->m_vTrees.size( ));
            for (int nTree = 0; nTree < nNumTrees; ++nTree)
            {
                int nThisIndex = pCell->m_vTrees[nTree]->InstanceOf( )->SortIndex( );
                if (nCurrentIndex != nThisIndex)
                {
                    st_assert(nThisIndex > NO_INSTANCES_FOR_BASE && nThisIndex < m_nNumBaseTrees);
                    pCell->m_vBaseIndices[nThisIndex] = nTree;
                    nCurrentIndex = nThisIndex;
                }
            }
        
            // center
#ifdef UPVECTOR_POS_Y
            pCell->m_afCenter[0] = 0.5f * (pCell->m_afExtents[0] + pCell->m_afExtents[3]);
            pCell->m_afCenter[2] = 0.5f * (pCell->m_afExtents[2] + pCell->m_afExtents[5]);
            if (pCell->m_vTrees.empty( ))
            {
                pCell->m_afExtents[1] = -1.0f;
                pCell->m_afExtents[4] = 100.0f; 
            }
            pCell->m_afCenter[1] = 0.5f * (pCell->m_afExtents[1] + pCell->m_afExtents[4]);
#else
            pCell->m_afCenter[0] = 0.5f * (pCell->m_afExtents[0] + pCell->m_afExtents[3]);
            pCell->m_afCenter[1] = 0.5f * (pCell->m_afExtents[1] + pCell->m_afExtents[4]);
            if (pCell->m_vTrees.empty( ))
            {
                pCell->m_afExtents[2] = -1.0f;
                pCell->m_afExtents[5] = 100.0f; 
            }
            pCell->m_afCenter[2] = 0.5f * (pCell->m_afExtents[2] + pCell->m_afExtents[5]);
#endif

            // cell's culling radius (a little oversized to compensate for tree overhang)
            pCell->m_fCullRadius = 1.1f * idv::Vec3(pCell->m_afCenter).Distance(idv::Vec3(pCell->m_afExtents + 3));

            // adjust the largest billboard buffer
            m_nMaxTreesInOneCell = st_max(m_nMaxTreesInOneCell, int(pCell->m_vTrees.size( )));
        }

        // allocate the billboard staging buffer to be used to efficiently build and copy
        // all billboard data for a given cell
#ifdef HORZ_BILLBOARDS
        m_pStagingBuffer = st_new_array<unsigned char>(2 * m_nMaxTreesInOneCell * SSpeedTreeBillboard::Size( ), "CSpeedTreeCullEngine::Init, m_pStagingBuffer");
#else
        m_pStagingBuffer = st_new_array<unsigned char>(m_nMaxTreesInOneCell * SSpeedTreeBillboard::Size( ), "CSpeedTreeCullEngine::Init, m_pStagingBuffer");
#endif

        bSuccess = true;
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::CullStatus

inline CSpeedTreeCullEngine::ECullStatus CSpeedTreeCullEngine::CullStatus(SCell* pCell) const
{
    for (int nPlane = 0; nPlane < 6; ++nPlane)
    {
        float fDistance = m_afFrustumPlanes[nPlane][0] * pCell->m_afCenter[0] + 
                          m_afFrustumPlanes[nPlane][1] * pCell->m_afCenter[1] + 
                          m_afFrustumPlanes[nPlane][2] * pCell->m_afCenter[2] + 
                          m_afFrustumPlanes[nPlane][3];
        if (fDistance < -pCell->m_fCullRadius)
            return CULLED_COMPLETE;
    }

    return CULLED_NONE;
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::CullStatus

inline bool CSpeedTreeCullEngine::CullStatus(const CSpeedTreeInstance* pTree) const
{
    const float* pPos = pTree->Pos( );
    const float fUpAxisCenter = pTree->UpAxisCenter( );
    const float fRadius = pTree->InstanceOf( )->CullRadius( ) * pTree->Scale( );

#ifdef UPVECTOR_POS_Y
    for (int nPlane = 0; nPlane < 6; ++nPlane)
    {
        float fDistance = m_afFrustumPlanes[nPlane][0] * pPos[0] + 
                          m_afFrustumPlanes[nPlane][1] * fUpAxisCenter + 
                          m_afFrustumPlanes[nPlane][2] * pPos[2] + 
                          m_afFrustumPlanes[nPlane][3];
        if (fDistance < -fRadius / 1.0f)
            return true;
    }
#else
    for (int nPlane = 0; nPlane < 6; ++nPlane)
        if (m_afFrustumPlanes[nPlane][0] * pPos[0] + m_afFrustumPlanes[nPlane][1] * pPos[1] + m_afFrustumPlanes[nPlane][2] * fUpAxisCenter + m_afFrustumPlanes[nPlane][3] < -fRadius)
            return true;
#endif

    return false;
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::SetProjection

void CSpeedTreeCullEngine::SetProjection(float fAspectRatio, float fFieldOfView, float fNearClip, float fFarClip)
{
    m_fAspectRatio = fAspectRatio;
    m_fFieldOfView = fFieldOfView; // degrees
    m_fNearPlane = fNearClip;
    m_fFarPlane = fFarClip;
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::SetCamera

void CSpeedTreeCullEngine::SetCamera(const float* pCameraPos, const float* pModelviewMatrix, const float* pProjection)
{
    memcpy(m_afCameraPos, pCameraPos, 3 * sizeof(float));

    // "right" vector
    m_afCameraRight[0] = pModelviewMatrix[0];
    m_afCameraRight[1] = pModelviewMatrix[4];
    m_afCameraRight[2] = pModelviewMatrix[8];

    // "up" vector
    m_afCameraUp[0] = pModelviewMatrix[1];
    m_afCameraUp[1] = pModelviewMatrix[5];
    m_afCameraUp[2] = pModelviewMatrix[9];

    // "out of screen" vector
    m_afCameraOut[0] = pModelviewMatrix[2];
    m_afCameraOut[1] = pModelviewMatrix[6];
    m_afCameraOut[2] = pModelviewMatrix[10];

    // compute new frustum box
    ComputeFrustum(pProjection, pModelviewMatrix);
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::Cull

void CSpeedTreeCullEngine::Cull(void)
{
    st_assert(m_pCells);

    // set all regions to culled, modify later
    for (int i = 0; i < m_nNumCells; ++i)
        m_pCells[i].m_eCullStatus = CULLED_COMPLETE;
    m_vVisibleCells.resize(0);

    // convert from world-space frustum extents to grid cells holding those values
    const SCell* pMinCell = PosToCell(m_afFrustumMin);
    const SCell* pMaxCell = PosToCell(m_afFrustumMax);

    // run through axis-aligned viewport bounding box, cell by cell
    for (int nRow = pMinCell->m_nRow; nRow <= pMaxCell->m_nRow; ++nRow)
    {
        assert(nRow < m_nNumCellRows);
        for (int nCol = pMinCell->m_nCol; nCol <= pMaxCell->m_nCol; ++nCol)
        {
            assert(nCol < m_nNumCellCols);
            SCell* pCell = Cell(nRow, nCol);

            // for a given cell, check to see if it's culled, fully visible, or partially visible
            pCell->m_eCullStatus = CullStatus(pCell);

            // is the cell visible at all?
            if (pCell->m_eCullStatus != CULLED_COMPLETE)
            {
                // determine if the cell is all billboards, or a mixture of LOD types
                ComputeCellLod(pCell);
                
                // if all billboards, not much to do
                if (pCell->m_eLodStatus == LOD_MIX)
                {
                    // figure each tree's visibility
                    if (!pCell->m_vTrees.empty( ))
                    {
                        CSpeedTreeInstance** pTree = &(pCell->m_vTrees[0]);

                        int nNumTrees = int(pCell->m_vTrees.size( ));
                        for (int nTree = 0; nTree < nNumTrees; ++nTree)
                        {
                            (*pTree)->SetCulled(CullStatus(*pTree));
                            ++pTree;
                        }
                    }

                    // compute each tree's LOD value (since there's a mixture)
                    ComputeTreesLod(pCell);
                }

                m_vVisibleCells.push_back(pCell);
            }
        }
    }

    // billboards
    ProcessBillboards( );
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::First3dTree

CSpeedTreeInstance* CSpeedTreeCullEngine::First3dTree(int nBaseIndex)
{
    m_nCurrentBaseIndex = nBaseIndex;
    m_nCurrentCell = -1;

    NextCell( );

    return Next3dTree( );
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::Next3dTree
//
//  Designed to abstract the calling application from needing to know which
//  tree is in which cell, the tree's cull status, and it's LOD value.
//  Calling Next3dTree() in succession should yield a list of trees that
//  are visible and whose LOD value is greater than 0.0.
//
//  In order to maintain a well-performing order, the trees are returned
//  in sorted order (sorted by their base tree types since switching base
//  trees during rendering results in texture binds and other state changes).
//  Next3dTree() will continue to return values until the last tree is
//  reached that has the same base index as was passed into First3dTree().

CSpeedTreeInstance* CSpeedTreeCullEngine::Next3dTree(void)
{
    CSpeedTreeInstance* pNextTree = NULL;

    // how many visible cells are there?
    int nNumCells = int(m_vVisibleCells.size( ));

    // make sure we haven't exhausted available cells
    while (m_nCurrentCell < nNumCells)
    {
        // retrieve the current cell
        const SCell* pCell = m_vVisibleCells[m_nCurrentCell];

        // if this cell is all billboards, we should move to the next cell
        if (pCell->m_eLodStatus == LOD_ALL_BILLBOARDS)
        {
            NextCell( );
        }
        else
        {
            // how many trees are there total in this cell (visible or otherwise)?
            int nNumTrees = int(pCell->m_vTrees.size( ));

            // make sure the current index doesn't exceed the number of trees in the current cell
            while (m_nCurrentTree < nNumTrees)
            {
                // extract the next tree in the list
                CSpeedTreeInstance* pNextTree = pCell->m_vTrees[m_nCurrentTree];

                // make sure that we haven't moved over to the next set of base trees
                if (pNextTree->InstanceOf( )->SortIndex( ) == m_nCurrentBaseIndex)
                {
                    // advance to get the next tree on a subsequent Next3dTree call
                    ++m_nCurrentTree;

                    // there are some reasons we'd skip over the tree at this point:
                    //      > the tree has been culled
                    //      > the tree is at LOD level 0.0 (billboard)
                    if (!pNextTree->IsCulled( ) && !pNextTree->OnlyBillboardIsActive( ))
                        return pNextTree;
                }
                else
                {
                    // move to the next cell
                    break;
                }
            }

            NextCell( );
        }
    }

    return pNextTree;
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::FirstBillboardCell

const CIdvInterleavedBuffer* CSpeedTreeCullEngine::FirstBillboardCell(int& nNumBBs)
{
    m_iCurrentBillboardCell = m_mBBs.begin( );

    return NextBillboardCell(nNumBBs);
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::NextBillboardCell

const CIdvInterleavedBuffer* CSpeedTreeCullEngine::NextBillboardCell(int& nNumBBs)
{
    const CIdvInterleavedBuffer* pBuffer = NULL;

    if (m_iCurrentBillboardCell != m_mBBs.end( ))
    {
        pBuffer = m_iCurrentBillboardCell->first;
        nNumBBs = m_iCurrentBillboardCell->second->m_nNumBBs;

        ++m_iCurrentBillboardCell;
    }

    return pBuffer;
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::DrawOverlay

void CSpeedTreeCullEngine::DrawOverlay(CIdvEffect* pEffect, CIdvConstant* pColor)
{
    const float c_fPointSize = 2.0f;

#ifdef IDV_OPENGL
#ifndef PS3
    glPushAttrib(GL_ENABLE_BIT | GL_POINT_BIT);
#endif // PS3
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPointSize(c_fPointSize);
    glDisable(GL_TEXTURE_2D);
#endif // IDV_OPENGL

#if IDV_DIRECTX9 | _XBOX
    CIdvEffect::GetDevice( )->SetRenderState(D3DRS_ALPHATESTENABLE, false);
    CIdvEffect::GetDevice( )->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
    CIdvEffect::GetDevice( )->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
    CIdvEffect::GetDevice( )->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
    CIdvEffect::GetDevice( )->SetRenderState(D3DRS_ZENABLE, false);
    CIdvEffect::GetDevice( )->SetRenderState(D3DRS_POINTSIZE, *((DWORD*) &c_fPointSize));
#endif // IDV_DIRECTX9 | _XBOX

    // initialize vertex buffers if needed
    if (m_cOverlayVB.NumVertices( ) == 0)
    {
        m_cOverlayVB.SetStride(sizeof(idv::Vec3));
        m_cOverlayVB.SetVertexOffset(3, 0);
        m_cOverlayVB.SetMaxNumVertices(c_nMaxOverlayBufferVerts);
    }

    Draw2dBackground(pEffect, pColor);
    Draw2dFlatGrid(pEffect, pColor);
    Draw2dFrustum(pEffect, pColor);
    Draw2dActiveCells(pEffect, pColor);
    Draw2dBillboardTrees(pEffect, pColor);
    Draw2dMeshTrees(pEffect, pColor);

    // restore render states
#ifdef IDV_OPENGL
#ifndef PS3
    glPopAttrib( );
#else
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);
    glPointSize(1.0f);
#endif // PS3
#endif // IDV_OPENGL

#if IDV_DIRECTX9 | _XBOX
    CIdvEffect::GetDevice( )->SetRenderState(D3DRS_ALPHATESTENABLE, true);
    CIdvEffect::GetDevice( )->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
    CIdvEffect::GetDevice( )->SetRenderState(D3DRS_ZENABLE, true);
#endif
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::PosToCell

CSpeedTreeCullEngine::SCell* CSpeedTreeCullEngine::PosToCell(const float* pPos) const
{
    st_assert(pPos);
    st_assert(m_pCells);

#ifdef UPVECTOR_POS_Y
    float fWidthPercent = (pPos[0] - m_afForestExtents[0]) / (m_afForestExtents[3] - m_afForestExtents[0]);
    float fHeightPercent = (pPos[2] - m_afForestExtents[2]) / (m_afForestExtents[5] - m_afForestExtents[2]);
#else
    float fWidthPercent = (pPos[0] - m_afForestExtents[0]) / (m_afForestExtents[3] - m_afForestExtents[0]);
    float fHeightPercent = (pPos[1] - m_afForestExtents[1]) / (m_afForestExtents[4] - m_afForestExtents[1]);
#endif

    // clamp to [0.0, 1.0] range
    fWidthPercent = st_max(fWidthPercent, 0.0f);
    fHeightPercent = st_max(fHeightPercent, 0.0f);
    fWidthPercent = st_min(fWidthPercent, 1.0f);
    fHeightPercent = st_min(fHeightPercent, 1.0f);

    int nCol = fWidthPercent >= 1.0f ? m_nNumCellCols - 1 : int(fWidthPercent * m_nNumCellCols);
    st_assert(nCol >= 0 && nCol <= m_nNumCellCols);
    int nRow = fHeightPercent >= 1.0f ? m_nNumCellRows - 1 : int(fHeightPercent * m_nNumCellRows);
    st_assert(nRow >= 0 && nRow <= m_nNumCellRows);

    return Cell(nRow, nCol);
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::DeleteCells

void CSpeedTreeCullEngine::DeleteCells(void)
{
    if (m_nNumCells > -1)
    {
        st_delete_array<SCell>(m_pCells, "CSpeedTreeCullEngine::~CSpeedTreeCullEngine, m_pCells");
        m_nNumCellRows = -1;
        m_nNumCellCols = -1;
        m_nNumCells = -1;
    }
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::ComputeExtents
//
//  afExtents[0-2] = min x, min y, min z
//  afExtents[3-5] = max x, max y, max z

void CSpeedTreeCullEngine::ComputeExtents(const CSpeedTreeInstance* pTrees, int nNumTrees)
{
    if (pTrees && nNumTrees > 0)
    {
        for (int nTree = 0; nTree < nNumTrees; ++nTree)
        {
            const CSpeedTreeInstance* pTree = pTrees + nTree;
            
            // query tree's extents
            float afTreeExtents[6];
            pTree->InstanceOf( )->GetBoundingBox(afTreeExtents);

            for (int nAxis = 0; nAxis < 3; ++nAxis)
            {
                float fMinPoint = pTree->Pos( )[nAxis] + afTreeExtents[nAxis + 0];
                float fMaxPoint = pTree->Pos( )[nAxis] + afTreeExtents[nAxis + 3];

                m_afForestExtents[nAxis + 0] = st_min(m_afForestExtents[nAxis + 0], fMinPoint);
                m_afForestExtents[nAxis + 3] = st_max(m_afForestExtents[nAxis + 3], fMaxPoint);
            }
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::ComputeFrustum

void CSpeedTreeCullEngine::ComputeFrustum(const float* pProjection, const float* pModelview)
{
    enum
    {
        NEARPLANE, FARPLANE, RIGHTPLANE, LEFTPLANE, BOTPLANE, TOPPLANE
    };

    // setup useful vectors
    idv::Vec3 cCameraIn(-m_afCameraOut[0], -m_afCameraOut[1], -m_afCameraOut[2]);
    idv::Vec3 cCameraUp(m_afCameraUp[0], m_afCameraUp[1], m_afCameraUp[2]);
    idv::Vec3 cCameraRight(m_afCameraRight[0], m_afCameraRight[1], m_afCameraRight[2]);
    idv::Vec3 cCameraPos(m_afCameraPos[0], m_afCameraPos[1], m_afCameraPos[2]);
    idv::Vec3 cFarPoint = cCameraPos + cCameraIn * m_fFarPlane;

    // frustum points
    float fFrustumHeight = m_fFarPlane * tanf(0.5f * idv::DegToRad(m_fFieldOfView));
    float fFrustumWidth = m_fFarPlane * tanf(0.5f * idv::DegToRad(m_fFieldOfView) * m_fAspectRatio);

    //idv::Vec3 acFrustum[5];
    m_acFrustumPoints[0] = cCameraPos;
    m_acFrustumPoints[1] = cFarPoint + cCameraRight * fFrustumWidth + cCameraUp * fFrustumHeight;
    m_acFrustumPoints[2] = cFarPoint - cCameraRight * fFrustumWidth + cCameraUp * fFrustumHeight;
    m_acFrustumPoints[3] = cFarPoint - cCameraRight * fFrustumWidth - cCameraUp * fFrustumHeight;
    m_acFrustumPoints[4] = cFarPoint + cCameraRight * fFrustumWidth - cCameraUp * fFrustumHeight;

    // find min/max (x,y) coordinates
    m_afFrustumMin[0] = m_afFrustumMin[1] = m_afFrustumMin[2] = FLT_MAX;
    m_afFrustumMax[0] = m_afFrustumMax[1] = m_afFrustumMax[2] =-FLT_MAX;
    for (int i = 0; i < 5; ++i)
    {
        m_afFrustumMin[0] = st_min(m_afFrustumMin[0], m_acFrustumPoints[i][0]);
        m_afFrustumMax[0] = st_max(m_afFrustumMax[0], m_acFrustumPoints[i][0]);
#ifdef UPVECTOR_POS_Y
        m_afFrustumMin[2] = st_min(m_afFrustumMin[2], m_acFrustumPoints[i][2]);
        m_afFrustumMax[2] = st_max(m_afFrustumMax[2], m_acFrustumPoints[i][2]);
#else
        m_afFrustumMin[1] = st_min(m_afFrustumMin[1], m_acFrustumPoints[i][1]);
        m_afFrustumMax[1] = st_max(m_afFrustumMax[1], m_acFrustumPoints[i][1]);
#endif
    }

    // grow the frustum points out by the largest possible tree extents
#ifdef UPVECTOR_POS_Y
    m_afFrustumMin[0] -= m_fLongestTreeExtent;
    m_afFrustumMin[2] -= m_fLongestTreeExtent;
    m_afFrustumMax[0] += m_fLongestTreeExtent;
    m_afFrustumMax[2] += m_fLongestTreeExtent;
#else
    m_afFrustumMin[0] -= m_fLongestTreeExtent;
    m_afFrustumMin[1] -= m_fLongestTreeExtent;
    m_afFrustumMax[0] += m_fLongestTreeExtent;
    m_afFrustumMax[1] += m_fLongestTreeExtent;
#endif

    // combine the two matrices (multiply projection by modelview)
    float afClip[16] =
    {
        pModelview[0] * pProjection[0] + pModelview[1] * pProjection[4] + pModelview[2] * pProjection[8] + pModelview[3] * pProjection[12],
        pModelview[0] * pProjection[1] + pModelview[1] * pProjection[5] + pModelview[2] * pProjection[9] + pModelview[3] * pProjection[13],
        pModelview[0] * pProjection[2] + pModelview[1] * pProjection[6] + pModelview[2] * pProjection[10] + pModelview[3] * pProjection[14],
        pModelview[0] * pProjection[3] + pModelview[1] * pProjection[7] + pModelview[2] * pProjection[11] + pModelview[3] * pProjection[15],

        pModelview[4] * pProjection[0] + pModelview[5] * pProjection[4] + pModelview[6] * pProjection[8] + pModelview[7] * pProjection[12],
        pModelview[4] * pProjection[1] + pModelview[5] * pProjection[5] + pModelview[6] * pProjection[9] + pModelview[7] * pProjection[13],
        pModelview[4] * pProjection[2] + pModelview[5] * pProjection[6] + pModelview[6] * pProjection[10] + pModelview[7] * pProjection[14],
        pModelview[4] * pProjection[3] + pModelview[5] * pProjection[7] + pModelview[6] * pProjection[11] + pModelview[7] * pProjection[15],

        pModelview[8] * pProjection[0] + pModelview[9] * pProjection[4] + pModelview[10] * pProjection[8] + pModelview[11] * pProjection[12],
        pModelview[8] * pProjection[1] + pModelview[9] * pProjection[5] + pModelview[10] * pProjection[9] + pModelview[11] * pProjection[13],
        pModelview[8] * pProjection[2] + pModelview[9] * pProjection[6] + pModelview[10] * pProjection[10] + pModelview[11] * pProjection[14],
        pModelview[8] * pProjection[3] + pModelview[9] * pProjection[7] + pModelview[10] * pProjection[11] + pModelview[11] * pProjection[15],

        pModelview[12] * pProjection[0] + pModelview[13] * pProjection[4] + pModelview[14] * pProjection[8] + pModelview[15] * pProjection[12],
        pModelview[12] * pProjection[1] + pModelview[13] * pProjection[5] + pModelview[14] * pProjection[9] + pModelview[15] * pProjection[13],
        pModelview[12] * pProjection[2] + pModelview[13] * pProjection[6] + pModelview[14] * pProjection[10] + pModelview[15] * pProjection[14],
        pModelview[12] * pProjection[3] + pModelview[13] * pProjection[7] + pModelview[14] * pProjection[11] + pModelview[15] * pProjection[15]
    };

    // extract the numbers for the RIGHT plane
    m_afFrustumPlanes[RIGHTPLANE][0] = afClip[3] - afClip[0];
    m_afFrustumPlanes[RIGHTPLANE][1] = afClip[7] - afClip[4];
    m_afFrustumPlanes[RIGHTPLANE][2] = afClip[11] - afClip[8];
    m_afFrustumPlanes[RIGHTPLANE][3] = afClip[15] - afClip[12];

    // normalize the result
    float t = sqrt( m_afFrustumPlanes[RIGHTPLANE][0] * m_afFrustumPlanes[RIGHTPLANE][0] + m_afFrustumPlanes[RIGHTPLANE][1] * m_afFrustumPlanes[RIGHTPLANE][1] + m_afFrustumPlanes[RIGHTPLANE][2] * m_afFrustumPlanes[RIGHTPLANE][2] );
    m_afFrustumPlanes[RIGHTPLANE][0] /= t;
    m_afFrustumPlanes[RIGHTPLANE][1] /= t;
    m_afFrustumPlanes[RIGHTPLANE][2] /= t;
    m_afFrustumPlanes[RIGHTPLANE][3] /= t;

    // extract the numbers for the LEFT plane
    m_afFrustumPlanes[LEFTPLANE][0] = afClip[3] + afClip[0];
    m_afFrustumPlanes[LEFTPLANE][1] = afClip[7] + afClip[4];
    m_afFrustumPlanes[LEFTPLANE][2] = afClip[11] + afClip[8];
    m_afFrustumPlanes[LEFTPLANE][3] = afClip[15] + afClip[12];

    // normalize the result
    t = sqrt( m_afFrustumPlanes[LEFTPLANE][0] * m_afFrustumPlanes[LEFTPLANE][0] + m_afFrustumPlanes[LEFTPLANE][1] * m_afFrustumPlanes[LEFTPLANE][1] + m_afFrustumPlanes[LEFTPLANE][2] * m_afFrustumPlanes[LEFTPLANE][2] );
    m_afFrustumPlanes[LEFTPLANE][0] /= t;
    m_afFrustumPlanes[LEFTPLANE][1] /= t;
    m_afFrustumPlanes[LEFTPLANE][2] /= t;
    m_afFrustumPlanes[LEFTPLANE][3] /= t;

    // extract the BOTTOM plane
    m_afFrustumPlanes[BOTPLANE][0] = afClip[3] + afClip[1];
    m_afFrustumPlanes[BOTPLANE][1] = afClip[7] + afClip[5];
    m_afFrustumPlanes[BOTPLANE][2] = afClip[11] + afClip[9];
    m_afFrustumPlanes[BOTPLANE][3] = afClip[15] + afClip[13];

    // normalize the result
    t = sqrt( m_afFrustumPlanes[BOTPLANE][0] * m_afFrustumPlanes[BOTPLANE][0] + m_afFrustumPlanes[BOTPLANE][1] * m_afFrustumPlanes[BOTPLANE][1] + m_afFrustumPlanes[BOTPLANE][2] * m_afFrustumPlanes[BOTPLANE][2] );
    m_afFrustumPlanes[BOTPLANE][0] /= t;
    m_afFrustumPlanes[BOTPLANE][1] /= t;
    m_afFrustumPlanes[BOTPLANE][2] /= t;
    m_afFrustumPlanes[BOTPLANE][3] /= t;

    // extract the TOP plane
    m_afFrustumPlanes[TOPPLANE][0] = afClip[3] - afClip[1];
    m_afFrustumPlanes[TOPPLANE][1] = afClip[7] - afClip[5];
    m_afFrustumPlanes[TOPPLANE][2] = afClip[11] - afClip[9];
    m_afFrustumPlanes[TOPPLANE][3] = afClip[15] - afClip[13];

    // Normalize the result
    t = sqrt( m_afFrustumPlanes[TOPPLANE][0] * m_afFrustumPlanes[TOPPLANE][0] + m_afFrustumPlanes[TOPPLANE][1] * m_afFrustumPlanes[TOPPLANE][1] + m_afFrustumPlanes[TOPPLANE][2] * m_afFrustumPlanes[TOPPLANE][2] );
    m_afFrustumPlanes[TOPPLANE][0] /= t;
    m_afFrustumPlanes[TOPPLANE][1] /= t;
    m_afFrustumPlanes[TOPPLANE][2] /= t;
    m_afFrustumPlanes[TOPPLANE][3] /= t;

    // extract the FAR plane
    m_afFrustumPlanes[FARPLANE][0] = afClip[3] - afClip[2];
    m_afFrustumPlanes[FARPLANE][1] = afClip[7] - afClip[6];
    m_afFrustumPlanes[FARPLANE][2] = afClip[11] - afClip[10];
    m_afFrustumPlanes[FARPLANE][3] = afClip[15] - afClip[14];

    // normalize the result
    t = sqrt( m_afFrustumPlanes[FARPLANE][0] * m_afFrustumPlanes[FARPLANE][0] + m_afFrustumPlanes[FARPLANE][1] * m_afFrustumPlanes[FARPLANE][1] + m_afFrustumPlanes[FARPLANE][2] * m_afFrustumPlanes[FARPLANE][2] );
    m_afFrustumPlanes[FARPLANE][0] /= t;
    m_afFrustumPlanes[FARPLANE][1] /= t;
    m_afFrustumPlanes[FARPLANE][2] /= t;
    m_afFrustumPlanes[FARPLANE][3] /= t;

    // extract the NEAR plane
    m_afFrustumPlanes[NEARPLANE][0] = afClip[3] + afClip[2];
    m_afFrustumPlanes[NEARPLANE][1] = afClip[7] + afClip[6];
    m_afFrustumPlanes[NEARPLANE][2] = afClip[11] + afClip[10];
    m_afFrustumPlanes[NEARPLANE][3] = afClip[15] + afClip[14];

    // normalize the result
    t = sqrt( m_afFrustumPlanes[NEARPLANE][0] * m_afFrustumPlanes[NEARPLANE][0] + m_afFrustumPlanes[NEARPLANE][1] * m_afFrustumPlanes[NEARPLANE][1] + m_afFrustumPlanes[NEARPLANE][2] * m_afFrustumPlanes[NEARPLANE][2] );
    m_afFrustumPlanes[NEARPLANE][0] /= t;
    m_afFrustumPlanes[NEARPLANE][1] /= t;
    m_afFrustumPlanes[NEARPLANE][2] /= t;
    m_afFrustumPlanes[NEARPLANE][3] /= t;
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::ComputeCellLod

void CSpeedTreeCullEngine::ComputeCellLod(SCell* pCell) const
{
    assert(pCell);

    // determine if region should be all billboards
    float fDistance = idv::Vec3(pCell->m_afCenter).Distance(idv::Vec3(m_afCameraPos));
    fDistance -= pCell->m_fCullRadius;

    // if far enough away, it's all billboards and no individual tree check is necessary
    if (fDistance >= m_fFarLod)
        pCell->m_eLodStatus = LOD_ALL_BILLBOARDS;
    else
        pCell->m_eLodStatus = LOD_MIX;
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::ComputeTreesLod

void CSpeedTreeCullEngine::ComputeTreesLod(SCell* pCell) const
{
    assert(pCell);
    assert(pCell->m_eCullStatus != CULLED_COMPLETE);

    if (pCell->m_eLodStatus == LOD_MIX)
    {
        int nNumTrees = int(pCell->m_vTrees.size( ));
        if (pCell->m_eCullStatus == CULLED_NONE)
        {
            for (int nTree = 0; nTree < nNumTrees; ++nTree)
            {
                CSpeedTreeInstance* pTree = pCell->m_vTrees[nTree];

                // compute tree LOD
                float fDistance = idv::Vec3(pTree->Pos( )).Distance(idv::Vec3(m_afCameraPos));

                // SpeedTreeRT assumes 1.0 = highest, 0.0 = lowest
                float fLod = 1.0f - (fDistance - m_fNearLod) / (m_fFarLod - m_fNearLod);
                fLod = st_max(fLod, 0.0f);
                fLod = st_min(fLod, 1.0f);
                pTree->UpdateLod(fLod);
            }
        }
        else // PARTIALLY_VISIBLE
        {
            for (int nTree = 0; nTree < nNumTrees; ++nTree)
            {
                CSpeedTreeInstance* pTree = pCell->m_vTrees[nTree];
                if (!pTree->IsCulled( ))
                {
                    // compute tree LOD
                    float fDistance = idv::Vec3(pTree->Pos( )).Distance(idv::Vec3(m_afCameraPos));

                    // SpeedTreeRT assumes 1.0 = highest, 0.0 = lowest
                    float fLod = 1.0f - (fDistance - m_fNearLod) / (m_fFarLod - m_fNearLod);
                    fLod = st_max(fLod, 0.0f);
                    fLod = st_min(fLod, 1.0f);
                    pTree->UpdateLod(fLod);
                }
            }
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::NextCell

void CSpeedTreeCullEngine::NextCell(void)
{
    // advance to the next cell     
    ++m_nCurrentCell;

    int nNumCells = int(m_vVisibleCells.size( ));
    while (m_nCurrentCell < nNumCells)
    {
        // set tree index to first tree of index m_nCurrentBaseIndex
        st_assert(m_nCurrentBaseIndex < int(m_vVisibleCells[m_nCurrentCell]->m_vBaseIndices.size( )));
        m_nCurrentTree = m_vVisibleCells[m_nCurrentCell]->m_vBaseIndices[m_nCurrentBaseIndex];

        // are there no trees of this base type to render?
        if (m_nCurrentTree == -1 || m_vVisibleCells[m_nCurrentCell]->m_eLodStatus != LOD_MIX)
        {
            // if not, advance the cell index and try again
            ++m_nCurrentCell;
        }
        else
            break;
    }
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::ProcessBillboards

void CSpeedTreeCullEngine::ProcessBillboards(void)
{
    // of the cells currently in the billboard map, find out which are no longer visible
    for (st_map_cells::iterator iMap = m_mBBs.begin( ); iMap != m_mBBs.end( ); ++iMap)
    {
        // get easier-to-use handles on map contents
        CIdvInterleavedBuffer* pBillboardVB = iMap->first;
        SCell* pCell = iMap->second;

        // if a region is culled out or isn't composed entirely of billboards, it
        // should be removed from this map and its render buffer made available
        if (pCell &&
            pCell->m_eCullStatus == CULLED_COMPLETE)
        {
            // this cell is being removed, so its billboard VB is no longer valid
            pCell->m_pBillboardVB = NULL;

            // make this VB available for use with another cell
            if (pBillboardVB)
                m_sBufferList.push_front(pBillboardVB);

            // remove this cell from the billboard map (a little tricky to delete since
            // we're in the middle of walking the map now)
            st_map_cells::iterator iErasePoint = iMap;
            ++iMap;
            (void) m_mBBs.erase(iErasePoint);

            if (iMap == m_mBBs.end( ))
                break;
        }
    }

    // run through the visible cells
    int nNumCells = int(m_vVisibleCells.size( ));
    for (int nCell = 0; nCell < nNumCells; ++nCell)
    {
        SCell* pCell = m_vVisibleCells[nCell];

        if (!pCell->m_vTrees.empty( ))
        {
            // if a cell is a mixture, then it's billboards are almost always in need of
            // updating (since they're not LOD_ALL_BILLBOARDS)
            if (pCell->m_eLodStatus == LOD_MIX)
            {
                // if a render buffer is already assigned, use it, otherwise get a new one
                CIdvInterleavedBuffer* pVB = pCell->m_pBillboardVB;
                if (!pVB)
                    pVB = NextRenderBuffer( );

                // fill the vb with bb data
                PopulateVertexBuffer_PartialBBs(pVB, pCell);

                // assign the active VB to the cell
                pCell->m_pBillboardVB = pVB;

                // store the association in the map
                m_mBBs[pVB] = pCell;

            }

            // looking for cells that i) has only billboards, ii) has at least one tree, and
            // iii) does not yet have a render buffer assigned to it - these are likely to be
            // regions that have just come into the back (or distant side) of the frustum.
            else if (pCell->m_eLodStatus == LOD_ALL_BILLBOARDS &&
                    !pCell->m_pBillboardVB)
            {
                // is a render buffer available off the stack or should a new one be made?
                CIdvInterleavedBuffer* pVB = NextRenderBuffer( );

                // fill the vb with bb data
                PopulateVertexBuffer_AllBBs(pVB, pCell);

                // assign the active VB to the cell
                pCell->m_pBillboardVB = pVB;

                // store the association in the map
                m_mBBs[pVB] = pCell;
            }
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::PopulateVertexBuffer_AllBBs

void CSpeedTreeCullEngine::PopulateVertexBuffer_AllBBs(CIdvInterleavedBuffer* pVB, SCell* pCell)
{
    st_assert(m_pStagingBuffer);

    // copy destination
    unsigned char* pDest = m_pStagingBuffer;

    // copy the mix of trees into buffer - the order is unimportant
    pCell->m_nNumBBs = int(pCell->m_vTrees.size( ));
    if (pCell->m_nNumBBs > 0)
    {
        // fill in normal billboards
        for (int nTree = 0; nTree < pCell->m_nNumBBs; ++nTree)
        {
            // get handles on the current instance and the base it represents
            const CSpeedTreeInstance* pTree = pCell->m_vTrees[nTree];
            CSpeedTreeWrapper* pBase = pTree->InstanceOf( );

            pBase->GetVertBillboardBuffer( ).UpdateVertical(pTree->Pos( ), pTree->Rotation( ), 1.0f, pTree->Scale( ));
            memcpy(pDest, &pBase->GetVertBillboardBuffer( ), SSpeedTreeBillboard::Size( ));
            pDest += SSpeedTreeBillboard::Size( );
        }

#ifdef HORZ_BILLBOARDS
        // fill in horizontal billboards
        for (int nTree = 0; nTree < pCell->m_nNumBBs; ++nTree)
        {
            // get handles on the current instance and the base it represents
            const CSpeedTreeInstance* pTree = pCell->m_vTrees[nTree];
            CSpeedTreeWrapper* pBase = pTree->InstanceOf( );
            const CSpeedTreeWrapper::SGeometry::SHorzBillboard& sHorzBB = pBase->GetHorzBillboard( );

            pBase->GetHorzBillboardBuffer( ).UpdateHorizontal(sHorzBB, pTree->Pos( ), pTree->Rotation( ), 1.0f, pTree->Scale( ));
            memcpy(pDest, &pBase->GetHorzBillboardBuffer( ), SSpeedTreeBillboard::Size( ));
            pDest += SSpeedTreeBillboard::Size( );
        }
        pVB->SetBuffer(m_pStagingBuffer, 2 * pCell->m_nNumBBs * SSpeedTreeBillboard::NumVerticesPerBillboard( ));
#else
        pVB->SetBuffer(m_pStagingBuffer, pCell->m_nNumBBs * SSpeedTreeBillboard::NumVerticesPerBillboard( ));
#endif
    }
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::PopulateVertexBuffer_PartialBBs

void CSpeedTreeCullEngine::PopulateVertexBuffer_PartialBBs(CIdvInterleavedBuffer* pVB, SCell* pCell)
{
    // copy destination
    unsigned char* pDest = m_pStagingBuffer;

    // copy the mix of trees into buffer - the order is unimportant
    int nNumTrees = int(pCell->m_vTrees.size( ));
    if (nNumTrees > 0)
    {
        // fill in normal billboards
        pCell->m_nNumBBs = 0;
        for (int nTree = 0; nTree < nNumTrees; ++nTree)
        {
            // get handles on the current instance and the base it represents
            CSpeedTreeInstance* pTree = pCell->m_vTrees[nTree];
            if (pTree->BillboardIsActive( ))
            {
#ifdef CULL_INDIVIDUAL_BILLBOARDS
                pTree->SetCulled(CullStatus(pTree));
                if (!pTree->IsCulled( ))
                {
#endif
                    CSpeedTreeWrapper* pBase = pTree->InstanceOf( );

                    pBase->GetVertBillboardBuffer( ).UpdateVertical(pTree->Pos( ), pTree->Rotation( ), pTree->GetBillboardFade( ), pTree->Scale( ));
                    memcpy(pDest, &pBase->GetVertBillboardBuffer( ), SSpeedTreeBillboard::Size( ));
                    pDest += SSpeedTreeBillboard::Size( );
                    ++pCell->m_nNumBBs;
#ifdef CULL_INDIVIDUAL_BILLBOARDS
                }
#endif
            }
        }

#ifdef HORZ_BILLBOARDS
        // fill in horizontal billboards
        for (int nTree = 0; nTree < nNumTrees; ++nTree)
        {
            // get handles on the current instance and the base it represents
            const CSpeedTreeInstance* pTree = pCell->m_vTrees[nTree];
            if (pTree->BillboardIsActive( ))
            {
                CSpeedTreeWrapper* pBase = pTree->InstanceOf( );
                const CSpeedTreeWrapper::SGeometry::SHorzBillboard& sHorzBB = pBase->GetHorzBillboard( );

                pBase->GetHorzBillboardBuffer( ).UpdateHorizontal(sHorzBB, pTree->Pos( ), pTree->Rotation( ), pTree->GetBillboardFade( ), pTree->Scale( ));
                memcpy(pDest, &pBase->GetHorzBillboardBuffer( ), SSpeedTreeBillboard::Size( ));
                pDest += SSpeedTreeBillboard::Size( );
            }
        }
        pVB->SetBuffer(m_pStagingBuffer, 2 * pCell->m_nNumBBs * SSpeedTreeBillboard::NumVerticesPerBillboard( ));
#else
        pVB->SetBuffer(m_pStagingBuffer, pCell->m_nNumBBs * SSpeedTreeBillboard::NumVerticesPerBillboard( ));
#endif
    }
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::NextRenderBuffer

CIdvInterleavedBuffer* CSpeedTreeCullEngine::NextRenderBuffer(void)
{
    CIdvInterleavedBuffer* pRenderBuffer = NULL;

    if (m_sBufferList.empty( ))
    {
        // stack is empty, so make a new one
        pRenderBuffer = new ("CSpeedTreeCullEngine::NextRenderBuffer, pRenderBuffer") CIdvInterleavedBuffer;
        pRenderBuffer->SetStride(sizeof(SSpeedTreeBillboard::SVertex));
        pRenderBuffer->SetVertexOffset(4, 0);
        pRenderBuffer->SetTexCoordOffset(0, 4, 4 * sizeof(float));
        pRenderBuffer->SetTexCoordOffset(1, 4, (4 + 4) * sizeof(float));
        pRenderBuffer->SetTexCoordOffset(2, 3, (4 + 4 + 4) * sizeof(float));
#ifdef HORZ_BILLBOARDS
        pRenderBuffer->SetMaxNumVertices(2 * m_nMaxTreesInOneCell * SSpeedTreeBillboard::NumVerticesPerBillboard( ));
#else
        pRenderBuffer->SetMaxNumVertices(m_nMaxTreesInOneCell * SSpeedTreeBillboard::NumVerticesPerBillboard( ));
#endif
    }
    else
    {
        // stack has at least one, so use the first available
        pRenderBuffer = m_sBufferList.front( );
        m_sBufferList.pop_front( );
    }
    st_assert(pRenderBuffer);

    return pRenderBuffer;
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::DrawActiveCells

void CSpeedTreeCullEngine::DrawActiveCells(CIdvEffect* pEffect, CIdvConstant* pColor)
{
    // setup rendering states (system dependent)
#if defined(IDV_OPENGL) && !defined(PS3)
    // setup render state for drawing blended lines w/o texturing
    glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT);
    glEnable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(2.0f);
#endif

#if IDV_DIRECTX9 | _XBOX
    CIdvEffect::GetDevice( )->SetRenderState(D3DRS_ALPHATESTENABLE, false);
    CIdvEffect::GetDevice( )->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
    CIdvEffect::GetDevice( )->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
    CIdvEffect::GetDevice( )->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
#endif

    // initialize vertex buffer if needed
    if (m_cOverlayVB.NumVertices( ) == 0)
    {
        m_cOverlayVB.SetStride(sizeof(idv::Vec3));
        m_cOverlayVB.SetVertexOffset(3, 0);
        m_cOverlayVB.SetMaxNumVertices(c_nMaxOverlayBufferVerts);
    }

    Draw3dCellsViaBuffer(pEffect, pColor);

    // restore states
#if defined(IDV_OPENGL) && !defined(PS3)
    glPopAttrib( );
#endif

#if IDV_DIRECTX9 | _XBOX
    CIdvEffect::GetDevice( )->SetRenderState(D3DRS_ALPHATESTENABLE, true);
    CIdvEffect::GetDevice( )->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
#endif
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::GetFrustumCenter

void CSpeedTreeCullEngine::GetFrustumCenter(float afCenter[3])
{
#ifdef UPVECTOR_POS_Y
    afCenter[0] = -0.5f * (m_afFrustumMax[0] + m_afFrustumMin[0]);
    afCenter[1] = 0.5f * (m_afFrustumMax[2] + m_afFrustumMin[2]);
#else
    afCenter[0] = 0.5f * (m_afFrustumMax[0] + m_afFrustumMin[0]);
    afCenter[1] = 0.5f * (m_afFrustumMax[1] + m_afFrustumMin[1]);
#endif
    afCenter[2] = 0.0f;
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::GetFrustumExtents

void CSpeedTreeCullEngine::GetFrustumExtents(float afMin[3], float afMax[3])
{
    memcpy(afMin, m_afFrustumMin, 3 * sizeof(float));
    memcpy(afMax, m_afFrustumMax, 3 * sizeof(float));
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::Draw3dCellsViaBuffer

void CSpeedTreeCullEngine::Draw3dCellsViaBuffer(CIdvEffect* pEffect, CIdvConstant* pColor)
{
    // set color
    (void) pColor->Set4f(1.0f, 1.0f, 0.0f, 1.0f);
#ifndef LOAD_BINARY_SHADERS
    (void) pEffect->CommitChanges( );
#endif

    // fill vertex buffer with cells
    vector<idv::Vec3> vPoints;
    int nNumCells = int(m_vVisibleCells.size( ));
    for (int nCell = 0; nCell < nNumCells; ++nCell)
    {
        SCell* pCell = m_vVisibleCells[nCell];
        if (!pCell->m_vTrees.empty( ))
        {
            // draw outline - a line_loop would be ideal, but not all platforms support it - we'll
            // use a line_strip where the first and last points are the same
#ifdef UPVECTOR_POS_Y
            vPoints.push_back(idv::Vec3(pCell->m_afExtents[0], pCell->m_afExtents[1], pCell->m_afExtents[2]));
            vPoints.push_back(idv::Vec3(pCell->m_afExtents[3], pCell->m_afExtents[1], pCell->m_afExtents[2]));
            vPoints.push_back(idv::Vec3(pCell->m_afExtents[3], pCell->m_afExtents[1], pCell->m_afExtents[5]));
            vPoints.push_back(idv::Vec3(pCell->m_afExtents[0], pCell->m_afExtents[1], pCell->m_afExtents[5]));
            vPoints.push_back(idv::Vec3(pCell->m_afExtents[0], pCell->m_afExtents[1], pCell->m_afExtents[2]));
#else
            vPoints.push_back(idv::Vec3(pCell->m_afExtents[0], pCell->m_afExtents[1], pCell->m_afExtents[2]));
            vPoints.push_back(idv::Vec3(pCell->m_afExtents[3], pCell->m_afExtents[1], pCell->m_afExtents[2]));
            vPoints.push_back(idv::Vec3(pCell->m_afExtents[3], pCell->m_afExtents[4], pCell->m_afExtents[2]));
            vPoints.push_back(idv::Vec3(pCell->m_afExtents[0], pCell->m_afExtents[4], pCell->m_afExtents[2]));
            vPoints.push_back(idv::Vec3(pCell->m_afExtents[0], pCell->m_afExtents[1], pCell->m_afExtents[2]));
#endif
        }
    }

    // make sure we don't overflow - if we do, cap it 
    int nNumPoints = CheckForOverflow(int(vPoints.size( )));

    // render the buffer
    if (!vPoints.empty( ))
    {
        m_cOverlayVB.SetBuffer((void*) &vPoints[0], nNumPoints);
        if (m_cOverlayVB.BindVertexBuffer(true))
        {
            int nNumCells = nNumPoints / 5;
            for (int i = 0; i < nNumCells; ++i)
                (void) m_cOverlayVB.DrawArrays(CIdvInterleavedBuffer::LINE_STRIP, i * 5, 5);

            (void) m_cOverlayVB.UnBindVertexBuffer(true);
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::Draw2dBackground

void CSpeedTreeCullEngine::Draw2dBackground(CIdvEffect* pEffect, CIdvConstant* pColor)
{
    // set color
    (void) pColor->Set4f(0.0f, 0.0f, 0.0f, 0.75f);
#ifndef LOAD_BINARY_SHADERS
    (void) pEffect->CommitChanges( );
#endif

    // fill vertex buffer with background square
    vector<idv::Vec3> vPoints;
#ifdef UPVECTOR_POS_Y
    vPoints.push_back(idv::Vec3(m_afForestExtents[3], m_afForestExtents[5]));
    vPoints.push_back(idv::Vec3(m_afForestExtents[0], m_afForestExtents[5]));
    vPoints.push_back(idv::Vec3(m_afForestExtents[0], m_afForestExtents[2]));
    vPoints.push_back(idv::Vec3(m_afForestExtents[3], m_afForestExtents[2]));
#else
    vPoints.push_back(idv::Vec3(m_afForestExtents[3], m_afForestExtents[4]));
    vPoints.push_back(idv::Vec3(m_afForestExtents[0], m_afForestExtents[4]));
    vPoints.push_back(idv::Vec3(m_afForestExtents[0], m_afForestExtents[1]));
    vPoints.push_back(idv::Vec3(m_afForestExtents[3], m_afForestExtents[1]));
#endif

    // make sure we don't overflow - if we do, cap it 
    int nNumPoints = CheckForOverflow(int(vPoints.size( )));

    // render the buffer
    if (!vPoints.empty( ))
    {
        m_cOverlayVB.SetBuffer((void*) &vPoints[0], nNumPoints);
        if (m_cOverlayVB.BindVertexBuffer(true))
        {
            (void) m_cOverlayVB.DrawArrays(CIdvInterleavedBuffer::TRIANGLE_FAN, 0, nNumPoints);
            (void) m_cOverlayVB.UnBindVertexBuffer(true);
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::Draw2dFlatGrid

void CSpeedTreeCullEngine::Draw2dFlatGrid(CIdvEffect* pEffect, CIdvConstant* pColor)
{
    // set color
    (void) pColor->Set4f(1.0f, 1.0f, 1.0f, 0.3f);
#ifndef LOAD_BINARY_SHADERS
    (void) pEffect->CommitChanges( );
#endif

    // fill vertex buffer with vert lines
    const float c_fLeft = m_afForestExtents[0];
    const float c_fRight = m_afForestExtents[3];
#ifdef UPVECTOR_POS_Y
    const float c_fTop = m_afForestExtents[5];
    const float c_fBottom = m_afForestExtents[2];
#else
    const float c_fTop = m_afForestExtents[4];
    const float c_fBottom = m_afForestExtents[1];
#endif

    vector<idv::Vec3> vPoints;
    for (int nCol = 0; nCol <= m_nNumCellCols; ++nCol)
    {
        float x = c_fLeft + nCol * (c_fRight - c_fLeft) / m_nNumCellCols;

        vPoints.push_back(idv::Vec3(x, c_fBottom, 0.0f));
        vPoints.push_back(idv::Vec3(x, c_fTop, 0.0f));
    }

    // fill vertex buffer with horz lines
    for (int nRow = 0; nRow <= m_nNumCellRows; ++nRow)
    {
        float y = c_fBottom + nRow * (c_fTop - c_fBottom) / m_nNumCellRows;

        vPoints.push_back(idv::Vec3(m_afForestExtents[0], y, 0.0f));
        vPoints.push_back(idv::Vec3(m_afForestExtents[3], y, 0.0f));
    }

    // make sure we don't blow up the m_cOverlayVB buffer
    int nNumPoints = CheckForOverflow(int(vPoints.size( )));

    // render the buffer
    if (!vPoints.empty( ))
    {
        m_cOverlayVB.SetBuffer((void*) &vPoints[0], nNumPoints);
        if (m_cOverlayVB.BindVertexBuffer(true))
        {
            (void) m_cOverlayVB.DrawArrays(CIdvInterleavedBuffer::LINES, 0, nNumPoints);
            (void) m_cOverlayVB.UnBindVertexBuffer(true);
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::Draw2dFrustum

void CSpeedTreeCullEngine::Draw2dFrustum(CIdvEffect* pEffect, CIdvConstant* pColor)
{
    // fill vertex buffer with frustum lines
    std::vector<idv::Vec3> vPoints;
    vPoints.push_back(Convert3d(m_acFrustumPoints[0]));
    vPoints.push_back(Convert3d(m_acFrustumPoints[1]));
    vPoints.push_back(Convert3d(m_acFrustumPoints[0]));
    vPoints.push_back(Convert3d(m_acFrustumPoints[2]));
    vPoints.push_back(Convert3d(m_acFrustumPoints[0]));
    vPoints.push_back(Convert3d(m_acFrustumPoints[3]));
    vPoints.push_back(Convert3d(m_acFrustumPoints[0]));
    vPoints.push_back(Convert3d(m_acFrustumPoints[4]));
    vPoints.push_back(Convert3d(m_acFrustumPoints[1]));
    vPoints.push_back(Convert3d(m_acFrustumPoints[2]));
    vPoints.push_back(Convert3d(m_acFrustumPoints[2]));
    vPoints.push_back(Convert3d(m_acFrustumPoints[3]));
    vPoints.push_back(Convert3d(m_acFrustumPoints[3]));
    vPoints.push_back(Convert3d(m_acFrustumPoints[4]));
    vPoints.push_back(Convert3d(m_acFrustumPoints[4]));
    vPoints.push_back(Convert3d(m_acFrustumPoints[1]));

    // add points for drawing ortho box
    int nOrthoOffset = int(vPoints.size( ));
#ifdef UPVECTOR_POS_Y
    vPoints.push_back(idv::Vec3(-m_afFrustumMin[0], m_afFrustumMin[2]));
    vPoints.push_back(idv::Vec3(-m_afFrustumMax[0], m_afFrustumMin[2]));
    vPoints.push_back(idv::Vec3(-m_afFrustumMax[0], m_afFrustumMax[2]));
    vPoints.push_back(idv::Vec3(-m_afFrustumMin[0], m_afFrustumMax[2]));
    vPoints.push_back(idv::Vec3(-m_afFrustumMin[0], m_afFrustumMin[2]));
#else
    vPoints.push_back(idv::Vec3(m_afFrustumMin[0], m_afFrustumMin[1]));
    vPoints.push_back(idv::Vec3(m_afFrustumMax[0], m_afFrustumMin[1]));
    vPoints.push_back(idv::Vec3(m_afFrustumMax[0], m_afFrustumMax[1]));
    vPoints.push_back(idv::Vec3(m_afFrustumMin[0], m_afFrustumMax[1]));
    vPoints.push_back(idv::Vec3(m_afFrustumMin[0], m_afFrustumMin[1]));
#endif

    // make sure we don't blow up the m_cOverlayVB buffer
    int nNumPoints = CheckForOverflow(int(vPoints.size( )));

    // render the buffer
    if (!vPoints.empty( ))
    {
        m_cOverlayVB.SetBuffer((void*) &vPoints[0], nNumPoints);

        // draw frustum
        (void) pColor->Set4f(1.0f, 1.0f, 0.0f, 1.0f);
#ifndef LOAD_BINARY_SHADERS
        (void) pEffect->CommitChanges( );
#endif
        if (m_cOverlayVB.BindVertexBuffer(true))
        {
            (void) m_cOverlayVB.DrawArrays(CIdvInterleavedBuffer::LINE_STRIP, 0, nNumPoints - 5);

            // draw frustum ortho outline
            (void) pColor->Set4f(1.0f, 1.0f, 0.0f, 0.45f);
#ifndef LOAD_BINARY_SHADERS
            (void) pEffect->CommitChanges( );
#endif
            (void) m_cOverlayVB.DrawArrays(CIdvInterleavedBuffer::LINE_STRIP, nOrthoOffset, 5);

            (void) m_cOverlayVB.UnBindVertexBuffer(true);
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::Draw2dActiveCells

void CSpeedTreeCullEngine::Draw2dActiveCells(CIdvEffect* pEffect, CIdvConstant* pColor)
{
    // set color
    (void) pColor->Set4f(1.0f, 1.0f, 1.0f, 1.0f);
#ifndef LOAD_BINARY_SHADERS
    (void) pEffect->CommitChanges( );
#endif

    // fill vertex buffer with cells
    const float c_fLeft = m_afForestExtents[0];
    const float c_fRight = m_afForestExtents[3];
#ifdef UPVECTOR_POS_Y
    const float c_fTop = m_afForestExtents[5];
    const float c_fBottom = m_afForestExtents[2];
#else
    const float c_fTop = m_afForestExtents[4];
    const float c_fBottom = m_afForestExtents[1];
#endif

    const float c_fCellWidth = (c_fRight - c_fLeft) / m_nNumCellCols;
    const float c_fCellHeight = (c_fTop - c_fBottom) / m_nNumCellRows;

    vector<idv::Vec3> vPoints;
    int nNumCells = int(m_vVisibleCells.size( ));
    for (int nCell = 0; nCell < nNumCells; ++nCell)
    {
        SCell* pCell = m_vVisibleCells[nCell];
        if (!pCell->m_vTrees.empty( ))
        {
            float x = c_fLeft + pCell->m_nCol * c_fCellWidth;
            float y = c_fBottom + pCell->m_nRow * c_fCellHeight;

#ifdef UPVECTOR_POS_Y
            x = -x;
#endif

            // draw outline - a line_loop would be ideal, but not all platforms support it - we'll
            // use a line_strip where the first and last points are the same
            vPoints.push_back(idv::Vec3(x, y, 0.0f));
#ifdef UPVECTOR_POS_Y
            vPoints.push_back(idv::Vec3(x - c_fCellWidth, y, 0.0f));
            vPoints.push_back(idv::Vec3(x - c_fCellWidth, y + c_fCellHeight, 0.0f));
#else
            vPoints.push_back(idv::Vec3(x + c_fCellWidth, y, 0.0f));
            vPoints.push_back(idv::Vec3(x + c_fCellWidth, y + c_fCellHeight, 0.0f));
#endif
            vPoints.push_back(idv::Vec3(x, y + c_fCellHeight, 0.0f));
            vPoints.push_back(idv::Vec3(x, y, 0.0f));
        }
    }

    // make sure we don't overflow - if we do, cap it 
    int nNumPoints = CheckForOverflow(int(vPoints.size( )));

    // render the buffer
    if (!vPoints.empty( ))
    {
        m_cOverlayVB.SetBuffer((void*) &vPoints[0], nNumPoints);
        if (m_cOverlayVB.BindVertexBuffer(true))
        {
            int nNumCells = nNumPoints / 5;
            for (int i = 0; i < nNumCells; ++i)
                (void) m_cOverlayVB.DrawArrays(CIdvInterleavedBuffer::LINE_STRIP, i * 5, 5);

            m_cOverlayVB.UnBindVertexBuffer(true);
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::Draw2dMeshTrees

void CSpeedTreeCullEngine::Draw2dMeshTrees(CIdvEffect* pEffect, CIdvConstant* pColor)
{
    // set color
    (void) pColor->Set4f(0.0f, 1.0f, 0.0f, 1.0f);
#ifndef LOAD_BINARY_SHADERS
    (void) pEffect->CommitChanges( );
#endif

    // add 3d tree points to the vertex buffer
    vector<idv::Vec3> vPoints;
    vector<float> vLods;
    for (int nBase = 0; nBase < m_nNumBaseTrees; ++nBase)
    {
        const CSpeedTreeInstance* pTree = First3dTree(nBase);
        while (pTree)
        {
#ifdef UPVECTOR_POS_Y
            vPoints.push_back(idv::Vec3(-pTree->Pos( )[0], pTree->Pos( )[2]));
#else
            vPoints.push_back(idv::Vec3(pTree->Pos( )[0], pTree->Pos( )[1]));
#endif
            vLods.push_back(pTree->GetLod( ));
            pTree = Next3dTree( );
        }
    }

    // make sure we don't overflow - if we do, cap it 
    int nNumPoints = CheckForOverflow(int(vPoints.size( )));

    // render the buffer
    if (!vPoints.empty( ))
    {
        m_cOverlayVB.SetBuffer((void*) &vPoints[0], nNumPoints);
        if (m_cOverlayVB.BindVertexBuffer(true))
        {
            for (int i = 0; i < int(vPoints.size( )); ++i)
            {
                // fade from red (highest LOD) to green (lowest LOD)
                (void) pColor->Set4f(vLods[i], 1.0f - vLods[i], 0.0f, 1.0f);
#ifndef LOAD_BINARY_SHADERS
                (void) pEffect->CommitChanges( );
#endif

                (void) m_cOverlayVB.DrawArrays(CIdvInterleavedBuffer::POINTS, i, 1);
            }
            (void) m_cOverlayVB.UnBindVertexBuffer(true);
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::Draw2dBillboardTrees

void CSpeedTreeCullEngine::Draw2dBillboardTrees(CIdvEffect* pEffect, CIdvConstant* pColor)
{
    // add billboards
    vector<idv::Vec3> vFullPoints;
    vector<idv::Vec3> vPartialPoint;
    for (int nCell = 0; nCell < int(m_vVisibleCells.size( )); ++nCell)
    {
        SCell* pCell = m_vVisibleCells[nCell];
        if (pCell->m_eLodStatus == LOD_ALL_BILLBOARDS)
        {
            for (int nTree = 0; nTree < int(pCell->m_vTrees.size( )); ++nTree)
            {
#ifdef UPVECTOR_POS_Y
                vFullPoints.push_back(idv::Vec3(-pCell->m_vTrees[nTree]->Pos( )[0], pCell->m_vTrees[nTree]->Pos( )[2]));
#else
                vFullPoints.push_back(idv::Vec3(pCell->m_vTrees[nTree]->Pos( )[0], pCell->m_vTrees[nTree]->Pos( )[1]));
#endif
            }
        }
        // in-between LOD batches and 3D trees
        else
        {
            for (int nTree = 0; nTree < int(pCell->m_vTrees.size( )); ++nTree)
            {
#ifdef CULL_INDIVIDUAL_BILLBOARDS
                if (pCell->m_vTrees[nTree]->GetLod( ) == 0.0f && !pCell->m_vTrees[nTree]->IsCulled( ))
#else
                if (pCell->m_vTrees[nTree]->GetLod( ) == 0.0f)
#endif
                {
#ifdef UPVECTOR_POS_Y
                    vPartialPoint.push_back(idv::Vec3(-pCell->m_vTrees[nTree]->Pos( )[0], pCell->m_vTrees[nTree]->Pos( )[2]));
#else
                    vPartialPoint.push_back(idv::Vec3(pCell->m_vTrees[nTree]->Pos( )[0], pCell->m_vTrees[nTree]->Pos( )[1]));
#endif
                }
            }
        }
    }

    // full points
    {
        // make sure we don't overflow - if we do, cap it 
        int nNumPoints = CheckForOverflow(int(vFullPoints.size( )));

        // render the buffers
        if (!vFullPoints.empty( ))
        {
            // full points
            (void) pColor->Set4f(0.0f, 1.0f, 1.0f, 0.3f);
#ifndef LOAD_BINARY_SHADERS
            (void) pEffect->CommitChanges( );
#endif

            m_cOverlayVB.SetBuffer((void*) &vFullPoints[0], nNumPoints);
            if (m_cOverlayVB.BindVertexBuffer(true))
            {
                (void) m_cOverlayVB.DrawArrays(CIdvInterleavedBuffer::POINTS, 0, nNumPoints);
                (void) m_cOverlayVB.UnBindVertexBuffer(true);
            }
        }
    }

    // partial points
    {
        // make sure we don't overflow - if we do, cap it 
        int nNumPoints = CheckForOverflow(int(vPartialPoint.size( )));

        // render the buffers
        if (!vPartialPoint.empty( ))
        {
            // full points
            (void) pColor->Set4f(0.0f, 1.0f, 1.0f, 1.0f);
#ifndef LOAD_BINARY_SHADERS
            (void) pEffect->CommitChanges( );
#endif

            m_cOverlayVB.SetBuffer((void*) &vPartialPoint[0], nNumPoints);
            if (m_cOverlayVB.BindVertexBuffer(true))
            {
                (void) m_cOverlayVB.DrawArrays(CIdvInterleavedBuffer::POINTS, 0, nNumPoints);
                (void) m_cOverlayVB.UnBindVertexBuffer(true);
            }
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeCullEngine::CheckForOverflow

int CSpeedTreeCullEngine::CheckForOverflow(int nNumPoints)
{
    // make sure we don't overflow - if we do, cap it 
    if (nNumPoints > c_nMaxOverlayBufferVerts)
    {
        IdvWarning("# of points overflows overlay vertex buffer [%d > %d]", nNumPoints, c_nMaxOverlayBufferVerts);
        nNumPoints = c_nMaxOverlayBufferVerts;
    }

    return nNumPoints;
}




