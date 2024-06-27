///////////////////////////////////////////////////////////////////////  
//  LeafGeometry.cpp
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

#include "LeafGeometry.h"
#include "IndexedGeometry.h"
#include "WindEngine.h"
#include "LeafOffsetTable.h"
#include "UpVector.h"
#include "TreeEngine.h"
#include <vector>
using namespace std;


///////////////////////////////////////////////////////////////////////  
//  SLodGeometry::SLodGeometry definition

CLeafGeometry::SLodGeometry::SLodGeometry( ) :
    m_bValid(false),
    m_pOrigCenterCoords(NULL)
{
}


///////////////////////////////////////////////////////////////////////  
//  SLodGeometry::~SLodGeometry definition

CLeafGeometry::SLodGeometry::~SLodGeometry( )
{
    // all of float attributes point to the same pool (the first element of which
    // can be either m_pOrigCenterCoords or m_pCenterCoords)
    if (m_pOrigCenterCoords)
        st_delete_array<float>(m_pOrigCenterCoords, "~SLodGeometry, m_pOrigCenterCoords");
    else
        st_delete_array<const float>(m_pCenterCoords, "~SLodGeometry, m_pCenterCoords");
    m_pOrigCenterCoords = NULL;
    m_pCenterCoords = NULL;
    m_pDimming = NULL;
    m_pNormals = NULL;
    m_pBinormals = NULL;
    m_pTangents = NULL;
    m_pWindWeights[0] = m_pWindWeights[1] = NULL;

    // delete variables inherited from CSpeedTreeRT::SGeometry::SLeaf
    st_delete_array<const unsigned char>(m_pLeafMapIndices, "~SLodGeometry, m_pLeafMapIndices");
    st_delete_array<const unsigned char>(m_pLeafCardIndices, "~SLodGeometry, m_pLeafCardIndices");
    st_delete_array<const unsigned int>(m_pColors, "~SLodGeometry, m_pColors");
    st_delete_array<const unsigned char>(m_pWindMatrixIndices[0], "~SLodGeometry, m_pWindMatrixIndices[0]");
    st_delete_array<const unsigned char>(m_pWindMatrixIndices[1], "~SLodGeometry, m_pWindMatrixIndices[1]");
    st_delete_array<CSpeedTreeRT::SGeometry::SLeaf::SCard>(m_pCards, "~SLodGeometry, m_pCards");

    // delete the mesh leaf information here
    const int c_nNumLeafMeshClusters = int(m_vMeshes.size( ));
    for (int i = 0; i < c_nNumLeafMeshClusters; ++i)
    {
        // all of the attributes point to one large array stored in m_pCoords
        st_delete_array<const float>(m_vMeshes[i].m_pCoords, "~SLodGeometry, m_pClusterMeshes->m_pCoords");
        m_vMeshes[i].m_pTexCoords = NULL;
        m_vMeshes[i].m_pNormals = NULL;
        m_vMeshes[i].m_pBinormals = NULL;
        m_vMeshes[i].m_pTangents = NULL;
    }
}


///////////////////////////////////////////////////////////////////////  
//  CLeafGeometry::CLeafGeometry definition

CLeafGeometry::CLeafGeometry(CWindEngine* pWindEngine) :
    m_bManualLighting(false),
    m_bVertexWeighting(false),
    m_pTreeEngine(NULL),
    // wind
    m_pWindEngine(pWindEngine),
    // compute tables
    m_nNumRockingGroups(3),
    m_pTimeOffsets(NULL),
    m_pLeafVertexes(NULL),
    m_pLeafTexCoords(NULL),
    m_pVertexProgramTable(NULL),
    // texture information
    m_nNumTextures(0),
    m_pLeafDimensions(NULL),
    m_pLeafOrigins(NULL),
    // LODs
    m_nNumLods(0),
    m_pLods(NULL)
{
    st_assert(pWindEngine);
}


///////////////////////////////////////////////////////////////////////  
//  CLeafGeometry::~CLeafGeometry definition

CLeafGeometry::~CLeafGeometry( )
{
    st_delete_array<stVec3>(m_pLeafDimensions, "~CLeafGeometry, m_pLeafDimensions");
    st_delete_array<stVec3>(m_pLeafOrigins, "~CLeafGeometry, m_pLeafOrigins");
    st_delete_array<CLeafGeometry::SLodGeometry>(m_pLods, "~CLeafGeometry, m_pLods");
    st_delete_array<float>(m_pVertexProgramTable, "~CLeafGeometry, m_pVertexProgramTable");
    
    m_pTimeOffsets = NULL;
    m_pLeafTexCoords = NULL;
}


///////////////////////////////////////////////////////////////////////  
//  CLeafGeometry::Invalidate definition

void CLeafGeometry::Invalidate(void)
{
    if (m_pLods)
        for (int i = 0; i < m_nNumLods; ++i)
            m_pLods[i].Invalidate( );
}


///////////////////////////////////////////////////////////////////////  
//  CLeafGeometry::GetLeafBillboardTable definition

const float* CLeafGeometry::GetLeafBillboardTable(unsigned int& nEntryCount) const
{
    // copy the leaf map position coordinates
    float* pTable = m_pVertexProgramTable;
    int nLodSize = m_nNumTextures * m_nNumRockingGroups * 2 * 4 * 4; // 2 for mirrored, 4 corners per leaf, 4 floats per vertex
    if (pTable && m_pLeafVertexes && m_pLeafVertexes[0] && m_pVertexProgramTable)
    {
        memcpy(pTable, m_pLeafVertexes[0], nLodSize * sizeof(float));
        pTable += nLodSize;

        // calculate total floats in table
        nEntryCount = int(pTable - m_pVertexProgramTable);
    }

    return m_pVertexProgramTable;
}


///////////////////////////////////////////////////////////////////////  
//  CLeafGeometry::ComputeExtents definition
//
//  Because the leaf clusters rock backand forth, taking their
//  contribution to the tree's overall bounding box is not
//  very straightforward.
//
//  Each leaf has a position, a size, and a leafmap origin.  The
//  longest diagonal from the origin the four corner is computed
//  for each leaf map type.  Those are then added to the leaf
//  positions throughout the tree in all six orthographic 
//  directions.

void CLeafGeometry::ComputeExtents(stRegion& cExtents) const
{
    if (m_pLeafOrigins &&
        m_pLeafDimensions &&
        m_pLods)
    {
        // for each basic leafmap type, determined by how many leaf texture
        // maps were specified in CAD, determine the longest diagonals.
        st_vector_float vLongestDiagonal(m_nNumTextures);
        const st_vector_leaftexture& vTextures = m_pTreeEngine->GetLeafTextures( );
        for (int i = 0; i < m_nNumTextures; ++i)
        {
            float fLongest = 0.0f;

            if (vTextures[i].m_bUseMeshes)
            {
                // this texture uses a mesh
                const SIdvMeshInfo* pOriginalMesh = m_pTreeEngine->GetMeshInfo(vTextures[i].m_nMeshLeafIndex);
                if (pOriginalMesh != NULL)
                {
                    float fMeshScale = st_max(m_pLeafDimensions[i][0], m_pLeafDimensions[i][1]);
                    int uiNumVerts = static_cast<int>(pOriginalMesh->m_vVertices.size( ));
                    for (int iVertIndex = 0; iVertIndex < uiNumVerts; ++iVertIndex)
                    {
                        const float fDist = fMeshScale * pOriginalMesh->m_vVertices[iVertIndex].m_cPos.Magnitude( );
                        if (fDist > fLongest)
                            fLongest = fDist;
                    }
                }
            }
            else
            {
                const stVec3 cScaledOrigin(m_pLeafOrigins[i][0] * m_pLeafDimensions[i][0], 
                                            m_pLeafOrigins[i][1] * m_pLeafDimensions[i][1]);

                if (cScaledOrigin.Distance(stVec3(0.0f, 0.0f)) > fLongest)
                    fLongest = cScaledOrigin.Distance(stVec3(0.0f, 0.0f));

                if (cScaledOrigin.Distance(stVec3(m_pLeafDimensions[i][0], 0.0f)) > fLongest)
                    fLongest = cScaledOrigin.Distance(stVec3(m_pLeafDimensions[i][0], 0.0f));

                if (cScaledOrigin.Distance(stVec3(m_pLeafDimensions[i][0], m_pLeafDimensions[i][1])) > fLongest)
                    fLongest = cScaledOrigin.Distance(stVec3(m_pLeafDimensions[i][0], m_pLeafDimensions[i][1]));

                if (cScaledOrigin.Distance(stVec3(0.0f, m_pLeafDimensions[i][1])) > fLongest)
                    fLongest = cScaledOrigin.Distance(stVec3(0.0f, m_pLeafDimensions[i][1]));
            }

            vLongestDiagonal[i] = fLongest;
        }

        // for each leaf in all leaf LODs, determine maximum space taken
        // in all six orthographic directions.

        //  for (int nLod = 0; nLod < m_nNumLods; ++nLod)
        for (int nLod = 0; nLod < 1; ++nLod) // now using only the highest LOD
        {
            SLodGeometry* pLod = m_pLods + nLod;
            for (int j = 0; j < pLod->m_nNumLeaves; ++j)
            {
                stRegion cLeafSwingExtents;

                const float* pPos = pLod->m_pCenterCoords + j * 3;
                const stVec3 cCenter(pPos[0], pPos[1], pPos[2]);
                const float fLongestSide = vLongestDiagonal[pLod->m_pLeafMapIndices[j] / 2];

                // the CRegion operator^ expands the region to include the right-hand stVec3 object.
                cLeafSwingExtents = cLeafSwingExtents ^ stVec3(cCenter[0] + fLongestSide, cCenter[1], cCenter[2]);
                cLeafSwingExtents = cLeafSwingExtents ^ stVec3(cCenter[0] - fLongestSide, cCenter[1], cCenter[2]);
                cLeafSwingExtents = cLeafSwingExtents ^ stVec3(cCenter[0], cCenter[1] + fLongestSide, cCenter[2]);
                cLeafSwingExtents = cLeafSwingExtents ^ stVec3(cCenter[0], cCenter[1] - fLongestSide, cCenter[2]);
                cLeafSwingExtents = cLeafSwingExtents ^ stVec3(cCenter[0], cCenter[1], cCenter[2] + fLongestSide);
                cLeafSwingExtents = cLeafSwingExtents ^ stVec3(cCenter[0], cCenter[1], cCenter[2] - fLongestSide);

                cExtents = cExtents + cLeafSwingExtents;
            }
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  CLeafGeometry::Transform definition

void CLeafGeometry::Transform(const stTransform& cTransform)
{
    if (m_pLods &&
        m_pLeafDimensions)
    {
        for (int nLod = 0; nLod < m_nNumLods; ++nLod)
        {
            SLodGeometry* pLod = m_pLods + nLod;

            // transform the base positions of the leaves
            for (int i = 0; i < pLod->m_nNumLeaves; ++i)
            {
                // transform positions
                float* pCoord = const_cast<float*>(pLod->m_pCenterCoords + i * 3);
                stVec3 cCoord(pCoord[0], pCoord[1], pCoord[2]);
                cCoord = cCoord * cTransform;
                memcpy(pCoord, cCoord.m_afData, 3 * sizeof(float));

                // m_pOrigPositions is used as a basis for wind computations,
                // need to transform those too, if they are being used
                if (m_bVertexWeighting)
                {
                    float* pOrigCoord = pLod->m_pOrigCenterCoords + i * 3;
                    stVec3 cOrigCoord(pOrigCoord[0], pOrigCoord[1], pOrigCoord[2]);
                    cOrigCoord = cOrigCoord * cTransform;
                    memcpy(pOrigCoord, cOrigCoord.m_afData, 3 * sizeof(float));
                }
            }
        }

        // scale the leaves up by x-scale factor (arbitrary)
        float fScalar = cTransform.m_afData[0][0];
        for (int j = 0; j < m_nNumTextures; ++j)
            m_pLeafDimensions[j] *= fScalar;
    }
}


///////////////////////////////////////////////////////////////////////  
//  CLeafGeometry::Init definition

void CLeafGeometry::Init(int nNumLeafLods, const st_vector_leaves* pvLeafLods, const SIdvLeafInfo& sLeafInfo, float fSizeIncreaseFactor)
{
    // set table info
    m_nNumRockingGroups = static_cast<unsigned short>(sLeafInfo.m_nNumRockingGroups);

    // deletion of these three pointers in still handled in the SIdvLeafInfo destructor
    m_pTimeOffsets = sLeafInfo.m_pTimeOffsets;
    m_pLeafVertexes = sLeafInfo.m_pLeafVertexes;
    m_pLeafTexCoords = sLeafInfo.m_pLeafTexCoords;

    // set texture info
    m_nNumTextures = static_cast<unsigned short>(sLeafInfo.m_vLeafTextures.size( ));
    m_pLeafDimensions = st_new_array<stVec3>(m_nNumTextures, "CLeafGeometry::Init, m_pLeafDimensions");
    m_pLeafOrigins = st_new_array<stVec3>(m_nNumTextures, "CLeafGeometry::Init, m_pLeafOrigins");
    for (int i = 0; i < m_nNumTextures; ++i)
    {
        m_pLeafDimensions[i] = sLeafInfo.m_vLeafTextures[i].m_cSizeUsed;
        m_pLeafOrigins[i] = sLeafInfo.m_vLeafTextures[i].m_cOrigin;
    }

    // new leaf vertex shader table
    int nSize = m_nNumTextures * m_nNumRockingGroups * 2 * 4 * 4;  // 2 for mirrored, 4 corners per leaf, 4 floats per vertex
    m_pVertexProgramTable = st_new_array<float>(nSize, "CLeafGeometry::Init, m_pVertexProgramTable");

    InitLods(nNumLeafLods, pvLeafLods, fSizeIncreaseFactor);
}


///////////////////////////////////////////////////////////////////////  
//  CLeafGeometry::SetTextureCoords definition
//
//  When composite leaf maps are used, this function allows CSpeedTreeRT to
//  pass in user-specified texture coordinates for each leaf map.

void CLeafGeometry::SetTextureCoords(unsigned int nLeafMapIndex, const float* pTexCoords)
{
    if (m_pLeafTexCoords &&
        pTexCoords)
    {
        float fSign = CSpeedTreeRT::GetTextureFlip( ) ? -1.0f : 1.0f;

        // one leaf map is setup just like the user specified
        float* pEntry = m_pLeafTexCoords + (nLeafMapIndex * 2) * (2 * 4);
        *pEntry++ = pTexCoords[0];
        *pEntry++ = fSign * pTexCoords[1];
        *pEntry++ = pTexCoords[2];
        *pEntry++ = fSign * pTexCoords[3];
        *pEntry++ = pTexCoords[4];
        *pEntry++ = fSign * pTexCoords[5];
        *pEntry++ = pTexCoords[6];
        *pEntry++ = fSign * pTexCoords[7];

        // another is mirrored (by swapping S coordinates) for a more interesting effect
        pEntry = m_pLeafTexCoords + (nLeafMapIndex * 2 + 1) * (2 * 4);
        *pEntry++ = pTexCoords[2];
        *pEntry++ = fSign * pTexCoords[1];
        *pEntry++ = pTexCoords[0];
        *pEntry++ = fSign * pTexCoords[3];
        *pEntry++ = pTexCoords[6];
        *pEntry++ = fSign * pTexCoords[5];
        *pEntry++ = pTexCoords[4];
        *pEntry++ = fSign * pTexCoords[7];

        // update mesh texcoords
        const st_vector_leaftexture& vTextures = m_pTreeEngine->GetLeafTextures( );
        for (int nLod = 0; nLod < m_nNumLods; ++nLod)
        {           
            for (unsigned int nTexture = nLeafMapIndex * 2; nTexture < nLeafMapIndex * 2 + 2; ++nTexture)
            {
                if (vTextures[nTexture / 2].m_bUseMeshes)
                {
                    bool bMirrored = !(nTexture % 2);
                    for (int nGroup = 0; nGroup < m_nNumRockingGroups; ++nGroup)
                    {
                        SLeafCard* pCard = &m_pLods[nLod].m_pCards[nTexture * m_nNumRockingGroups + nGroup];
                        CSpeedTreeRT::SGeometry::SLeaf::SMesh* pMeshLeaf = &m_pLods[nLod].m_vMeshes[nTexture * m_nNumRockingGroups + nGroup];
                        const SIdvMeshInfo* pOriginalMesh = m_pTreeEngine->GetMeshInfo(vTextures[nTexture / 2].m_nMeshLeafIndex);
                        if (pMeshLeaf!= NULL && pOriginalMesh != NULL)
                        {
                            float* pTex = const_cast<float*>(pMeshLeaf->m_pTexCoords);
                            for (int iVertIndex = 0; iVertIndex < pMeshLeaf->m_nNumVertices; ++iVertIndex)
                            {
                                pTex[iVertIndex * 2 + 0] = VecInterpolate(pCard->m_pTexCoords[bMirrored ? 4 : 0], 
                                                                          pCard->m_pTexCoords[bMirrored ? 0 : 4], 
                                                                          pOriginalMesh->m_vVertices[iVertIndex].m_afUV[0]);
                                pTex[iVertIndex * 2 + 1] = VecInterpolate(pCard->m_pTexCoords[5], 
                                                                          pCard->m_pTexCoords[1], 
                                                                          pOriginalMesh->m_vVertices[iVertIndex].m_afUV[1]);
                            }
                        }
                    }
                }
            }
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  CLeafGeometry::Update definition
//
//  Update is generally called once per frame per tree.  It advances the leaf rocking
//  groups by setting up and applying the billboard and rocking computations.  Part of
//  the optimization scheme is have a small number of template leaves that are billboarded
//  and rocked and the rest of the leaves are offset from them.

void CLeafGeometry::Update(CSpeedTreeRT::SGeometry::SLeaf& sGeometry, int nLodLevel, float fAzimuth, float fPitch, float fSizeIncreaseFactor)
{
    if (m_pLods &&
        nLodLevel < m_nNumLods &&
        m_pLeafVertexes &&
        m_pLeafOrigins &&
        m_pLeafDimensions &&
        m_pWindEngine &&
        m_pTimeOffsets)
    {
        SLodGeometry* pLod = m_pLods + nLodLevel;
        st_assert(pLod);

        const st_vector_leaftexture& vTextures = m_pTreeEngine->GetLeafTextures( );

        if (!pLod->IsValid( ))
        {
            if (m_pWindEngine->GetLeafWindMethod( ) == CSpeedTreeRT::WIND_CPU)
                (void) ComputeWindEffect(nLodLevel);

            // determines how much larger the lower LODs get relative to the original leaves
            float fSizeMultiplier = 1.0f;
            if (nLodLevel > 0)
                fSizeMultiplier = 1.0f + fSizeIncreaseFactor * float(nLodLevel);

            stVec3 acRawCorners[4];
            stVec3 acBillboardedCorners[4];
            for (int i = 0; i < m_nNumTextures * 2; ++i) // X 2 for mirrored textures
            {
                // determine origin and size
                stVec3 cOrigin = m_pLeafOrigins[i / 2];
                if (i % 2) // if this is a mirrored leaf, flip x-value of origin
                    cOrigin[0] = 1.0f - cOrigin[0];
                const stVec3& cSize = m_pLeafDimensions[i / 2];

                // layout the coordinates for the simple, non-billboarded leaf
                float afP1[3] = { 0.0f, fSizeMultiplier * (1.0f - cOrigin[0]) * cSize[0], fSizeMultiplier * (1.0f - cOrigin[1]) * cSize[1] };
                float afP2[3] = { 0.0f, fSizeMultiplier * -(cOrigin[0] * cSize[0]), afP1[2] };
                float afP3[3] = { 0.0f, afP2[1], fSizeMultiplier * -(cOrigin[1] * cSize[1]) };
                float afP4[3] = { 0.0f, afP1[1], afP3[2] };
                
                // adjust the coordinates for alternate coordinate system
                Assign3d(acRawCorners[0].m_afData, afP1);
                Assign3d(acRawCorners[1].m_afData, afP2);
                Assign3d(acRawCorners[2].m_afData, afP3);
                Assign3d(acRawCorners[3].m_afData, afP4);

                // each leaf texture has m_nNumRockingGroups number of leaf groups
                stVec3 cTempVec;
                for (int j = 0; j < m_nNumRockingGroups; ++j)
                {
                    // Both billboarding and rocking/rustling are performed in CWindEngine's RockLeaf( ) methods
                    stRotTransform cTrans;
                    if (!m_pWindEngine->UsingExternalRockAngles( ))
                        m_pWindEngine->RockLeaf(fAzimuth, fPitch, i * m_nNumRockingGroups + j, (i + 1) * m_pTimeOffsets[j], cTrans);
                    else
                        m_pWindEngine->RockLeaf(fAzimuth, fPitch, i * m_nNumRockingGroups + j, cTrans);

                    // transform the straight quad into a billboarded/rocking leaf
                    acBillboardedCorners[0] = acRawCorners[0] * cTrans;
                    acBillboardedCorners[1] = acRawCorners[1] * cTrans;
                    acBillboardedCorners[2] = acRawCorners[2] * cTrans;
                    acBillboardedCorners[3] = acRawCorners[3] * cTrans;

                    // copy data into array for use by client application
                    float* pBillboard = m_pLeafVertexes[nLodLevel] + i * (m_nNumRockingGroups * 16) + j * 16;
                    memcpy(pBillboard, acBillboardedCorners[0], 3 * sizeof(float)); pBillboard += 4;
                    memcpy(pBillboard, acBillboardedCorners[1], 3 * sizeof(float)); pBillboard += 4;
                    memcpy(pBillboard, acBillboardedCorners[2], 3 * sizeof(float)); pBillboard += 4;
                    memcpy(pBillboard, acBillboardedCorners[3], 3 * sizeof(float)); pBillboard += 4;

                    // mesh leaves
                    if (vTextures[i / 2].m_bUseMeshes)
                    {
                        cTrans.LoadIdentity( );
                        if (m_pWindEngine->UsingExternalRockAngles( ))
                            m_pWindEngine->RockMesh(i * m_nNumRockingGroups + j, cTrans);
                        else
                            m_pWindEngine->RockMesh((i + 1) * m_pTimeOffsets[j], cTrans);

                        SLeafCard* pCluster = &pLod->m_pCards[i * m_nNumRockingGroups + j];

                        const SIdvMeshInfo* pOriginalMesh = m_pTreeEngine->GetMeshInfo(vTextures[i / 2].m_nMeshLeafIndex);

                        if (pCluster != NULL && pOriginalMesh != NULL)
                        {
                            CSpeedTreeRT::SGeometry::SLeaf::SMesh* pMeshLeaf = &pLod->m_vMeshes[i * m_nNumRockingGroups + j];
                            float* pPos = const_cast<float*>(pMeshLeaf->m_pCoords);
                            float* pNormal = const_cast<float*>(pMeshLeaf->m_pNormals);
                            float* pBinormal = const_cast<float*>(pMeshLeaf->m_pBinormals);
                            float* pTangent = const_cast<float*>(pMeshLeaf->m_pTangents);

                            const float fMeshScale = st_max(pCluster->m_fWidth, pCluster->m_fHeight);

                            for (int iVertIndex = 0; iVertIndex < pMeshLeaf->m_nNumVertices; ++iVertIndex)
                            {
                                const SIdvMeshVertex* pSrc = &pOriginalMesh->m_vVertices[iVertIndex];

                                Assign3d(pPos, pSrc->m_cPos * fMeshScale * cTrans);
                                Assign3d(pNormal, pSrc->m_cNormal * cTrans);
                                Assign3d(pBinormal, pSrc->m_cBinormal * cTrans);
                                Assign3d(pTangent, pSrc->m_cTangent * cTrans);
                                
                                pPos += 3;
                                pNormal += 3;
                                pBinormal += 3;
                                pTangent += 3;
                            }
                        }
                    }
                }
            }

            pLod->Validate( );
        }
        
        // update the appropriate pointers and variables of the resulting computations
        sGeometry = *pLod;
        //sGeometry.m_bIsActive = true;
        sGeometry.m_nDiscreteLodLevel = nLodLevel;
    }
}


///////////////////////////////////////////////////////////////////////  
//  CLeafGeometry::ComputeWindEffect definition
//
//  This is the code for the CPU leaf wind solution.  It takes two 
//  points:  the original leaf position, and the leaf position multiplied
//  times the appropriate wind matrix, and then interpolates between them
//  based on the wind weight value (0.0 to 1.0).

void CLeafGeometry::ComputeWindEffect(int nLodLevel)
{
    if (m_pWindEngine &&
        nLodLevel < m_nNumLods &&
        m_bVertexWeighting &&
        m_pLods)
    {
        SLodGeometry* pLod = m_pLods + nLodLevel;

        if (pLod &&
            pLod->m_pOrigCenterCoords &&
            pLod->m_pCenterCoords)
        {
            stVec3 cFullWindEffect, cInterpWindEffect;
            for (int i = 0; i < pLod->m_nNumLeaves; ++i)
            {
                // original one level wind computation
                float fWeight = pLod->m_pWindWeights[0][i];
                float* pOrigCoord = pLod->m_pOrigCenterCoords + i * 3;

                cFullWindEffect.Set(pOrigCoord[0], pOrigCoord[1], pOrigCoord[2]);
                unsigned char ucWindGroup = pLod->m_pWindMatrixIndices[0][i];
                cFullWindEffect = cFullWindEffect * m_pWindEngine->GetWindMatrix(ucWindGroup);
                cInterpWindEffect.Set(VecInterpolate(pOrigCoord[0], cFullWindEffect[0], fWeight),
                                    VecInterpolate(pOrigCoord[1], cFullWindEffect[1], fWeight),
                                    VecInterpolate(pOrigCoord[2], cFullWindEffect[2], fWeight));

                // additional second level wind computation
                fWeight = pLod->m_pWindWeights[1][i];

                cFullWindEffect = cInterpWindEffect;
                ucWindGroup = pLod->m_pWindMatrixIndices[1][i];
                cFullWindEffect = cFullWindEffect * m_pWindEngine->GetWindMatrix(ucWindGroup);
                cInterpWindEffect.Set(VecInterpolate(cInterpWindEffect[0], cFullWindEffect[0], fWeight),
                                    VecInterpolate(cInterpWindEffect[1], cFullWindEffect[1], fWeight),
                                    VecInterpolate(cInterpWindEffect[2], cFullWindEffect[2], fWeight));

                memcpy(const_cast<float*>(pLod->m_pCenterCoords + i * 3), cInterpWindEffect.m_afData, 12);
            }
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  CLeafGeometry::InitLods definition
//
//  This is where the bulk of the work is done in transfering the leaf data computed
//  by CBranch and CTreeEngine.  The code changes depending on the API/engine 
//  defined during compilation as well as the coordinate system.

void CLeafGeometry::InitLods(int nNumLeafLods, const st_vector_leaves* pvLeafLods, float fSizeIncreaseFactor)
{
    if (pvLeafLods &&
        m_pLeafVertexes &&
        m_pWindEngine &&
        m_pLeafTexCoords)
    {
        m_nNumLods = nNumLeafLods;
        m_pLods = st_new_array<SLodGeometry>(nNumLeafLods, "CLeafGeometry::InitLods, m_pLods");

        for (int nLod = 0; nLod < nNumLeafLods; ++nLod)
        {
            SLodGeometry* pLod = m_pLods + nLod;

            // init some parameters
            pLod->m_nDiscreteLodLevel = nLod;
            pLod->m_nNumLeaves = int(pvLeafLods[nLod].size( ));
            pLod->m_fLeafRockScalar = m_pWindEngine->GetSpeedWindRockScalar( );
            pLod->m_fLeafRustleScalar = m_pWindEngine->GetSpeedWindRustleScalar( );

            // setup clusters
            InitClusters(*pLod, fSizeIncreaseFactor);

            // allocate one large float array to be pointed to by the attribute pointers in order to
            // reduce memory fragmentation caused by many small allocations
            int nNumFloats = 0;
            if (m_bVertexWeighting)
            {
                nNumFloats += pLod->m_nNumLeaves * 3;       // pLod->m_pOrigCenterCoords
                nNumFloats += pLod->m_nNumLeaves;           // pLod->m_pWindWeights[0]
                nNumFloats += pLod->m_nNumLeaves;           // pLod->m_pWindWeights[1]
            }
            nNumFloats += pLod->m_nNumLeaves * 3;           // pLod->m_pCenterCoords
            if (!m_bManualLighting)
            {
                nNumFloats += 4 * pLod->m_nNumLeaves * 3;   // pLod->m_pNormals
                nNumFloats += 4 * pLod->m_nNumLeaves * 3;   // pLod->m_pTangents
                nNumFloats += 4 * pLod->m_nNumLeaves * 3;   // pLod->m_pBinormals
            }
            nNumFloats += pLod->m_nNumLeaves;               // pLod->m_pDimming
            float* pShared = st_new_array<float>(nNumFloats, "CLeafGeometry::InitLods, pLod->m_pOrigCenterCoords (shared)");

            // wind related
            if (m_bVertexWeighting)
            {
                // orig positions (basis for wind)
                pLod->m_pOrigCenterCoords = pShared;
                pShared += pLod->m_nNumLeaves * 3;
                int i = 0; // declared outside of the for-scope for portability
                for (i = 0; i < pLod->m_nNumLeaves; ++i)
                    Assign3d(pLod->m_pOrigCenterCoords + i * 3, pvLeafLods[nLod][i].GetPosition( ).m_afData);

                // wind weights
                pLod->m_pWindWeights[0] = pShared;
                pShared += pLod->m_nNumLeaves;
                for (i = 0; i < pLod->m_nNumLeaves; ++i)
                    (const_cast<float*>(pLod->m_pWindWeights[0]))[i] = 1.0f - pvLeafLods[nLod][i].GetWeight1( );

                pLod->m_pWindWeights[1] = pShared;
                pShared += pLod->m_nNumLeaves;
                for (i = 0; i < pLod->m_nNumLeaves; ++i)
                    (const_cast<float*>(pLod->m_pWindWeights[1]))[i] = 1.0f - pvLeafLods[nLod][i].GetWeight2( );

                // wind groups
                pLod->m_pWindMatrixIndices[0] = st_new_array<unsigned char>(pLod->m_nNumLeaves, "CLeafGeometry::InitLods, pLod->m_pWindMatrixIndices[0]");
                for (i = 0; i < pLod->m_nNumLeaves; ++i)
                {
                    unsigned char ucGroup = pvLeafLods[nLod][i].GetWindGroup1( );

                    // chGroup spans a wide range of matrices, mod it down to size
                    unsigned int nStartingMatrix = 0, nMatrixSpan = 1;
                    m_pWindEngine->GetLocalMatrices(nStartingMatrix, nMatrixSpan);
                    ucGroup = static_cast<unsigned char>(nStartingMatrix) + ucGroup % static_cast<unsigned char>(nMatrixSpan);

                    (const_cast<unsigned char*>(pLod->m_pWindMatrixIndices[0]))[i] = ucGroup;
                }

                pLod->m_pWindMatrixIndices[1] = st_new_array<unsigned char>(pLod->m_nNumLeaves, "CLeafGeometry::InitLods, pLod->m_pWindMatrixIndices[1]");
                for (i = 0; i < pLod->m_nNumLeaves; ++i)
                {
                    unsigned char ucGroup = pvLeafLods[nLod][i].GetWindGroup2( );

                    // chGroup spans a wide range of matrices, mod it down to size
                    unsigned int nStartingMatrix = 0, nMatrixSpan = 1;
                    m_pWindEngine->GetLocalMatrices(nStartingMatrix, nMatrixSpan);
                    ucGroup = static_cast<unsigned char>(nStartingMatrix) + ucGroup % static_cast<unsigned char>(nMatrixSpan);

                    (const_cast<unsigned char*>(pLod->m_pWindMatrixIndices[1]))[i] = ucGroup;
                }
            }

            // positions
            pLod->m_pCenterCoords = pShared;
            pShared += pLod->m_nNumLeaves * 3;
            int i = 0; // declared outside of the for-scope for portability
            for (i = 0; i < pLod->m_nNumLeaves; ++i)
                Assign3d(const_cast<float*>(pLod->m_pCenterCoords + i * 3), pvLeafLods[nLod][i].GetPosition( ).m_afData);

            // texture indexes
            pLod->m_pLeafMapIndices = st_new_array<unsigned char>(pLod->m_nNumLeaves, "CLeafGeometry::InitLods, pLod->m_pLeafMapIndices");
            for (i = 0; i < pLod->m_nNumLeaves; ++i)
                (const_cast<unsigned char*>(pLod->m_pLeafMapIndices))[i] = static_cast<unsigned char>(pvLeafLods[nLod][i].GetAltTextureIndex( ));

            // cluster indices, used for vertex shaders mostly
            pLod->m_pLeafCardIndices = st_new_array<unsigned char>(pLod->m_nNumLeaves, "CLeafGeometry::InitLods, pLod->m_pLeafCardIndices");
            for (i = 0; i < pLod->m_nNumLeaves; ++i)
                (const_cast<unsigned char*>(pLod->m_pLeafCardIndices))[i] = static_cast<unsigned char>(pvLeafLods[nLod][i].GetAltTextureIndex( ) *
                    m_nNumRockingGroups + pvLeafLods[nLod][i].GetAngle( ));

            // colors
            pLod->m_pColors = st_new_array<unsigned int>(4 * pLod->m_nNumLeaves, "CLeafGeometry::InitLods, pLod->m_pColors");
            for (i = 0; i < pLod->m_nNumLeaves; ++i)
            {
                (const_cast<unsigned int*>(pLod->m_pColors))[i * 4 + 0] = pvLeafLods[nLod][i].GetColorUInt(0);
                (const_cast<unsigned int*>(pLod->m_pColors))[i * 4 + 1] = pvLeafLods[nLod][i].GetColorUInt(1);
                (const_cast<unsigned int*>(pLod->m_pColors))[i * 4 + 2] = pvLeafLods[nLod][i].GetColorUInt(2);
                (const_cast<unsigned int*>(pLod->m_pColors))[i * 4 + 3] = pvLeafLods[nLod][i].GetColorUInt(3);
            }


            // normals
            if (!m_bManualLighting)
            {
                pLod->m_pNormals = pShared;
                pShared += 4 * pLod->m_nNumLeaves * 3;
                for (i = 0; i < pLod->m_nNumLeaves; ++i)
                {
                    Assign3d(const_cast<float*>(pLod->m_pNormals + i * 12 + 0), pvLeafLods[nLod][i].GetNormal(0).m_afData);
                    Assign3d(const_cast<float*>(pLod->m_pNormals + i * 12 + 3), pvLeafLods[nLod][i].GetNormal(1).m_afData);
                    Assign3d(const_cast<float*>(pLod->m_pNormals + i * 12 + 6), pvLeafLods[nLod][i].GetNormal(2).m_afData);
                    Assign3d(const_cast<float*>(pLod->m_pNormals + i * 12 + 9), pvLeafLods[nLod][i].GetNormal(3).m_afData);
                }

                // normal mapping support
                pLod->m_pTangents = pShared;
                pShared += 4 * pLod->m_nNumLeaves * 3;
                pLod->m_pBinormals = pShared;
                pShared += 4 * pLod->m_nNumLeaves * 3;
                for (i = 0; i < pLod->m_nNumLeaves; ++i)
                {
                    // tangents
                    Assign3d(const_cast<float*>(pLod->m_pTangents + i * 12 + 0), pvLeafLods[nLod][i].GetTangent(0).m_afData);
                    Assign3d(const_cast<float*>(pLod->m_pTangents + i * 12 + 3), pvLeafLods[nLod][i].GetTangent(1).m_afData);
                    Assign3d(const_cast<float*>(pLod->m_pTangents + i * 12 + 6), pvLeafLods[nLod][i].GetTangent(2).m_afData);
                    Assign3d(const_cast<float*>(pLod->m_pTangents + i * 12 + 9), pvLeafLods[nLod][i].GetTangent(3).m_afData);

                    // binormals
                    Assign3d(const_cast<float*>(pLod->m_pBinormals + i * 12 + 0), pvLeafLods[nLod][i].GetBinormal(0).m_afData);
                    Assign3d(const_cast<float*>(pLod->m_pBinormals + i * 12 + 3), pvLeafLods[nLod][i].GetBinormal(1).m_afData);
                    Assign3d(const_cast<float*>(pLod->m_pBinormals + i * 12 + 6), pvLeafLods[nLod][i].GetBinormal(2).m_afData);
                    Assign3d(const_cast<float*>(pLod->m_pBinormals + i * 12 + 9), pvLeafLods[nLod][i].GetBinormal(3).m_afData);             
                }

                // dimming (for static lighting, the dimming is baked into the color)
                pLod->m_pDimming = pShared;//st_new_array<float>(pLod->m_nNumLeaves, "CLeafGeometry::InitLods, pLod->m_pDimming");
                pShared += 4 * pLod->m_nNumLeaves * 3;
                for (i = 0; i < pLod->m_nNumLeaves; ++i)
                    (const_cast<float*>(pLod->m_pDimming))[i] = pvLeafLods[nLod][i].GetDimming( );
            }
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  CLeafGeometry::InitClusters definition

void CLeafGeometry::InitClusters(SLodGeometry& sLod, float fSizeIncreaseFactor)
{
    const st_vector_leaftexture& vTextures = m_pTreeEngine->GetLeafTextures( );

    const int c_nNumClustersPerLod = m_nNumTextures * m_nNumRockingGroups * 2;
    sLod.m_pCards = st_new_array<SLeafCard>(c_nNumClustersPerLod, "CLeafGeometry::InitClusters, m_pCards[0]");
    
    // make mesh data for the clusters
    sLod.m_vMeshes.resize(c_nNumClustersPerLod);

    const float fSizeMultiplier = (sLod.m_nDiscreteLodLevel > 0) ? 1.0f + fSizeIncreaseFactor * float(sLod.m_nDiscreteLodLevel) : 1.0f;

    // update each cluster
    SLeafCard* pCluster = sLod.m_pCards;
    for (int nTexture = 0; nTexture < m_nNumTextures * 2; ++nTexture)
    {
        const float fMeshScale = st_max(m_pLeafDimensions[nTexture / 2][0], m_pLeafDimensions[nTexture / 2][1]);

        bool bMirrored = !(nTexture % 2);
        for (int nGroup = 0; nGroup < m_nNumRockingGroups; ++nGroup)
        {
            pCluster->m_fWidth = fSizeMultiplier * m_pLeafDimensions[nTexture / 2][0];
            pCluster->m_fHeight = fSizeMultiplier * m_pLeafDimensions[nTexture / 2][1];
            pCluster->m_afPivotPoint[0] = bMirrored ? 1.0f - m_pLeafOrigins[nTexture / 2][0] : m_pLeafOrigins[nTexture / 2][0];
            pCluster->m_afPivotPoint[1] = m_pLeafOrigins[nTexture / 2][1];
            const int c_nTableIndex = (nTexture * m_nNumRockingGroups) % c_nLeafOffsetCount;
            pCluster->m_afAngleOffsets[0] = c_afLeafOffsets[c_nTableIndex];
            pCluster->m_afAngleOffsets[1] = c_afLeafOffsets[c_nLeafOffsetCount - c_nTableIndex - 1];
            pCluster->m_pTexCoords = m_pLeafTexCoords + nTexture * 8;
            pCluster->m_pCoords = m_pLeafVertexes[sLod.m_nDiscreteLodLevel] + nTexture * (m_nNumRockingGroups * 16) + nGroup * 16;

            // mesh leaves
            CSpeedTreeRT::SGeometry::SLeaf::SMesh* pMesh = &sLod.m_vMeshes[nTexture * m_nNumRockingGroups + nGroup];
            pCluster->m_pMesh = NULL;
            if (vTextures[nTexture / 2].m_bUseMeshes)
            {
                const SIdvMeshInfo* pOriginalMesh = m_pTreeEngine->GetMeshInfo(vTextures[nTexture / 2].m_nMeshLeafIndex);
                if (pOriginalMesh != NULL)
                {   
                    // vertex buffer (unique per cluster because of size and texcoords)
                    pCluster->m_pMesh = pMesh;
                    pMesh->m_nNumVertices = int(pOriginalMesh->m_vVertices.size( ));

                    // allocate one large float array to be pointed to by the attribute pointers in order to
                    // reduce memory fragmentation caused by many small allocations
                    int nNumFloats = 0;
                    nNumFloats += pMesh->m_nNumVertices * 3; // pMesh->m_pCoords
                    nNumFloats += pMesh->m_nNumVertices * 2; // pMesh->m_pTexCoords
                    nNumFloats += pMesh->m_nNumVertices * 3; // pMesh->m_pNormals
                    nNumFloats += pMesh->m_nNumVertices * 3; // pMesh->m_pBinormals
                    nNumFloats += pMesh->m_nNumVertices * 3; // pMesh->m_pTangents
                    float* pShared = st_new_array<float>(nNumFloats, "CLeafGeometry::InitClusters, pMesh->m_pCoords (shared)");

                    pMesh->m_pCoords = pShared;
                    pShared += pMesh->m_nNumVertices * 3;

                    pMesh->m_pTexCoords = pShared;
                    pShared += pMesh->m_nNumVertices * 2;

                    pMesh->m_pNormals = pShared;
                    pShared += pMesh->m_nNumVertices * 3;

                    pMesh->m_pBinormals = pShared;
                    pShared += pMesh->m_nNumVertices * 3;

                    pMesh->m_pTangents = pShared;

                    float* pPos = (float*) pMesh->m_pCoords;
                    float* pTex = (float*) pMesh->m_pTexCoords;
                    float* pNormal = (float*) pMesh->m_pNormals;
                    float* pBinormal = (float*) pMesh->m_pBinormals;
                    float* pTangent = (float*) pMesh->m_pTangents;

                    for (int iVertIndex = 0; iVertIndex < pMesh->m_nNumVertices; ++iVertIndex)
                    {
                        const SIdvMeshVertex* pSrc = &(pOriginalMesh->m_vVertices[iVertIndex]);

                        stVec3 cPos = pSrc->m_cPos;
                        cPos *= fMeshScale;
                        Assign3d(pPos, cPos);
                        
                        memcpy(pTex, pSrc->m_afUV, 2 * sizeof(float));

                        Assign3d(pNormal, pSrc->m_cNormal);
                        Assign3d(pBinormal, pSrc->m_cBinormal);
                        Assign3d(pTangent, pSrc->m_cTangent);

                        pPos += 3;
                        pTex += 2;
                        pNormal += 3;
                        pBinormal += 3;
                        pTangent += 3;
                    }

                    // index buffer (shared)
                    pMesh->m_nNumIndices = int(pOriginalMesh->m_vIndices.size( ));
                    pMesh->m_pIndices = &pOriginalMesh->m_vIndices[0];
                }
            }

            ++pCluster;
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  CLeafGeometry::GetTextureCoords definition

void CLeafGeometry::GetTextureCoords(unsigned int nLeafMapIndex, float* pTexCoords) const
{
    if (m_pLeafTexCoords &&
        pTexCoords)
    {
        const float* pEntry = m_pLeafTexCoords + (nLeafMapIndex * 2) * (2 * 4);
        memcpy(pTexCoords, pEntry, 8 * sizeof(float));
    }
}


///////////////////////////////////////////////////////////////////////  
//  CLeafGeometry::DeleteGeometry definition

void CLeafGeometry::DeleteGeometry(void)
{
    if (m_pWindEngine && m_pWindEngine->GetLeafWindMethod( ) != CSpeedTreeRT::WIND_CPU)
        st_delete_array<CLeafGeometry::SLodGeometry>(m_pLods, "CLeafGeometry::DeleteGeometry, m_pLods");
}


///////////////////////////////////////////////////////////////////////  
//  CLeafGeometry::GetTriangleCount definition

unsigned int CLeafGeometry::GetTriangleCount(unsigned int nLodLevel) const      
{ 
    st_assert(m_pLods);

    unsigned int nCount = 0;
    const SLodGeometry* pLod = &m_pLods[nLodLevel];
    for (int i = 0; i < pLod->m_nNumLeaves; ++i)
    {
        if (pLod->m_pCards[pLod->m_pLeafCardIndices[i]].m_pMesh)
            nCount += pLod->m_pCards[pLod->m_pLeafCardIndices[i]].m_pMesh->m_nNumIndices / 3;
        else
            nCount += 2;
    }

    return nCount; 
}

