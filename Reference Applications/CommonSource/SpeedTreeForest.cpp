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

#include "IdvGlobal.h"
#include <cstdio>
#include "SpeedTreeForest.h"
#include "SpeedTreeWrapper.h"
#include <cfloat>
#include "IdvFilename.h"
#include "IdvRandom.h"
#include <algorithm>
#include "IdvState.h"
#include "IdvTimer.h"
#include "UpVector.h"
#include "IdvVectorMath.h"
#ifdef _XBOX
#include <omp.h>
#endif
#ifdef PS3
#include <sys/paths.h>
#endif
using namespace std;


///////////////////////////////////////////////////////////////////////  
//  Static variables

CEnvironment* CSpeedTreeForest::m_pEnvironment = NULL;
int CSpeedTreeForest::m_nRefCount = 0;


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::CSpeedTreeForest

CSpeedTreeForest::CSpeedTreeForest( ) :
      m_fLightScale(1.0f)
    , m_fFogStart(300.0f)
    , m_fFogEnd(2000.0f)
    , m_bSelfShadowBound(false)
    , m_b360BillboardsSupported(false)
    , m_bAlphaTesting(true)
    , m_bWireframe(false)
    , m_bNormalMapping(true)
    , m_bVisible(true)
    , m_fNearLod(300.0f)
    , m_fFarLod(600.0f)
    , m_bDebugFlag(false)
    , m_fTime(0.0f)
    , m_fSelfShadowAdvance(0.0f)
#ifdef _XBOX
    , m_strMainShaderFile("game:\\Shaders\\SpeedTree.fx")
    , m_strEnvironmentShaderFile("game:\\Shaders\\Ground.fx")
#elif PS3
    , m_strMainShaderFile(SYS_APP_HOME"/Data/Shaders/SpeedTree.fx")
    , m_strEnvironmentShaderFile(SYS_APP_HOME"/Data/Shaders/Ground.fx")
#else
    , m_strMainShaderFile("../../Data/Shaders/SpeedTree.fx")
    , m_strEnvironmentShaderFile("../../Data/Shaders/Ground.fx")
#endif
    , m_uiScreenWidth(0)
    , m_uiScreenHeight(0)
    , m_fOverlayZoomFactor(0.95f)
{
    ++m_nRefCount;
    if (m_nRefCount == 1)
        m_pEnvironment = new CEnvironment;

    for (int nAxis = 0; nAxis < 3; ++nAxis)
    {
        m_afExtents[nAxis] = FLT_MAX;
        m_afExtents[nAxis + 3] = -FLT_MAX;
    }

    // default lighting
    m_afLightColor[0] = m_afLightColor[1] = m_afLightColor[2] = 1.0f;
    m_afLightDir[0] = -0.707f;
    m_afLightDir[1] = 0.0f;
    m_afLightDir[2] = 0.707f;

    // fog color
    m_afFogColor[0] = m_afFogColor[1] = m_afFogColor[2] = m_afFogColor[3] = 1.0f;

    for (int i = 0; i < 16; ++i)
        m_afLastModelviewMatrix[i] = 0.0f;
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::~CSpeedTreeForest

CSpeedTreeForest::~CSpeedTreeForest( )
{
    --m_nRefCount;
    for (int i = 0; i < int(m_vBaseTrees.size( )); ++i)
    {
        delete m_vBaseTrees[i];
        m_vBaseTrees[i] = NULL;
    }

    if (m_nRefCount == 0)
    {
        delete m_pEnvironment;
        m_pEnvironment = NULL;
    }
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::Draw

void CSpeedTreeForest::Draw(unsigned int uiFlagVector)
{
    m_sDrawStats.Reset( );

    // initial render states & constants
    CIdvState::SetAlphaTesting(m_bAlphaTesting && !m_bWireframe);
    CIdvState::SetRenderStyle(m_bWireframe ? CIdvState::WIREFRAME : CIdvState::SOLID);
    (void) m_conLightColor.Set3fv(m_afLightColor);
    (void) m_conLightColor_PS.Set3fv(m_afLightColor);
    (void) m_conLightDir.Set3fv(m_afLightDir);
    (void) m_conWorldViewProjectionMatrix.SetModelviewProjectionMatrix( );
    (void) m_conFogColor.Set4fv(m_afFogColor);
    (void) m_conFogParams.Set3f(m_fFogStart, m_fFogEnd, m_fFogEnd - m_fFogStart);

    // upload wind matrices
    UploadWindMatricesToShader( );

    // branches
    CIdvState::SetFaceCulling(CIdvState::BACK);
    if (uiFlagVector & BRANCHES)
    {
        (void) m_conSelfShadowMap.SetTexture(m_texSelfShadow, 2);
        m_bSelfShadowBound = true;

        DrawBranches( );
    }

    // set composite texture
    (void) m_conDiffuseCompositeLeafMap.SetTexture(m_texLeavesAndFronds[CSpeedTreeRT::TL_DIFFUSE], 0);
    if (m_bNormalMapping)
        (void) m_conNormalCompositeLeafMap.SetTexture(m_texLeavesAndFronds[CSpeedTreeRT::TL_NORMAL], 1);
    else
        (void) m_conNormalCompositeLeafMap.SetTexture(m_texFlatNormalMap, 1);

    // leaves 
    CIdvState::SetFaceCulling(CIdvState::NONE);

    if (uiFlagVector & LEAVES)
    {
        DrawLeafCards( );
        DrawLeafMeshes( );
    }

    // fronds
    if (uiFlagVector & FRONDS)
    {
        (void) m_conSelfShadowMap.SetTexture(m_texSelfShadow, 2);
        m_bSelfShadowBound = true;

        DrawFronds( );
    }

    CIdvTexture::Disable(1);

    // billboards
    if (uiFlagVector & BILLBOARDS)
    {
        (void) m_conBillboardDiffuseMap.SetTexture(m_texBillboards[CSpeedTreeRT::TL_DIFFUSE], 0);
        (void) m_conBillboardNormalMap.SetTexture(m_texBillboards[CSpeedTreeRT::TL_NORMAL], 1);

        UploadLightAdjustment( );
        if (!m_v360TexCoordTable.empty( ))
        {
            st_assert(int(m_v360TexCoordTable.size( )) % 4 == 0);
            (void) m_con360TexCoords.SetArray4f(&m_v360TexCoordTable[0], int(m_v360TexCoordTable.size( )) / 4);
        }

        DrawVertBillboards(0);
        if (m_b360BillboardsSupported)
            DrawVertBillboards(1);

#ifdef HORZ_BILLBOARDS
        DrawHorzBillboards( );
#endif
    }

    // debug draws
    //m_cCullEngine.DrawActiveCells( );
    //DrawCullRadii( );
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::DrawGround

void CSpeedTreeForest::DrawGround(void)
{
    float afExtents[6];
    m_cCullEngine.GetFrustumExtents(afExtents, afExtents + 3);

    m_pEnvironment->Draw(afExtents);
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::PopulateFromStfFile

bool CSpeedTreeForest::PopulateFromStfFile(const char* pFilename)
{
    bool bSuccess = false;

    // open the file
    int nAttemptedReads = 0, nSuccessfulReads = 0;            
    FILE* pFile = fopen(pFilename, "r");
    if (pFile)
    {
        // find the end of the file
        fseek(pFile, 0L, SEEK_END);
        int nEnd = ftell(pFile);
        fseek(pFile, 0L, SEEK_SET);

        int nTree = 0;
        st_vector_baseparams vBaseTrees;
        st_vector_instparams vInstances;
        while (ftell(pFile) != nEnd)
        {
            // keep track of the tree for error reporting
            ++nTree;

            // read the mandatory data
            char szName[1024];
            unsigned int nSeed = 0, nNumInstances = 0;
            float fSize = 0.0f, fSizeVariance = 0.0f;
            float afPos[3] = { 0.0f };
            nAttemptedReads++;
            if (fscanf(pFile, "%s %d %g %g %d\n", szName, &nSeed, &fSize, &fSizeVariance, &nNumInstances) == 5)
            {
                nSuccessfulReads++;
                nAttemptedReads += nNumInstances;

                // add the base tree to the filename list
                SBaseParams sBaseTree;
                sBaseTree.m_strSptFile = IdvPath(pFilename) + st_string(szName);
                sBaseTree.m_nSeed = nSeed;
                sBaseTree.m_fSize = fSize;
                sBaseTree.m_fSizeVariance = fSizeVariance;
                vBaseTrees.push_back(sBaseTree);

                // instances
                for (unsigned int i = 0; i < nNumInstances; ++i)
                {
                    // read the instance location
                    float fRotation = 0.0f;
                    float fScale = 1.0f;
                    if (fscanf(pFile, "%g %g %g %g %g\n", afPos, afPos + 1, afPos + 2, &fRotation, &fScale) == 5)
                    {
                        // make an instance of this tree
                        SInstanceParams sInstance;
                        memcpy(sInstance.m_afPos, afPos, 3 * sizeof(float));
                        sInstance.m_fRotation = fRotation;
                        sInstance.m_fScale = fScale;
                        sInstance.m_nBaseIndex = int(vBaseTrees.size( )) - 1;
                        vInstances.push_back(sInstance);

                        ++nSuccessfulReads;
                    }
                    else
                        IdvWarning("Error reading STF instance data [tree %d in %s]", nTree, pFilename);
                }
            }
            else
                IdvWarning("Error reading STF base data [tree %d in %s]", nTree, pFilename);
        }
        fclose(pFile);

        if (!vBaseTrees.empty( ) &&
            !vInstances.empty( ) &&
            nAttemptedReads == nSuccessfulReads &&
            nAttemptedReads > 0)
        {
            bSuccess = PopulateManually(&vBaseTrees[0], int(vBaseTrees.size( )), &vInstances[0], int(vInstances.size( )));
        }
    }
    else
        IdvError("Failed to open STF file [%s]: %s", pFilename, strerror(errno));

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::PopulateRandomly

bool CSpeedTreeForest::PopulateRandomly(const float* pExtents, const st_vector_string& vSptFiles, int nNumInstances)
{
    bool bSuccess = false;

    if (!vSptFiles.empty( ) && nNumInstances > 0)
    {
        // copy filenames into base parameters vector
        int nNumBaseTrees = int(vSptFiles.size( ));
        st_vector_baseparams vBaseTrees(nNumBaseTrees);
        int i = 0;
        for (i = 0; i < nNumBaseTrees; ++i)
            vBaseTrees[i].m_strSptFile = vSptFiles[i];

        // build an array of instance parameters
        st_vector_instparams vParams(nNumInstances);
        for (i = 0; i < nNumInstances; ++i)
        {
            vParams[i].m_nBaseIndex = IdvRandom(0, int(vSptFiles.size( ) - 1));
            // trees are placed on flat plane, but don't have to be
#ifdef UPVECTOR_POS_Y
            vParams[i].m_afPos[0] = IdvRandom(pExtents[0], pExtents[3]);
            vParams[i].m_afPos[1] = 0.0f;
            vParams[i].m_afPos[2] = IdvRandom(pExtents[2], pExtents[5]);
#else
            vParams[i].m_afPos[0] = IdvRandom(pExtents[0], pExtents[3]);
            vParams[i].m_afPos[1] = IdvRandom(pExtents[1], pExtents[4]);
            vParams[i].m_afPos[2] = 0.0f;
#endif
 
            vParams[i].m_fRotation = IdvRandom(0.0f, idv::Pi * 2.0f);
            vParams[i].m_fScale = IdvRandom(0.7f, 1.5f);
        }

        // populate
        bSuccess = PopulateManually(&vBaseTrees[0], int(vBaseTrees.size( )), &vParams[0], int(vParams.size( )));
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::AdjustExtents

void CSpeedTreeForest::AdjustExtents(float x, float y, float z)
{
    // min
    m_afExtents[0] = st_min(m_afExtents[0], x);
    m_afExtents[1] = st_min(m_afExtents[1], y); 
    m_afExtents[2] = st_min(m_afExtents[2], z); 

    // max
    m_afExtents[3] = st_max(m_afExtents[3], x); 
    m_afExtents[4] = st_max(m_afExtents[4], y); 
    m_afExtents[5] = st_max(m_afExtents[5], z);
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::AdjustExtents

bool CSpeedTreeForest::PopulateManually(const SBaseParams* pBaseTrees, int nNumBaseTrees, const SInstanceParams* pInstances, int nNumInstances, const idv::Region& cExtents)
{
    bool bSuccess = false;

    // guess at path
    const st_string strPath = (nNumBaseTrees > 0 && !pBaseTrees) ? "" : IdvPath(pBaseTrees[0].m_strSptFile);

    // load wind configuration (need to pass some info from here into the SPT models)
    InitWind(m_strSpeedWindFile.empty( ) ? strPath + "SpeedWind.ini" : m_strSpeedWindFile);

    // create and load the base trees
    int i = 0;
    for (i = 0; i < nNumBaseTrees; ++i)
    {
        // make a new tree structure
        CSpeedTreeWrapper* pBaseTree = new CSpeedTreeWrapper;

        // load this base tree that its instances will reference
        st_string strError = pBaseTree->LoadSptFile(pBaseTrees[i].m_strSptFile.c_str( ), 
                                                    m_cSpeedWind.GetNumWindMatrices( ),
                                                    pBaseTrees[i].m_nSeed,
                                                    pBaseTrees[i].m_fSize,
                                                    pBaseTrees[i].m_fSizeVariance);
        if (strError.empty( ))
            AddBaseTree(pBaseTree);
        else
            IdvError("SPT load failure - %s", strError.c_str( ));
    }

    if (!m_vBaseTrees.empty( ))
    {
        m_vAllTrees.resize(nNumInstances);
        for (i = 0; i < nNumInstances; ++i)
        {
            const SInstanceParams* pInstanceParams = pInstances + i;

            // pick a random base tree
            st_assert(pInstanceParams->m_nBaseIndex > -1 && pInstanceParams->m_nBaseIndex < int(m_vBaseTrees.size( )));
            CSpeedTreeWrapper* pBaseTree = m_vBaseTrees[pInstanceParams->m_nBaseIndex];

            // make the new instance
            CSpeedTreeInstance* pInstance = &m_vAllTrees[i];
            pBaseTree->MakeInstance(pInstance);
            pInstance->SetScale(pInstanceParams->m_fScale);
            pInstance->SetPos(pInstanceParams->m_afPos);
            pInstance->SetRotation(pInstanceParams->m_fRotation);
            pInstance->SetWindMatrixOffset(IdvRandom(0.0f, 1.0f));
            AddInstance(pInstance);
        }

        bSuccess = (int(m_vAllTrees.size( )) == nNumInstances);

        if (bSuccess)
        {
            // setup grid-based frustum culling
            if (InitGraphics(strPath) &&
                m_cCullEngine.Init(&m_vAllTrees[0], int(m_vAllTrees.size( )), int(m_vBaseTrees.size( )), 50, 50, cExtents))
            {
                bSuccess = true;
            }
        }
    }
    else
        IdvError("No SPT files loaded");

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CSortByBase

class CSortByBase
{
public:
    bool operator( )(const CSpeedTreeInstance& cLeft, const CSpeedTreeInstance& cRight)
    {
        return (cLeft.InstanceOf( )->SortIndex( ) < cRight.InstanceOf( )->SortIndex( ));
    }
};


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::InitGraphics
//
//  This function initializes most of the graphics constructs needed
//  for forest rendering.  Called after the forest has been populated.

bool CSpeedTreeForest::InitGraphics(const st_string& strResourcePath)
{
    bool bSuccess = false;

    // setup geometry
    int nNumBaseTrees = int(m_vBaseTrees.size( ));

    // branches
    bool bGeometryPresent = false;
    int i = 0;
    for (i = 0; i < nNumBaseTrees; ++i)
    {
        m_vBaseTrees[i]->SetupBranchGeometry(m_cBranchGeometry, m_cSpeedWind);
        if (m_vBaseTrees[i]->HasBranchGeometry( ))
            bGeometryPresent = true;
        m_vBaseTrees[i]->DeleteBranchGeometry( );
    }
    if (bGeometryPresent)
    {
        m_cBranchGeometry.FinishVertices( );
        m_cBranchGeometry.FinishIndices( );
    }

    // fronds
    bGeometryPresent = false;
    for (i = 0; i < nNumBaseTrees; ++i)
    {
        m_vBaseTrees[i]->SetupFrondGeometry(m_cFrondGeometry, m_cSpeedWind);
        if (m_vBaseTrees[i]->HasFrondGeometry( ))
            bGeometryPresent = true;
        m_vBaseTrees[i]->DeleteFrondGeometry( );
    }
    if (bGeometryPresent)
    {
        m_cFrondGeometry.FinishVertices( );
        m_cFrondGeometry.FinishIndices( );
    }

    // leaf cards
    bGeometryPresent = false;
    for (i = 0; i < nNumBaseTrees; ++i)
    {
        m_vBaseTrees[i]->SetupLeafCardGeometry(m_cLeafCardGeometry, m_cSpeedWind);
        if (m_vBaseTrees[i]->HasLeafCardGeometry( ))
            bGeometryPresent = true;
    }
    if (bGeometryPresent)
        m_cLeafCardGeometry.FinishVertices( );

    // leaf meshes
    bGeometryPresent = false;
    for (i = 0; i < nNumBaseTrees; ++i)
    {
        m_vBaseTrees[i]->SetupLeafMeshGeometry(m_cLeafMeshGeometry, m_cSpeedWind);
        if (m_vBaseTrees[i]->HasLeafMeshGeometry( ))
            bGeometryPresent = true;
        m_vBaseTrees[i]->DeleteLeafGeometry( );
    }
    if (bGeometryPresent)
    {
        m_cLeafMeshGeometry.FinishVertices( );
        m_cLeafMeshGeometry.FinishIndices( );
    }

    // billboards
    int nTexCoordsOffset = 0;
    for (i = 0; i < nNumBaseTrees; ++i)
    {
        m_vBaseTrees[i]->SetupBillboardGeometry(nTexCoordsOffset);
        nTexCoordsOffset += m_vBaseTrees[i]->GetDrawData( ).m_nNum360Billboards;
    }

    // flush the rest of the transient RT data 
    for (i = 0; i < nNumBaseTrees; ++i)
        m_vBaseTrees[i]->DeleteTransientData( );

    // build 360-degree billboard texcoords table
    BuildBillboardTexcoordsTable( );

    // textures
    LoadTextures(strResourcePath);

    // environment
    if (!m_pEnvironment->IsInitialized( ))
        (void) m_pEnvironment->Init(strResourcePath, m_strEnvironmentShaderFile, m_fFogStart, m_fFogEnd, m_afFogColor);

    // shaders
    if (LoadShaders(m_cSpeedWind))
        bSuccess = true;

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::SetFogParams

void CSpeedTreeForest::SetFogParams(float fStart, float fEnd, const float* pColor)
{
    st_assert(pColor);

    m_fFogStart = fStart;
    m_fFogEnd = fEnd;
    memcpy(m_afFogColor, pColor, 4 * sizeof(float));
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::SetProjection

void CSpeedTreeForest::SetProjection(float fAspectRatio, float fFieldOfView, float fNearClip, float fFarClip)
{
    m_cCullEngine.SetProjection(fAspectRatio, fFieldOfView, fNearClip, fFarClip);
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::UpdateCamera

void CSpeedTreeForest::UpdateCamera(const float* pCameraPos, const float* pProjectionMatrix, const float* pModelviewMatrix, bool bOverride)
{
    bool bCameraOrientationChanged = memcmp(m_afLastModelviewMatrix, pModelviewMatrix, 16 * sizeof(float)) != 0 ||
                                     memcmp(m_afLastProjectionMatrix, pProjectionMatrix, 16 * sizeof(float)) != 0;
    memcpy(m_afLastModelviewMatrix, pModelviewMatrix, 16 * sizeof(float));
    memcpy(m_afLastProjectionMatrix, pProjectionMatrix, 16 * sizeof(float));

    // this is a static call, so it will affect all of the shader's modelview/projection matrices
    CIdvConstant::UpdateModelviewProjectionMatrix(pProjectionMatrix, pModelviewMatrix);

    // run through the trees, updating the cull status for each
    if (bCameraOrientationChanged || bOverride)
    {
        m_cCullEngine.SetCamera(pCameraPos, pModelviewMatrix, pProjectionMatrix);
        m_cCullEngine.Cull( );

        // pass new camera information into SpeedTreeRT
        float afCameraDir[3] = { pModelviewMatrix[2], pModelviewMatrix[6], pModelviewMatrix[10] };
        CSpeedTreeRT::SetCamera(pCameraPos, afCameraDir);
    }
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::ForceCameraUpdate

void CSpeedTreeForest::ForceCameraUpdate(void)
{
    float afCameraPos[3], afCameraDir[3];
    CSpeedTreeRT::GetCamera(afCameraPos, afCameraDir);
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::AdjustSelfShadow
//
//  Returns true if a bind takes place

bool CSpeedTreeForest::AdjustSelfShadow(bool bGeometryReceivesShadow)
{
    bool bBind = false;

    if (bGeometryReceivesShadow && !m_bSelfShadowBound)
    {
        (void) m_conSelfShadowMap.SetTexture(m_texSelfShadow, 2);
        m_bSelfShadowBound = true;
        bBind = true;
    }
    else if (!bGeometryReceivesShadow && m_bSelfShadowBound)
    {
        (void) m_conSelfShadowMap.SetTexture(m_texWhite, 2);
        m_bSelfShadowBound = false;
        bBind = true;
    }

    return bBind;
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::DrawBranches

void CSpeedTreeForest::DrawBranches(void)
{
    bool bContextBound = false;

    // bind the branch vertex buffer
    if (!m_cBranchGeometry.Empty( ))
    {
        int nNumBaseTrees = int(m_vBaseTrees.size( ));
        for (int nBaseIndex = 0; nBaseIndex < nNumBaseTrees; ++nBaseIndex)
        {
            const CSpeedTreeWrapper* pBaseTree = m_vBaseTrees[nBaseIndex];
            st_assert(pBaseTree);
            if (pBaseTree->HasBranchGeometry( ) &&
                m_cCullEngine.First3dTree(nBaseIndex))
            {
                if (!bContextBound)
                {
                    m_sDrawStats.m_nBufferBinds++;
                    (void) m_cBranchTechnique.Begin( );
                    (void) m_cBranchGeometry.EnableFormat( );
                    (void) m_cBranchGeometry.BindVertexBuffer( );
                    (void) m_cBranchGeometry.BindIndexBuffer( );
                    bContextBound = true;
                }

                const CSpeedTreeWrapper::SDrawData* pDrawData = &pBaseTree->GetDrawData( );

                // upload textures
                (void) m_conBranchDiffuseMap.SetTexture(pBaseTree->GetBranchMap(CSpeedTreeRT::TL_DIFFUSE), 0);
                if (m_bNormalMapping)
                    (void) m_conBranchNormalMap.SetTexture(pBaseTree->GetBranchMap(CSpeedTreeRT::TL_NORMAL), 1);
                else
                    (void) m_conBranchNormalMap.SetTexture(m_texFlatNormalMap, 1);
#ifdef BRANCH_DETAIL_LAYER
                // not every tree will have a detail layer, so check that we have a valid one before binding
                const CIdvTexture& cDetailMap = pBaseTree->GetBranchMap(CSpeedTreeRT::TL_DETAIL);
                if (cDetailMap.IsValid( ))
                    (void) m_conBranchDetailMap.SetTexture(cDetailMap, 3);
                else
                    (void) m_conBranchDetailMap.SetTexture(m_texNoDetail, 3);
#endif
#if IDV_OPENGL && !PS3
                // Cg 1.4 (PC) requires this in order for a texture bind to take effect.  It's supposedly 
                // fixed in Cg 1.5, but Cg 1.5 Beta 1 had an insurmountable bug in it at the time of this release.
                (void) m_cBranchTechnique.Begin( );
#endif

                // upload material
                (void) m_conAmbientMaterial.Set3fv(pBaseTree->GetBranchMaterial( ).Ambient( ));
                (void) m_conDiffuseMaterial.Set3fv(pBaseTree->GetBranchMaterial( ).Diffuse( ));
                (void) m_conDiffuseScale.Set1f(pBaseTree->GetLightScales( ).m_fBranchLightScalar * pBaseTree->GetLightScales( ).m_fGlobalLightScalar * m_fLightScale);

                // setup self-shadow
                if (AdjustSelfShadow(pBaseTree->BranchesReceiveShadows( )))
                {
#if IDV_OPENGL & !PS3
                    // Cg 1.4 (PC) requires this in order for a texture bind to take effect.  It's supposedly 
                    // fixed in Cg 1.5, but Cg 1.5 Beta 1 had an insurmountable bug in it at the time of this release.
                    (void) m_cBranchTechnique.Begin( );
#endif
                }

                // render instances of this base
                int nBranchLevel = 0;
                float fBranchAlpha = 0.0f;
                const CSpeedTreeInstance* pTree = m_cCullEngine.First3dTree(nBaseIndex);
                while (pTree)
                {
                    pTree->GetBranchLod(nBranchLevel, fBranchAlpha);
                    st_assert(nBranchLevel > -1);

                    CIdvState::SetAlphaFunction(CIdvState::GREATER, fBranchAlpha);

                    (void) m_conTreePos.Set3fv(pTree->Pos( ));
                    (void) m_conTreeRotationTrig.Set4fv(pTree->RotationTrigValues( ));
                    (void) m_conTreeScale.Set1f(pTree->Scale( ));
                    (void) m_conWindMatrixOffset.Set1f(pTree->WindMatrixOffset( ));
#ifdef INTERMEDIATE_FRONDBRANCH_FADING
                    (void) m_conBranchLodValues.Set2f(pTree->GetLod( ), pBaseTree->BranchFadeDistance( ));
#endif
#ifndef LOAD_BINARY_SHADERS
                    (void) m_cEffect.CommitChanges( );
#endif
                    (void) m_cBranchGeometry.DrawIndexed(CIdvGeometryBuffer::TRIANGLE_STRIP,
                                                         pDrawData->m_pBranchIndexDataStart[nBranchLevel], 
                                                         pDrawData->m_pBranchStripLengths[nBranchLevel]);
                    m_sDrawStats.m_nNumDrawCalls++;
                    m_sDrawStats.m_nNumTrianglesDrawn += pDrawData->m_pBranchStripLengths[nBranchLevel] - 2;
                    m_sDrawStats.m_nNum3DTrees++;

                    pTree = m_cCullEngine.Next3dTree( );
                }
            }
        }

        if (bContextBound)
        {
            m_cBranchGeometry.DisableFormat( );
            (void) m_cBranchTechnique.End( );
            m_sDrawStats.m_nBufferBinds++;
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::DrawFronds

void CSpeedTreeForest::DrawFronds(void)
{
    bool bContextBound = false;

    if (!m_cFrondGeometry.Empty( ))
    {
        int nNumBaseTrees = int(m_vBaseTrees.size( ));
        for (int nBaseIndex = 0; nBaseIndex < nNumBaseTrees; ++nBaseIndex)
        {
            const CSpeedTreeWrapper* pBaseTree = m_vBaseTrees[nBaseIndex];
            if (pBaseTree->HasFrondGeometry( ) &&
                m_cCullEngine.First3dTree(nBaseIndex))
            {
                if (!bContextBound)
                {
                    m_sDrawStats.m_nBufferBinds++;
                    (void) m_cFrondTechnique.Begin( );
                    (void) m_cFrondGeometry.EnableFormat( );
                    (void) m_cFrondGeometry.BindVertexBuffer( );
                    (void) m_cFrondGeometry.BindIndexBuffer( );
                    bContextBound = true;
                }

                const CSpeedTreeWrapper::SDrawData* pDrawData = &pBaseTree->GetDrawData( );

                // upload material
                (void) m_conAmbientMaterial.Set3fv(pBaseTree->GetFrondMaterial( ).Ambient( ));
                (void) m_conDiffuseMaterial.Set3fv(pBaseTree->GetFrondMaterial( ).Diffuse( ));
                (void) m_conDiffuseScale.Set1f(pBaseTree->GetLightScales( ).m_fFrondLightScalar * pBaseTree->GetLightScales( ).m_fGlobalLightScalar * m_fLightScale);

                // setup self-shadow
                if (AdjustSelfShadow(pBaseTree->FrondsReceiveShadows( )))
                {
#if IDV_OPENGL & !PS3
                    // Cg 1.4 (PC) requires this in order for a texture bind to take effect.  It's supposedly 
                    // fixed in Cg 1.5, but Cg 1.5 Beta 1 had an insurmountable bug in it at the time of this release.
                    (void) m_cFrondTechnique.Begin( );
#endif
                }

                // render instances of this base
                int nFrondLevel = 0;
                float fFrondAlpha = 0.0f;
                const CSpeedTreeInstance* pTree = m_cCullEngine.First3dTree(nBaseIndex);

                while (pTree)
                {
                    pTree->GetFrondLod(nFrondLevel, fFrondAlpha);
                    st_assert(nFrondLevel > -1);

                    CIdvState::SetAlphaFunction(CIdvState::GREATER, fFrondAlpha);

                    (void) m_conTreePos.Set3fv(pTree->Pos( ));
                    (void) m_conTreeRotationTrig.Set4fv(pTree->RotationTrigValues( ));
                    (void) m_conTreeScale.Set1f(pTree->Scale( ));
                    (void) m_conWindMatrixOffset.Set1f(pTree->WindMatrixOffset( ));
#ifdef INTERMEDIATE_FRONDBRANCH_FADING
                    (void) m_conBranchLodValues.Set2f(pTree->GetLod( ), pBaseTree->FrondFadeDistance( ));
#endif
#ifndef LOAD_BINARY_SHADERS
                    (void) m_cEffect.CommitChanges( );
#endif

                    (void) m_cFrondGeometry.DrawIndexed(CIdvGeometryBuffer::TRIANGLE_STRIP, 
                                                        pDrawData->m_pFrondIndexDataStart[nFrondLevel], 
                                                        pDrawData->m_pFrondStripLengths[nFrondLevel]);
                    m_sDrawStats.m_nNumDrawCalls++;
                    m_sDrawStats.m_nNumTrianglesDrawn += pDrawData->m_pFrondStripLengths[nFrondLevel] - 2;

                    pTree = m_cCullEngine.Next3dTree( );
                }
            }
        }

        if (bContextBound)
        {
            m_cFrondGeometry.DisableFormat( );
            (void) m_cFrondTechnique.End( );
            m_sDrawStats.m_nBufferBinds++;
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::DrawLeafCards

void CSpeedTreeForest::DrawLeafCards(void)
{
    bool bContextBound = false;

    if (m_cLeafCardGeometry.NumVertices( ) > 0)
    {
        CIdvGeometryBuffer::UnBindIndexBuffer( );

        int nNumBaseTrees = int(m_vBaseTrees.size( ));
        for (int nBaseIndex = 0; nBaseIndex < nNumBaseTrees; ++nBaseIndex)
        {
            const CSpeedTreeWrapper* pBaseTree = m_vBaseTrees[nBaseIndex];
            if (pBaseTree->HasLeafCardGeometry( ) &&
                m_cCullEngine.First3dTree(nBaseIndex))
            {
                if (!bContextBound)
                {
                    m_sDrawStats.m_nBufferBinds++;
                    (void) m_cLeafCardTechnique.Begin( );
                    (void) m_cLeafCardGeometry.EnableFormat( );
                    (void) m_cLeafCardGeometry.BindVertexBuffer( );
                    UploadBillboardData(true);
                    UploadLeafWindAngles(m_conLeafCardAngles);
                    (void) m_conLeafLightingAdjust.Set1f(pBaseTree->GetLeafLightingAdjustment( ));
                    bContextBound = true;
                }

                const CSpeedTreeWrapper::SDrawData* pDrawData = &pBaseTree->GetDrawData( );

                // upload material
                (void) m_conAmbientMaterial.Set3fv(pBaseTree->GetLeafMaterial( ).Ambient( ));
                (void) m_conDiffuseMaterial.Set3fv(pBaseTree->GetLeafMaterial( ).Diffuse( ));
                (void) m_conDiffuseScale.Set1f(pBaseTree->GetLightScales( ).m_fLeafLightScalar * pBaseTree->GetLightScales( ).m_fGlobalLightScalar * m_fLightScale);

                // upload leaf wind scalars
                (void) m_conLeafCardAngleScalars.Set2f(pBaseTree->GetLeafRockScalar( ), pBaseTree->GetLeafRustleScalar( ));

                // render instances of this base
                int anLeafLevels[2];
                float afLeafAlphas[2];
                const CSpeedTreeInstance* pTree = m_cCullEngine.First3dTree(nBaseIndex);
                while (pTree)
                {
                    pTree->GetLeafLod(anLeafLevels, afLeafAlphas);

                    (void) m_conTreePos.Set3fv(pTree->Pos( ));
                    (void) m_conTreeScale.Set1f(pTree->Scale( ));
                    (void) m_conWindMatrixOffset.Set1f(pTree->WindMatrixOffset( ));
                    (void) m_conTreeRotationTrig.Set4fv(pTree->RotationTrigValues( ));
#ifndef LOAD_BINARY_SHADERS
                    (void) m_cEffect.CommitChanges( );
#endif

                    // leaves 0
                    if (anLeafLevels[0] > -1)
                    {
                        CIdvState::SetAlphaFunction(CIdvState::GREATER, afLeafAlphas[0]);

#if IDV_OPENGL | _XBOX
                        (void) m_cLeafCardGeometry.DrawArrays(CIdvGeometryBuffer::QUADS, pDrawData->m_pLeafCardVertexDataStart[anLeafLevels[0]], pDrawData->m_pLeafCardVertexDataLengths[anLeafLevels[0]]);
#elif IDV_DIRECTX9
                        (void) m_cLeafCardGeometry.DrawArrays(CIdvGeometryBuffer::TRIANGLES, pDrawData->m_pLeafCardVertexDataStart[anLeafLevels[0]], pDrawData->m_pLeafCardVertexDataLengths[anLeafLevels[0]]);
#endif
                        m_sDrawStats.m_nNumDrawCalls++;
                        m_sDrawStats.m_nNumTrianglesDrawn += pDrawData->m_pLeafCardVertexDataLengths[anLeafLevels[0]] / 2;
                    }

                    // leaves 1
                    if (anLeafLevels[1] > -1)
                    {
                        CIdvState::SetAlphaFunction(CIdvState::GREATER, afLeafAlphas[1]);

#if IDV_OPENGL | _XBOX
                        (void) m_cLeafCardGeometry.DrawArrays(CIdvGeometryBuffer::QUADS, pDrawData->m_pLeafCardVertexDataStart[anLeafLevels[1]], pDrawData->m_pLeafCardVertexDataLengths[anLeafLevels[1]]);
#elif IDV_DIRECTX9
                        (void) m_cLeafCardGeometry.DrawArrays(CIdvGeometryBuffer::TRIANGLES, pDrawData->m_pLeafCardVertexDataStart[anLeafLevels[1]], pDrawData->m_pLeafCardVertexDataLengths[anLeafLevels[1]]);
#endif
                        m_sDrawStats.m_nNumDrawCalls++;
                        m_sDrawStats.m_nNumTrianglesDrawn += pDrawData->m_pLeafCardVertexDataLengths[anLeafLevels[1]] / 2;
                    }

                    pTree = m_cCullEngine.Next3dTree( );
                }
            }
        }

        if (bContextBound)
        {
            m_cLeafCardGeometry.DisableFormat( );
            (void) m_cLeafCardTechnique.End( );
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::DrawLeafMeshes

void CSpeedTreeForest::DrawLeafMeshes(void)
{
    bool bContextBound = false;

    if (m_cLeafMeshGeometry.NumVertices( ) > 0)
    {
        // render leaf meshes
        int nNumBaseTrees = int(m_vBaseTrees.size( ));
        for (int nBaseIndex = 0; nBaseIndex < nNumBaseTrees; ++nBaseIndex)
        {
            const CSpeedTreeWrapper* pBaseTree = m_vBaseTrees[nBaseIndex];
            if (pBaseTree->HasLeafMeshGeometry( ) &&
                m_cCullEngine.First3dTree(nBaseIndex))
            {
                if (!bContextBound)
                {
                    m_sDrawStats.m_nBufferBinds++;
                    (void) m_cLeafMeshTechnique.Begin( );
                    (void) m_cLeafMeshGeometry.EnableFormat( );
                    (void) m_cLeafMeshGeometry.BindVertexBuffer( );
                    (void) m_cLeafMeshGeometry.BindIndexBuffer( );
                    UploadLeafWindAngles(m_conLeafMeshAngles);
                    (void) m_conLeafLightingAdjust.Set1f(pBaseTree->GetLeafLightingAdjustment( ));
                    bContextBound = true;
                }

                const CSpeedTreeWrapper::SDrawData* pDrawData = &pBaseTree->GetDrawData( );

                // upload material
                (void) m_conAmbientMaterial.Set3fv(pBaseTree->GetLeafMaterial( ).Ambient( ));
                (void) m_conDiffuseMaterial.Set3fv(pBaseTree->GetLeafMaterial( ).Diffuse( ));
                (void) m_conDiffuseScale.Set1f(pBaseTree->GetLightScales( ).m_fLeafLightScalar * pBaseTree->GetLightScales( ).m_fGlobalLightScalar * m_fLightScale);

                // upload leaf wind scalars
                (void) m_conLeafMeshAngleScalars.Set2f(pBaseTree->GetLeafRockScalar( ), pBaseTree->GetLeafRustleScalar( ));

                // render instances of this base
                int anLeafLevels[2];
                float afLeafAlphas[2];
                const CSpeedTreeInstance* pTree = m_cCullEngine.First3dTree(nBaseIndex);
                while (pTree)
                {
                    pTree->GetLeafLod(anLeafLevels, afLeafAlphas);

                    (void) m_conTreePos.Set3fv(pTree->Pos( ));
                    (void) m_conTreeScale.Set1f(pTree->Scale( ));
                    (void) m_conWindMatrixOffset.Set1f(pTree->WindMatrixOffset( ));
                    (void) m_conTreeRotationTrig.Set4fv(pTree->RotationTrigValues( ));
#ifndef LOAD_BINARY_SHADERS
                    (void) m_cEffect.CommitChanges( );
#endif

                    // leaves 0
                    if (anLeafLevels[0] > -1)
                    {
                        CIdvState::SetAlphaFunction(CIdvState::GREATER, afLeafAlphas[0]);

                        (void) m_cLeafMeshGeometry.DrawIndexed(CIdvGeometryBuffer::TRIANGLES, 
                                                               pDrawData->m_pLeafMeshIndexDataStart[anLeafLevels[0]], 
                                                               pDrawData->m_pLeafMeshTriListLengths[anLeafLevels[0]]);
                        m_sDrawStats.m_nNumDrawCalls++;
                        m_sDrawStats.m_nNumTrianglesDrawn += pDrawData->m_pLeafMeshTriListLengths[anLeafLevels[0]] / 2;
                    }

                    // leaves 1
                    if (anLeafLevels[1] > -1)
                    {
                        CIdvState::SetAlphaFunction(CIdvState::GREATER, afLeafAlphas[1]);

                        (void) m_cLeafMeshGeometry.DrawIndexed(CIdvGeometryBuffer::TRIANGLES, 
                                                               pDrawData->m_pLeafMeshIndexDataStart[anLeafLevels[1]], 
                                                               pDrawData->m_pLeafMeshTriListLengths[anLeafLevels[1]]);
                        m_sDrawStats.m_nNumDrawCalls++;
                        m_sDrawStats.m_nNumTrianglesDrawn += pDrawData->m_pLeafMeshTriListLengths[anLeafLevels[1]] / 2;
                    }

                    pTree = m_cCullEngine.Next3dTree( );
                }
            }
        }

        if (bContextBound)
        {
            m_cLeafMeshGeometry.DisableFormat( );
            (void) m_cLeafMeshTechnique.End( );
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::DrawVertBillboards

void CSpeedTreeForest::DrawVertBillboards(int nPass)
{
    int nNumBBs = 0;
    const CIdvInterleavedBuffer* pBuffer = m_cCullEngine.FirstBillboardCell(nNumBBs);

    if (pBuffer)
    {
        // setup a few constants for leaf billboarding
        UploadBillboardData(false, nPass);

        // bind the technique
        (void) m_cBillboardTechniques[nPass].Begin( );

        // setup static alpha test value
        CIdvState::SetAlphaFunction(CIdvState::GREATER, 84.0f);

        // render billboards, cell by cell
        const CIdvInterleavedBuffer* pFirstBuffer = NULL;
        while (pBuffer)
        {
            // draw billboards
            if (nNumBBs > 0)
            {
                if (!pFirstBuffer)
                {
                    pFirstBuffer = pBuffer;
                    // all billboard objects shared the same vertex format, so only enable it once
                    pFirstBuffer->EnableFormat( );
                }

                // each set of billboards is on a different buffer object
                (void) pBuffer->BindVertexBuffer( );
#if IDV_OPENGL | _XBOX
                (void) pBuffer->DrawArrays(CIdvInterleavedBuffer::QUADS, 0, nNumBBs * 4);
#endif
#ifdef IDV_DIRECTX9
                (void) pBuffer->DrawArrays(CIdvInterleavedBuffer::TRIANGLES, 0, nNumBBs * 6);
#endif

                if (nPass == 0)
                    m_sDrawStats.m_nNumBBs += nNumBBs;
                m_sDrawStats.m_nNumDrawCalls++;
                m_sDrawStats.m_nNumTrianglesDrawn += 2 * nNumBBs;
            }

            pBuffer = m_cCullEngine.NextBillboardCell(nNumBBs);
        }

        if (pFirstBuffer)
        {
            pFirstBuffer->DisableFormat( );
            CIdvGeometryBuffer::UnBindVertexBuffer( );
        }

        (void) m_cBillboardTechniques[nPass].End( );
    }
}


#ifdef HORZ_BILLBOARDS
///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::DrawHorzBillboards

void CSpeedTreeForest::DrawHorzBillboards(void)
{
    // how much are the horz bb's faded in
    const float c_fHorzFadeValue = CSpeedTreeRT::GetHorzBillboardFadeValue( );

    // no need to proceed if they're not active
    if (c_fHorzFadeValue < 0.01f)
        return;

    int nNumBBs = 0;
    const CIdvInterleavedBuffer* pBuffer = m_cCullEngine.FirstBillboardCell(nNumBBs);

    if (pBuffer)
    {
        // bind the technique
        m_cHorzBillboardTechnique.Begin( );

        // setup static alpha test value
        CIdvState::AlphaFunction(CIdvState::GREATER, 84.0f);

        // pass in global horizontal fade value - this value can be tuned by calling
        // CSpeedTreeRT::SetHorzBillboardFadeAngles()
        m_conHorzBillboardFadeValue.Set1f(1.0f - c_fHorzFadeValue);

        // render billboards, cell by cell
        const CIdvInterleavedBuffer* pFirstBuffer = NULL;
        while (pBuffer)
        {
            // draw billboards
            if (nNumBBs > 0)
            {
                if (!pFirstBuffer)
                {
                    pFirstBuffer = pBuffer;
                    // all billboard objects shared the same vertex format, so only enable it once
                    pFirstBuffer->EnableFormat( );
                }

                // each set of billboards is on a different buffer object
                pBuffer->BindVertexBuffer( );
#if IDV_OPENGL | _XBOX
                int nOffset = nNumBBs * 4;
                pBuffer->DrawArrays(CIdvInterleavedBuffer::QUADS, nOffset, nOffset);
#endif
#ifdef IDV_DIRECTX9
                int nOffset = nNumBBs * 6;
                pBuffer->DrawArrays(CIdvInterleavedBuffer::TRIANGLES, nOffset, nOffset);
#endif

                m_sDrawStats.m_nNumDrawCalls++;
                m_sDrawStats.m_nNumTrianglesDrawn += 2 * nNumBBs;
            }

            pBuffer = m_cCullEngine.NextBillboardCell(nNumBBs);
        }

        if (pFirstBuffer)
        {
            pFirstBuffer->DisableFormat( );
            CIdvGeometryBuffer::UnBindVertexBuffer( );
        }

        m_cHorzBillboardTechnique.End( );
    }
}
#endif

///////////////////////////////////////////////////////////////////////  
//  DrawBox (debug)

#if WIN32 & IDV_OPENGL
void DrawBox(const float* pMin, const float* pMax)
{
    glBegin(GL_QUADS);
    // -z
    glVertex3f(pMin[0], pMin[1], pMin[2]);
    glVertex3f(pMin[0], pMax[1], pMin[2]);
    glVertex3f(pMax[0], pMax[1], pMin[2]);
    glVertex3f(pMax[0], pMin[1], pMin[2]);

    // +z
    glVertex3f(pMin[0], pMin[1], pMax[2]);
    glVertex3f(pMax[0], pMin[1], pMax[2]);
    glVertex3f(pMax[0], pMax[1], pMax[2]);
    glVertex3f(pMin[0], pMax[1], pMax[2]);

    // -x
    glVertex3f(pMin[0], pMin[1], pMin[2]);
    glVertex3f(pMin[0], pMin[1], pMax[2]);
    glVertex3f(pMin[0], pMax[1], pMax[2]);
    glVertex3f(pMin[0], pMax[1], pMin[2]);

    // +x
    glVertex3f(pMax[0], pMin[1], pMin[2]);
    glVertex3f(pMax[0], pMax[1], pMin[2]);
    glVertex3f(pMax[0], pMax[1], pMax[2]);
    glVertex3f(pMax[0], pMin[1], pMax[2]);

    // -y
    glVertex3f(pMin[0], pMin[1], pMin[2]);
    glVertex3f(pMax[0], pMin[1], pMin[2]);
    glVertex3f(pMax[0], pMin[1], pMax[2]);
    glVertex3f(pMin[0], pMin[1], pMax[2]);

    // +y
    glVertex3f(pMin[0], pMax[1], pMin[2]);
    glVertex3f(pMin[0], pMax[1], pMax[2]);
    glVertex3f(pMax[0], pMax[1], pMax[2]);
    glVertex3f(pMax[0], pMax[1], pMin[2]);

    glEnd( );
}
#endif


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::DrawCullRadii

void CSpeedTreeForest::DrawCullRadii(void)
{
    // currently only work on PC OpenGL
#if WIN32 & IDV_OPENGL
    // generate a sphere (first time through only)
    static GLuint uiSphereDL = 0;
    if (!uiSphereDL)
    {
        uiSphereDL = glGenLists(1);
        glNewList(uiSphereDL, GL_COMPILE);
            GLUquadricObj* pQuadric = gluNewQuadric( );
            gluSphere(pQuadric, 1.0f, 10, 10);
        glEndList( );
    }

    // setup render state for blending
    glPushAttrib(GL_ENABLE_BIT);
    glDisable(GL_VERTEX_PROGRAM_ARB);
    glDisable(GL_FRAGMENT_PROGRAM_ARB);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);

    // render each of the trees
    for (int nBase = 0; nBase < int(m_vBaseTrees.size( )); ++nBase)
    {
        float afBox[6];
        m_vBaseTrees[nBase]->GetBoundingBox(afBox);

        const CSpeedTreeInstance* pTree = m_cCullEngine.First3dTree(nBase);

        while (pTree)
        {
            idv::Vec3 cPos(pTree->Pos( ));

            // draw radius line from center to top
            //idv::Vec3 cCenter = (idv::Vec3(afBox + 0) + idv::Vec3(afBox + 3)) * 0.5f + cPos;
            idv::Vec3 cCenter(pTree->Pos( )[0], pTree->Pos( )[1], pTree->UpAxisCenter( ));
            float fRadius = pTree->InstanceOf( )->CullRadius( ) * pTree->Scale( );
            glColor3f(1.0f, 0.0f, 0.0f);
            glBegin(GL_LINES);
                glVertex3fv(cCenter);
#ifdef UPVECTOR_POS_Y
                glVertex3f(cCenter[0], cCenter[1] + fRadius, cCenter[2]);
#else
                glVertex3f(cCenter[0], cCenter[1], cCenter[2] + fRadius);
#endif
            glEnd( );

            // draw bounding box
            //glPushMatrix( );
            //  glColor4f(1.0f, 1.0f, 0.0f, 0.4f);
            //  glTranslatef(cPos[0], cPos[1], cPos[2]);
            //  float afMin[3] = { afBox[0] * pTree->Scale( ), afBox[1] * pTree->Scale( ), afBox[2] * pTree->Scale( ) } ;
            //  float afMax[3] = { afBox[3] * pTree->Scale( ), afBox[4] * pTree->Scale( ), afBox[5] * pTree->Scale( ) } ;
            //  DrawBox(afMin, afMax);
            //glPopMatrix( );

            // draw sphere
            //glPushMatrix( );
            //  glColor4f(0.0f, 1.0f, 0.0f, 0.4f);
            //  idv::Vec3 cSphereCenter = cPos;//(idv::Vec3(afBox + 0) + idv::Vec3(afBox + 3)) * 0.5f + cPos;
            //  glTranslatef(cSphereCenter[0], cSphereCenter[1], cSphereCenter[2]);
            //  float fSphereRadius = pTree->InstanceOf( )->CullRadius( ) * pTree->Scale( );
            //  glScalef(fSphereRadius, fSphereRadius, fSphereRadius);
            //  glCallList(uiSphereDL);
            //glPopMatrix( );

            pTree = m_cCullEngine.Next3dTree( );
        }
    }

    // restore render stats
    glPopAttrib( );
#endif
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::LoadTextures

void CSpeedTreeForest::LoadTextures(const st_string& strTexturePath)
{
    st_string strCompositeDiffuseLeaf, strCompositeNormalLeaf, astrCompositeBB[2], strCompositeShadow;
    ResolveCompositeTextureFilenames(strTexturePath, strCompositeDiffuseLeaf, strCompositeNormalLeaf, astrCompositeBB, strCompositeShadow);

    // load leaf composite diffuse map
    if (!m_texLeavesAndFronds[CSpeedTreeRT::TL_DIFFUSE].Load(strCompositeDiffuseLeaf))
        IdvWarning("Failed to load leaf & frond diffuse composite texture [%s]", strCompositeDiffuseLeaf.c_str( ));

#if defined(FROND_NORMAL_MAPPING) || defined(LEAF_NORMAL_MAPPING)
    // load leaf composite normal map
    if (!m_texLeavesAndFronds[CSpeedTreeRT::TL_NORMAL].Load(strCompositeNormalLeaf))
        IdvWarning("Failed to load leaf & frond normal composite texture [%s]", strCompositeNormalLeaf.c_str( ));
#endif

    // load diffuse billboard composite map
    if (!m_texBillboards[CSpeedTreeRT::TL_DIFFUSE].Load(astrCompositeBB[0], CIdvTexture::FILTER_NONE, CIdvTexture::FILTER_NONE))
        IdvWarning("Failed to load billboard diffuse composite texture [%s]", astrCompositeBB[0].c_str( ));

    // load normal-map billboard composite map
    if (!m_texBillboards[CSpeedTreeRT::TL_NORMAL].Load(astrCompositeBB[1], CIdvTexture::FILTER_NONE, CIdvTexture::FILTER_NONE))
        IdvWarning("Failed to load billboard normal-map composite texture [%s]", astrCompositeBB[1].c_str( ));

    // load self-shadow composite map
    if (!m_texSelfShadow.Load(strCompositeShadow))
        IdvWarning("Failed to load self-shadow composite texture [%s]", strCompositeShadow.c_str( ));

    // load white map to use when layers are missing (like the branch detail layer) 
    (void) m_texWhite.LoadWhite( ); // don't need to warn if failure
    (void) m_texNoDetail.LoadSolidRGBATexture("no_detail", 0xffffff00); // don't need to warn if failure
    (void) m_texFlatNormalMap.LoadSolidRGBATexture("flat_normal_map", 0x7f7fffff);
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::LoadShaders
//
//  Shader loading is handled completely differently on PS3.

#ifndef LOAD_BINARY_SHADERS
bool CSpeedTreeForest::LoadShaders(const CSpeedWind& cWind)
{
    bool bSuccess = false;

    // setup shader #defines - vector is formatted in pairs of STL strings - first of pair is macro name, second is macro value
    st_vector_string vShaderDefines;
#ifdef IDV_DIRECTX9
    vShaderDefines.push_back("IDV_DIRECTX9");
    vShaderDefines.push_back("Doesn't matter");
#endif
#ifdef BRANCH_DETAIL_LAYER
    vShaderDefines.push_back("BRANCH_DETAIL_LAYER");
    vShaderDefines.push_back("Doesn't matter");
#endif
#ifdef UPVECTOR_POS_Y
    vShaderDefines.push_back("UPVECTOR_POS_Y");
    vShaderDefines.push_back("Doesn't matter");
#endif
    if (m_fFogStart != -1.0f && m_fFogEnd != -1.0f)
    {
        vShaderDefines.push_back("USE_FOG");
        vShaderDefines.push_back("Doesn't matter");
    }
    vShaderDefines.push_back("SELF_SHADOW_LAYER");
    vShaderDefines.push_back("Doesn't matter");

    vShaderDefines.push_back("BILLBOARD_NORMAL_MAPPING");
    vShaderDefines.push_back("Doesn't matter");

#ifdef HORZ_BILLBOARDS
    vShaderDefines.push_back("HORZ_BILLBOARDS");
    vShaderDefines.push_back("Doesn't matter");
#endif
#ifdef BRANCH_NORMAL_MAPPING
    vShaderDefines.push_back("BRANCH_NORMAL_MAPPING");
    vShaderDefines.push_back("Doesn't matter");
#endif
#ifdef FROND_NORMAL_MAPPING
    vShaderDefines.push_back("FROND_NORMAL_MAPPING");
    vShaderDefines.push_back("Doesn't matter");
#endif
#ifdef LEAF_NORMAL_MAPPING
    vShaderDefines.push_back("LEAF_NORMAL_MAPPING");
    vShaderDefines.push_back("Doesn't matter");
#endif
#ifdef INTERMEDIATE_FRONDBRANCH_FADING
    vShaderDefines.push_back("INTERMEDIATE_FRONDBRANCH_FADING");
    vShaderDefines.push_back("Doesn't matter");
#endif

    vShaderDefines.push_back("NUM_WIND_MATRICES");
    vShaderDefines.push_back(IdvFormatString("%d", cWind.GetNumWindMatrices( )));
    printf("Number of wind matrices: %d\n", cWind.GetNumWindMatrices( ));

    vShaderDefines.push_back("MAX_NUM_LEAF_ANGLES");
    vShaderDefines.push_back(IdvFormatString("%d", cWind.GetNumLeafAngleMatrices( )));

    // find the number of 360 billboard images used by all base trees
    vShaderDefines.push_back("NUM_360_IMAGES");
    int nNum360TexCoords = st_max(4, int(m_v360TexCoordTable.size( )));
    st_assert(nNum360TexCoords % 4 == 0);
    vShaderDefines.push_back(IdvFormatString("%d", nNum360TexCoords / 4));
    printf("\n[%d] 360 billboards found\n", nNum360TexCoords / 4);

    // take a guess at whether 360 billboards should be supported
    m_b360BillboardsSupported = (nNum360TexCoords / 4) / int(m_vBaseTrees.size( )) > 1;
    if (m_b360BillboardsSupported)
    {
        vShaderDefines.push_back("SUPPORT_360_BILLBOARDS");
        vShaderDefines.push_back("Doesn't matter");
    }
    
    // load shader
    st_string strShaderError = m_cEffect.Load(m_strMainShaderFile, vShaderDefines);
    if (strShaderError.empty( ))
    {
        // find techniques
        m_cBranchTechnique = m_cEffect.GetTechnique("Branches");
        m_cFrondTechnique = m_cEffect.GetTechnique("Fronds");
        m_cLeafCardTechnique = m_cEffect.GetTechnique("LeafCards");
        m_cLeafMeshTechnique = m_cEffect.GetTechnique("LeafMeshes");
        m_cBillboardTechniques[0] = m_cEffect.GetTechnique("Billboards1");
        m_cBillboardTechniques[1] = m_cEffect.GetTechnique("Billboards2");
        m_cOverlayTechnique = m_cEffect.GetTechnique("Overlay");
        st_assert(m_cBranchTechnique.IsValid( ));
        st_assert(m_cFrondTechnique.IsValid( ));
        st_assert(m_cLeafCardTechnique.IsValid( ));
        st_assert(m_cLeafMeshTechnique.IsValid( ));
        st_assert(m_cBillboardTechniques[0].IsValid( ));
        st_assert(m_cBillboardTechniques[1].IsValid( ));
        st_assert(m_cOverlayTechnique.IsValid( ));
#ifdef HORZ_BILLBOARDS
        m_cHorzBillboardTechnique = m_cEffect.GetTechnique("HorzBillboards");
        st_assert(m_cHorzBillboardTechnique.IsValid( ));
#endif

        // find constants
        m_conWorldViewProjectionMatrix = m_cEffect.GetConstant("g_mModelViewProj");
        m_conTreePos = m_cEffect.GetConstant("g_vTreePos");
        m_conTreeRotationTrig = m_cEffect.GetConstant("g_vTreeRotationTrig");
        m_conTreeScale = m_cEffect.GetConstant("g_fTreeScale");
        m_conWindMatrices = m_cEffect.GetConstant("g_amWindMatrices");
        m_conWindMatrixOffset = m_cEffect.GetConstant("g_fWindMatrixOffset");
        m_conLightDir = m_cEffect.GetConstant("g_vLightDir");
        m_conLightColor = m_cEffect.GetConstant("g_vLightDiffuse");
        m_conLightColor_PS = m_cEffect.GetConstant("g_vLightDiffuse");
        m_conAmbientMaterial = m_cEffect.GetConstant("g_vMaterialAmbient");
        m_conDiffuseMaterial = m_cEffect.GetConstant("g_vMaterialDiffuse");
        m_conAmbientScale = m_cEffect.GetConstant("g_fAmbientScale");
        m_conDiffuseScale = m_cEffect.GetConstant("g_fDiffuseScale");
#ifdef IDV_OPENGL
        m_conSelfShadowMap = m_cEffect.GetConstant("samSelfShadowMap");
        m_conDiffuseCompositeLeafMap = m_cEffect.GetConstant("samCompositeDiffuseLeafMap");
        m_conNormalCompositeLeafMap = m_cEffect.GetConstant("samCompositeNormalLeafMap");
#else
        m_conSelfShadowMap = m_cEffect.GetConstant("g_tSelfShadowMap");
        m_conDiffuseCompositeLeafMap = m_cEffect.GetConstant("g_tCompositeDiffuseLeafMap");
        m_conNormalCompositeLeafMap = m_cEffect.GetConstant("g_tCompositeNormalLeafMap");
#endif
        m_conCameraAngles = m_cEffect.GetConstant("g_vCameraAngles");
        m_conCameraAzimuthTrig = m_cEffect.GetConstant("g_vCameraAzimuthTrig");
        m_conSelfShadowMotion = m_cEffect.GetConstant("g_vSelfShadowMotion");
        m_conFogColor = m_cEffect.GetConstant("g_vFogColor");
        m_conFogParams = m_cEffect.GetConstant("g_vFogParams");
#ifdef INTERMEDIATE_FRONDBRANCH_FADING
        m_conBranchLodValues = m_cEffect.GetConstant("g_vBranchLodValues");
#endif

        // branch-specific
#ifdef IDV_OPENGL
        m_conBranchDiffuseMap = m_cEffect.GetConstant("samBranchDiffuseMap");
        m_conBranchNormalMap = m_cEffect.GetConstant("samBranchNormalMap");
#ifdef BRANCH_DETAIL_LAYER
        m_conBranchDetailMap = m_cEffect.GetConstant("samBranchDetailMap");
#endif
#else // IDV_OPENGL
        m_conBranchDiffuseMap = m_cEffect.GetConstant("g_tBranchDiffuseMap");
        m_conBranchNormalMap = m_cEffect.GetConstant("g_tBranchNormalMap");
#ifdef BRANCH_DETAIL_LAYER
        m_conBranchDetailMap = m_cEffect.GetConstant("g_tBranchDetailMap");
#endif
#endif // IDV_OPENGL

        // leaf-specific
        m_conLeafCardAngles = m_cEffect.GetConstant("g_avLeafAngles");
        m_conLeafCardAngleScalars = m_cEffect.GetConstant("g_vLeafAngleScalars");
        m_conLeafMeshAngles = m_cEffect.GetConstant("g_avLeafAngles");
        m_conLeafMeshAngleScalars = m_cEffect.GetConstant("g_vLeafAngleScalars");
        m_conLeafLightingAdjust = m_cEffect.GetConstant("g_fLeafLightingAdjust");

        // billboard-specific
#ifdef IDV_OPENGL
        m_conBillboardDiffuseMap = m_cEffect.GetConstant("samBillboardDiffuseMap");
        m_conBillboardNormalMap = m_cEffect.GetConstant("samBillboardNormalMap");
#else
        m_conBillboardDiffuseMap = m_cEffect.GetConstant("g_tBillboardDiffuseMap");
        m_conBillboardNormalMap = m_cEffect.GetConstant("g_tBillboardNormalMap");
#endif
        m_con360TexCoords = m_cEffect.GetConstant("g_v360TexCoords");
        m_conLightAdjustment = m_cEffect.GetConstant("g_fBillboardLightAdjust");
        for (int nPass = 0; nPass < 2; ++nPass)
        {
            m_conSpecialAzimuth[nPass] = m_cEffect.GetConstant("g_fSpecialAzimuth");
            m_conBBNormals[nPass] = m_cEffect.GetConstant("g_amBBNormals");
            m_conBBBinormals[nPass] = m_cEffect.GetConstant("g_amBBBinormals");
            m_conBBTangents[nPass] = m_cEffect.GetConstant("g_amBBTangents");
        }
#ifdef HORZ_BILLBOARDS
        m_conHorzBillboardFadeValue = m_cEffect.GetConstant("g_fBillboardFadeValue");
#endif

        // overlay specific
        m_conOverlayColor = m_cEffect.GetConstant("g_vOverlayColor");

        bSuccess = true;
    }
    else
        IdvError("Failed to load FX file [%s]: %s\n", m_strMainShaderFile.c_str( ), strShaderError.c_str( ));

    return bSuccess;
}

#else // !LOAD_BINARY_SHADERS

// PLAYSTATION3 version
bool CSpeedTreeForest::LoadShaders(const CSpeedWind& )
{
    bool bSuccess = false;

    // load shaders
    st_string strPath = IdvPath(m_strMainShaderFile);

#ifdef IDV_OPENGL
#ifdef PS3
    strPath += "/PS3";
#else
    strPath += "/PC_OpenGL";
#endif
#endif

    printf("Load shaders from path [%s]\n", strPath.c_str( ));

    // branch
    st_string strError = m_cBranchTechnique.LoadBinary(strPath + "/Branch.vpo", strPath + "/Branch.fpo");

    // frond
    if (strError.empty( ))
        strError = m_cFrondTechnique.LoadBinary(strPath + "/Frond.vpo", strPath + "/Frond.fpo");

    // leaf cards
    if (strError.empty( ))
        strError = m_cLeafCardTechnique.LoadBinary(strPath + "/LeafCard.vpo", strPath + "/LeafCard.fpo");

    // leaf meshes
    if (strError.empty( ))
        strError = m_cLeafMeshTechnique.LoadBinary(strPath + "/LeafMesh.vpo", strPath + "/LeafMesh.fpo");

    // billboard1
    if (strError.empty( ))
        strError = m_cBillboardTechniques[0].LoadBinary(strPath + "/Billboard1.vpo", strPath + "/Billboard.fpo");

    // billboard2
    if (strError.empty( ))
        strError = m_cBillboardTechniques[1].LoadBinary(strPath + "/Billboard2.vpo", strPath + "/Billboard.fpo");
    m_b360BillboardsSupported = true;

    // overlay
    if (strError.empty( ))
        strError = m_cOverlayTechnique.LoadBinary(strPath + "/Overlay.vpo", strPath + "/Overlay.fpo");

    if (!strError.empty( ))
    {
        printf("failed to load shaders: %s\n", strError.c_str( ));
    }
    else
    {
        // shared parameters
        m_conWorldViewProjectionMatrix = m_cBranchTechnique.GetVertexConstant("g_mModelViewProj");
        m_conTreePos = m_cBranchTechnique.GetVertexConstant("g_vTreePos");
        m_conTreeRotationTrig = m_cBranchTechnique.GetVertexConstant("g_vTreeRotationTrig");
        m_conTreeScale = m_cBranchTechnique.GetVertexConstant("g_fTreeScale");
        m_conWindMatrices = m_cBranchTechnique.GetVertexConstant("g_amWindMatrices");
        m_conWindMatrixOffset = m_cBranchTechnique.GetVertexConstant("g_fWindMatrixOffset");
        m_conLightDir = m_cFrondTechnique.GetVertexConstant("g_vLightDir");
        m_conLightColor = m_cFrondTechnique.GetVertexConstant("g_vLightDiffuse");
        m_conLightColor_PS = m_cBranchTechnique.GetPixelConstant("g_vLightDiffuse");
        m_conAmbientMaterial = m_cFrondTechnique.GetPixelConstant("g_vMaterialAmbient");
        m_conDiffuseMaterial = m_cFrondTechnique.GetVertexConstant("g_vMaterialDiffuse");
        m_conAmbientScale = m_cFrondTechnique.GetPixelConstant("g_fAmbientScale");
        m_conDiffuseScale = m_cFrondTechnique.GetPixelConstant("g_fDiffuseScale");
        m_conSelfShadowMotion = m_cFrondTechnique.GetVertexConstant("g_vSelfShadowMotion");
#ifdef IDV_OPENGL
        m_conSelfShadowMap = m_cBranchTechnique.GetPixelConstant("samSelfShadowMap");
        m_conDiffuseCompositeLeafMap = m_cLeafCardTechnique.GetPixelConstant("samCompositeDiffuseLeafMap");
        m_conNormalCompositeLeafMap = m_cLeafCardTechnique.GetPixelConstant("samCompositeNormalLeafMap");
#else
        m_conSelfShadowMap = m_cBranchTechnique.GetPixelConstant("g_tSelfShadowMap");
        m_conDiffuseCompositeLeafMap = m_cFrondTechnique.GetPixelConstant("g_tCompositeDiffuseLeafMap");
        m_conNormalCompositeLeafMap = m_cFrondTechnique.GetPixelConstant("g_tCompositeNormalLeafMap");
#endif
        m_conFogColor = m_cBranchTechnique.GetPixelConstant("g_vFogColor");
        m_conFogParams = m_cBranchTechnique.GetVertexConstant("g_vFogParams");
#ifdef INTERMEDIATE_FRONDBRANCH_FADING
        m_conBranchLodValues = m_cBranchTechnique.GetVertexConstant("g_vBranchLodValues");
#endif

        // branch-specific
#ifdef IDV_OPENGL
        m_conBranchDiffuseMap = m_cBranchTechnique.GetPixelConstant("samBranchDiffuseMap");
        m_conBranchNormalMap = m_cBranchTechnique.GetPixelConstant("samBranchNormalMap");
#ifdef BRANCH_DETAIL_LAYER
        m_conBranchDetailMap = m_cBranchTechnique.GetPixelConstant("samBranchDetailMap");
#endif
#else // IDV_OPENGL
        m_conBranchDiffuseMap = m_cBranchTechnique.GetPixelConstant("g_tBranchDiffuseMap");
        m_conBranchNormalMap = m_cBranchTechnique.GetPixelConstant("g_tBranchNormalMap");
#ifdef BRANCH_DETAIL_LAYER
        m_conBranchDetailMap = m_cBranchTechnique.GetPixelConstant("g_tBranchDetailMap");
#endif
#endif // IDV_OPENGL

        // leaf-specific
        m_conLeafCardAngles = m_cLeafCardTechnique.GetVertexConstant("g_avLeafAngles");
        m_conLeafCardAngleScalars = m_cLeafCardTechnique.GetVertexConstant("g_vLeafAngleScalars");
        m_conLeafMeshAngles = m_cLeafMeshTechnique.GetVertexConstant("g_avLeafAngles");
        m_conLeafMeshAngleScalars = m_cLeafMeshTechnique.GetVertexConstant("g_vLeafAngleScalars");
        m_conLeafLightingAdjust = m_cLeafCardTechnique.GetVertexConstant("g_fLeafLightingAdjust");
        m_conCameraAngles = m_cLeafCardTechnique.GetVertexConstant("g_vCameraAngles");

        // billboard-specific
#ifdef IDV_OPENGL
        m_conBillboardDiffuseMap = m_cBillboardTechniques[0].GetPixelConstant("samBillboardDiffuseMap");
        m_conBillboardNormalMap = m_cBillboardTechniques[0].GetPixelConstant("samBillboardNormalMap");
#else
        m_conBillboardDiffuseMap = m_cBillboardTechniques[0].GetPixelConstant("g_tBillboardDiffuseMap");
        m_conBillboardNormalMap = m_cBillboardTechniques[0].GetPixelConstant("g_tBillboardNormalMap");
#endif
        m_con360TexCoords = m_cBillboardTechniques[0].GetVertexConstant("g_v360TexCoords");
        m_conCameraAzimuthTrig = m_cBillboardTechniques[0].GetVertexConstant("g_vCameraAzimuthTrig");
        m_conLightAdjustment = m_cBillboardTechniques[0].GetPixelConstant("g_fBillboardLightAdjust");
        for (int nPass = 0; nPass < 2; ++nPass)
        {
            m_conSpecialAzimuth[nPass] = m_cBillboardTechniques[nPass].GetVertexConstant("g_fSpecialAzimuth");
            m_conBBNormals[nPass] = m_cBillboardTechniques[nPass].GetVertexConstant("g_amBBNormals");
            m_conBBBinormals[nPass] = m_cBillboardTechniques[nPass].GetVertexConstant("g_amBBBinormals");
            m_conBBTangents[nPass] = m_cBillboardTechniques[nPass].GetVertexConstant("g_amBBTangents");
        }

        // overlay-specific
        m_conOverlayColor = m_cOverlayTechnique.GetPixelConstant("g_vOverlayColor");

        bSuccess = true;
    }

    return bSuccess;
}

#endif // !LOAD_BINARY_SHADERS


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::SetLighting

void CSpeedTreeForest::SetLighting(const float* pLightDir, const float* pLightColor, float fLightScale)
{
    memcpy(m_afLightDir, pLightDir, 3 * sizeof(float));
    memcpy(m_afLightColor, pLightColor, 3 * sizeof(float));
    m_fLightScale = fLightScale;
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::DebugSpin

void CSpeedTreeForest::DebugSpin(float fAngleInc)
{
    // only rotate the 3D trees
    int nNumBaseTrees = int(m_vBaseTrees.size( ));
    for (int nBaseIndex = 0; nBaseIndex < nNumBaseTrees; ++nBaseIndex)
    {
        CSpeedTreeInstance* pTree = m_cCullEngine.First3dTree(nBaseIndex);
        while (pTree)
        {
            float fAngle = pTree->Rotation( );
            fAngle += fAngleInc;
            if (fAngle >= 2.0f * idv::Pi)
                fAngle = 0.0f;
            if (fAngle < 0.0f)
                fAngle = 2.0f * idv::Pi;
            pTree->SetRotation(fAngle);

            pTree = m_cCullEngine.Next3dTree( );
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::AddBaseTree

void CSpeedTreeForest::AddBaseTree(CSpeedTreeWrapper* pTree)
{
    pTree->SetSortIndex(int(m_vBaseTrees.size( )));
    m_vBaseTrees.push_back(pTree);
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::UploadBillboardData

void CSpeedTreeForest::UploadBillboardData(bool bLeaves, int nPass)
{
    // query camera angles
    float fCameraAzimuth, fCameraPitch;
    CSpeedTreeRT::GetCameraAngles(fCameraAzimuth, fCameraPitch);

    // adjust azimuth from range [-180,180] to [0,360]
    fCameraAzimuth = idv::DegToRad(fCameraAzimuth);
    if (fCameraAzimuth < 0.0f)
        fCameraAzimuth += idv::TwoPi;

    // upload to leaf shader
    if (bLeaves)
    {
        (void) m_conCameraAngles.Set2f(fCameraAzimuth, idv::DegToRad(fCameraPitch));
    }
    // upload to billboard shader
    else
    {
        // query RT for camera data
        float afCameraPos[3], afCameraDir[3];
        CSpeedTreeRT::GetCamera(afCameraPos, afCameraDir);

        // prep azimuth for use in shader
        afCameraDir[0] = -afCameraDir[0];
        afCameraDir[1] = -afCameraDir[1];
        afCameraDir[2] = -afCameraDir[2];
#ifdef UPVECTOR_POS_Y
        float fBillboardAzimuth = atan2f(afCameraDir[2], -afCameraDir[0]);
#else
        float fBillboardAzimuth = atan2f(afCameraDir[1], afCameraDir[0]);
#endif
        if (fBillboardAzimuth < 0.0f)
            fBillboardAzimuth += idv::TwoPi;
        else if (fBillboardAzimuth >= idv::TwoPi)
            fBillboardAzimuth = fmod(fBillboardAzimuth, idv::TwoPi);

        // upload angles to shader
        (void) m_conCameraAngles.Set2f(fCameraAzimuth, idv::DegToRad(fCameraPitch));
        (void) m_conCameraAzimuthTrig.Set4f(sinf(-fCameraAzimuth), cosf(-fCameraAzimuth), -sinf(-fCameraAzimuth), 0.0f);
        (void) m_conSpecialAzimuth[nPass].Set1f(fBillboardAzimuth);

        // get updated billboard attributes
        CSpeedTreeRT::SGeometry::S360Billboard sBillboard;
        CSpeedTreeRT::UpdateBillboardLighting(sBillboard);

        // normals
        float afFourNormals[4][4] =
        {
            { sBillboard.m_pNormals[0], sBillboard.m_pNormals[1], sBillboard.m_pNormals[2], 0.0f },
            { sBillboard.m_pNormals[3], sBillboard.m_pNormals[4], sBillboard.m_pNormals[5], 0.0f },
            { sBillboard.m_pNormals[6], sBillboard.m_pNormals[7], sBillboard.m_pNormals[8], 0.0f },
            { sBillboard.m_pNormals[9], sBillboard.m_pNormals[10], sBillboard.m_pNormals[11], 0.0f },
        };
        (void) m_conBBNormals[nPass].SetMatrixTranspose((const float*) afFourNormals);

        // binormals
        float afFourBinormals[4][4] =
        {
            { sBillboard.m_pBinormals[0], sBillboard.m_pBinormals[1], sBillboard.m_pBinormals[2], 0.0f },
            { sBillboard.m_pBinormals[3], sBillboard.m_pBinormals[4], sBillboard.m_pBinormals[5], 0.0f },
            { sBillboard.m_pBinormals[6], sBillboard.m_pBinormals[7], sBillboard.m_pBinormals[8], 0.0f },
            { sBillboard.m_pBinormals[9], sBillboard.m_pBinormals[10], sBillboard.m_pBinormals[11], 0.0f },
        };
        (void) m_conBBBinormals[nPass].SetMatrixTranspose((const float*) afFourBinormals);

        // tangents
        float afFourTangents[4][4] =
        {
            { sBillboard.m_pTangents[0], sBillboard.m_pTangents[1], sBillboard.m_pTangents[2], 0.0f },
            { sBillboard.m_pTangents[3], sBillboard.m_pTangents[4], sBillboard.m_pTangents[5], 0.0f },
            { sBillboard.m_pTangents[6], sBillboard.m_pTangents[7], sBillboard.m_pTangents[8], 0.0f },
            { sBillboard.m_pTangents[9], sBillboard.m_pTangents[10], sBillboard.m_pTangents[11], 0.0f },
        };
        (void) m_conBBTangents[nPass].SetMatrixTranspose((const float*) afFourTangents);
    }
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::InitWind

void CSpeedTreeForest::InitWind(const st_string& strSpeedWindFile)
{
    if (!m_cSpeedWind.Load(strSpeedWindFile.c_str( )))
        IdvWarning("Failed to load SpeedWind file: [%s]", strSpeedWindFile.c_str( ));
    else
        printf("[%s] speedwind file successfully loaded\n", strSpeedWindFile.c_str( ));

    // if failure to load, proceed with CSpeedWind's default values
    st_assert(m_cSpeedWind.GetNumWindMatrices( ) > 0);
    st_assert(m_cSpeedWind.GetNumLeafAngleMatrices( ) > 0);

    m_vWindMatrices.resize(m_cSpeedWind.GetNumWindMatrices( ));
    m_vWindRustleAngles.resize(m_cSpeedWind.GetNumLeafAngleMatrices( ));
    m_vWindRockAngles.resize(m_cSpeedWind.GetNumLeafAngleMatrices( ));

    m_cSpeedWind.SetWindStrengthAndDirection(0.2f, 1.0f, 0.0f, 0.0f);
    for (int i = 0; i < int(m_vWindMatrices.size( )); ++i)
        m_vWindMatrices[i] = m_cSpeedWind.GetWindMatrix((unsigned int) i);
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::SetTime

void CSpeedTreeForest::SetTime(float fTime)
{
    float fElapsedTime = fTime - m_fTime;
    m_fTime = fTime;

    // query camera position from RT to pass into SpeedWind
    float afCameraPos[3], afCameraDir[3];
    CSpeedTreeRT::GetCamera(afCameraPos, afCameraDir);
    m_cSpeedWind.Advance(fTime, true, false, afCameraDir[0], afCameraDir[1], afCameraDir[2]);

    // query matrices to be uploaded during the render
    int nNumMatrices = int(m_cSpeedWind.GetNumWindMatrices( ));
    assert(int(m_vWindMatrices.size( )) == nNumMatrices);
    for (int i = 0; i < nNumMatrices; ++i)
        m_vWindMatrices[i] = m_cSpeedWind.GetWindMatrix((unsigned int) i);

    // query the rock/rustle angles
    st_assert(m_vWindRockAngles.size( ) == m_cSpeedWind.GetNumLeafAngleMatrices( ));
    st_assert(m_vWindRustleAngles.size( ) == m_cSpeedWind.GetNumLeafAngleMatrices( ));
    if (!m_vWindRockAngles.empty( ))
        (void) m_cSpeedWind.GetRockAngles(&m_vWindRockAngles[0]);
    if (!m_vWindRustleAngles.empty( ))
        (void) m_cSpeedWind.GetRustleAngles(&m_vWindRustleAngles[0]);

    // simulate self shadow motion based on wind strength (affects branch and frond geometry)
    const float c_fCurrentStrength = m_cSpeedWind.GetFinalStrength( );
    const float c_fExponent = 1.0f;
    const float c_fSpeed = 40.0f;
    m_fSelfShadowAdvance += (powf(c_fCurrentStrength, c_fExponent) * c_fSpeed) * fElapsedTime;

    const float c_fMotionDistance = 0.0015f;
    (void) m_conSelfShadowMotion.Set2f(c_fMotionDistance * sinf(m_fSelfShadowAdvance * 0.075f) * cosf(m_fSelfShadowAdvance * 0.05f), c_fMotionDistance * cosf(m_fSelfShadowAdvance * 0.1f));
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::ToggleWindStrength

void CSpeedTreeForest::ToggleWindStrength(void)
{
    float fStrength = 0.0f, afDir[3];
    m_cSpeedWind.GetWindStrengthAndDirection(fStrength, afDir[0], afDir[1], afDir[2]);

    if (fStrength == 1.0f)
        fStrength = 0.2f;
    else
        fStrength = 1.0f;

    m_cSpeedWind.SetWindStrengthAndDirection(fStrength, afDir[0], afDir[1], afDir[2]);
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::OverlayZoom

void CSpeedTreeForest::OverlayZoom(float fDirection)
{
    if (fDirection < 0.0f)
        m_fOverlayZoomFactor /= -fDirection;
    else
        m_fOverlayZoomFactor *= fDirection;

    m_fOverlayZoomFactor = st_max(0.05f, m_fOverlayZoomFactor);
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::UploadWindMatricesToShader

void CSpeedTreeForest::UploadWindMatricesToShader(void)
{
    assert(m_vWindMatrices.size( ) == m_cSpeedWind.GetNumWindMatrices( ));
    for (unsigned int i = 0; i < m_cSpeedWind.GetNumWindMatrices( ); ++i)
    {
        if (m_vWindMatrices[i])
            (void) m_conWindMatrices.SetMatrixIndex(m_vWindMatrices[i], i);
        else
            IdvInternal("Attempting to pass NULL matrix into SetMatrixIndex()");
    }
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::UploadLeafWindAngles

void CSpeedTreeForest::UploadLeafWindAngles(const CIdvConstant& cParameter)
{
    const int nNumLeafAngles = m_cSpeedWind.GetNumLeafAngleMatrices( );
    st_assert(int(m_vWindRockAngles.size( )) == nNumLeafAngles);
    st_assert(int(m_vWindRustleAngles.size( )) == nNumLeafAngles);

    // upload leaf angle table
    if (m_vLeafAngleUploadTable.empty( ))
        m_vLeafAngleUploadTable.resize(4 * nNumLeafAngles);
    float* pTablePtr = &m_vLeafAngleUploadTable[0];
    for (int i = 0; i < nNumLeafAngles; ++i)
    {
        *pTablePtr++ = idv::DegToRad(m_vWindRockAngles[i]);
        *pTablePtr++ = idv::DegToRad(m_vWindRustleAngles[i]);
        pTablePtr += 2;
    }
    (void) cParameter.SetArray4f(&m_vLeafAngleUploadTable[0], nNumLeafAngles);
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::UploadLightAdjustment

void CSpeedTreeForest::UploadLightAdjustment(void)
{
    // get camera direction
    idv::Vec3 cCamPos, cCamDir;
    CSpeedTreeRT::GetCamera(cCamPos, cCamDir); // cCamPos will not be used
    cCamDir.Normalize( );

#ifdef UPVECTOR_POS_Y
    idv::Vec3 cLightDirFlat(-m_afLightDir[0], 0.0f, m_afLightDir[2]);
    idv::Vec3 cCameraDirFlat(-cCamDir[0], 0.0f, cCamDir[2]);
#else
    idv::Vec3 cLightDirFlat(m_afLightDir[0], m_afLightDir[1], 0.0f);
    idv::Vec3 cCameraDirFlat(cCamDir[0], cCamDir[1], 0.0f);
#endif
    cLightDirFlat.Normalize( );
    cCameraDirFlat.Normalize( );
    float fDot = (cLightDirFlat.Dot(cCameraDirFlat) + 1.0f) * 0.5f;

    (void) m_conLightAdjustment.Set1f(1.0f - fDot);
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::UploadLightAdjustment
//
//  To resolve potentially conflicting composite filenames, tree models
//  are queried until one is found that exists.  If another is found that
//  exists, then a warning is issued.

void CSpeedTreeForest::ResolveCompositeTextureFilenames(const st_string& strTexturePath, st_string& strCompositeDiffuseLeaf, st_string& strCompositeNormalLeaf, st_string strCompositeBB[2], st_string& strCompositeShadow)
{
    CSpeedTreeWrapper* pLeafMapTree = NULL;
    CSpeedTreeWrapper* pBBMapTrees[2] = { NULL, NULL };
    CSpeedTreeWrapper* pShadowMapTree = NULL;

    for (int i = 0; i < int(m_vBaseTrees.size( )); ++i)
    {
        CSpeedTreeWrapper* pTree = m_vBaseTrees[i];

        // query the textures from RT
        CSpeedTreeRT::SMapBank sMapBank;
        pTree->GetMapBank(sMapBank);

        // find leaf diffuse name
        st_string strTextureFilename = IdvNoPath(sMapBank.m_pCompositeMaps[CSpeedTreeRT::TL_DIFFUSE]);
        st_string strFullPath = strTexturePath + strTextureFilename;
        bool bTextureExists = !strTextureFilename.empty( ) && IdvFileExists(strFullPath);
        if (bTextureExists)
        {
            if (strCompositeDiffuseLeaf.empty( ) || strCompositeDiffuseLeaf == strFullPath)
            {
                strCompositeDiffuseLeaf = strFullPath;
                pLeafMapTree = pTree;
            }
            else
            {
                st_assert(pLeafMapTree);
                IdvWarning("Conflicting diffuse leaf map reported:\n\t[%s] by [%s]\n\t[%s] by [%s]\n",
                    strFullPath.c_str( ), pTree->GetName( ).c_str( ), strCompositeDiffuseLeaf.c_str( ), pLeafMapTree->GetName( ).c_str( ));
            }
        }

        // find leaf normal map name
        strTextureFilename = IdvNoPath(sMapBank.m_pCompositeMaps[CSpeedTreeRT::TL_NORMAL]);
        strFullPath = strTexturePath + strTextureFilename;
        bTextureExists = !strTextureFilename.empty( ) && IdvFileExists(strFullPath);
        if (bTextureExists)
        {
            if (strCompositeNormalLeaf.empty( ) || strCompositeNormalLeaf == strFullPath)
            {
                strCompositeNormalLeaf = strFullPath;
                pLeafMapTree = pTree;
            }
            else
            {
                st_assert(pLeafMapTree);
                IdvWarning("Conflicting normal leaf map reported:\n\t[%s] by [%s]\n\t[%s] by [%s]\n",
                    strFullPath.c_str( ), pTree->GetName( ).c_str( ), strCompositeNormalLeaf.c_str( ), pLeafMapTree->GetName( ).c_str( ));
            }
        }

        // find bb diffuse name
        strTextureFilename = IdvNoPath(sMapBank.m_pBillboardMaps[CSpeedTreeRT::TL_DIFFUSE]);
        strFullPath = strTexturePath + strTextureFilename;
        bTextureExists = !strTextureFilename.empty( ) && IdvFileExists(strFullPath);
        if (bTextureExists)
        {
            if (strCompositeBB[0].empty( ) || strCompositeBB[0] == strFullPath)
            {
                strCompositeBB[0] = strFullPath;
                pBBMapTrees[0] = pTree;
            }
            else
            {
                st_assert(pBBMapTrees[0]);
                IdvWarning("Conflicting diffuse billboard map reported:\n\t[%s] by [%s]\n\t[%s] by [%s]\n",
                    strFullPath.c_str( ), pTree->GetName( ).c_str( ), strCompositeBB[0].c_str( ), pBBMapTrees[0]->GetName( ).c_str( ));
            }
        }

        // find bb normal-map name
        strTextureFilename = IdvNoPath(sMapBank.m_pBillboardMaps[CSpeedTreeRT::TL_NORMAL]);
        strFullPath = strTexturePath + strTextureFilename;
        bTextureExists = !strTextureFilename.empty( ) && IdvFileExists(strFullPath);
        if (bTextureExists)
        {
            if (strCompositeBB[1].empty( ) || strCompositeBB[1] == strFullPath)
            {
                strCompositeBB[1] = strFullPath;
                pBBMapTrees[1] = pTree;
            }
            else
            {
                st_assert(pBBMapTrees[1]);
                IdvWarning("Conflicting normal-map billboard map reported:\n\t[%s] by [%s]\n\t[%s] by [%s]\n",
                    strFullPath.c_str( ), pTree->GetName( ).c_str( ), strCompositeBB[1].c_str( ), pBBMapTrees[1]->GetName( ).c_str( ));
            }
        }

        // find self-shadow name
        strTextureFilename = IdvNoPath(sMapBank.m_pSelfShadowMap);
        strFullPath = strTexturePath + strTextureFilename;
        bTextureExists = !strTextureFilename.empty( ) && IdvFileExists(strFullPath);
        if (bTextureExists)
        {
            if (strCompositeShadow.empty( ) || strCompositeShadow == strFullPath)
            {
                strCompositeShadow = strFullPath;
                pShadowMapTree = pTree;
            }
            else
            {
                st_assert(pShadowMapTree);
                IdvWarning("Conflicting self-shadow map reported:\n\t[%s] by [%s]\n\t[%s] by [%s]\n",
                    strFullPath.c_str( ), pTree->GetName( ).c_str( ), strCompositeDiffuseLeaf.c_str( ), pShadowMapTree->GetName( ).c_str( ));
            }
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::AddInstance

void CSpeedTreeForest::AddInstance(CSpeedTreeInstance* pTree)
{
    // query the bounding box
    float afBoundingBox[6];
    pTree->InstanceOf( )->GetBoundingBox(afBoundingBox);

    const float* pPos = pTree->Pos( );
    AdjustExtents(pPos[0], pPos[1], pPos[2]);
#ifdef UPVECTOR_POS_Y
    AdjustExtents(pPos[0], pPos[1] + afBoundingBox[4], pPos[2]);
#else
    AdjustExtents(pPos[0], pPos[1], pPos[2] + afBoundingBox[5]);
#endif
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::BuildBillboardTexcoordsTable

void CSpeedTreeForest::BuildBillboardTexcoordsTable(void)
{
    for (int nBase = 0; nBase < int(m_vBaseTrees.size( )); ++nBase)
    {
        int nCount = m_vBaseTrees[nBase]->GetDrawData( ).m_nNum360Billboards * 4;
        if (nCount > 0)
        {
            int nCopyPoint = int(m_v360TexCoordTable.size( ));
            m_v360TexCoordTable.resize(m_v360TexCoordTable.size( ) + nCount);

            (void) copy(m_vBaseTrees[nBase]->GetDrawData( ).m_pTexCoords360, m_vBaseTrees[nBase]->GetDrawData( ).m_pTexCoords360 + nCount, m_v360TexCoordTable.begin( ) + nCopyPoint);
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::SetWindowBounds

void CSpeedTreeForest::SetWindowBounds(unsigned int uiWidth, unsigned int uiHeight)
{
    m_uiScreenWidth = uiWidth;
    m_uiScreenHeight = uiHeight;
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::AdjustForAspectRatio

static void AdjustForAspectRatio(int nWindowWidth, int nWindowHeight, float* cMin, float* cMax, float* cNewMin, float* cNewMax)
{
    float fAspect = (float) nWindowHeight / (float) nWindowWidth;
    cNewMin[0] = cMin[0];
    cNewMin[1] = cMin[1];
    cNewMax[0] = cMax[0];
    cNewMax[1] = cMax[1];

    float fXDistance = (cMax[0] - cMin[0]);
    float fYDistance = (cMax[1] - cMin[1]);
    float fWidth = fXDistance;
    float fHeight = fYDistance;
    if (fYDistance > fXDistance)// && false)
    {
        float fTemp = fHeight / fAspect;
        float fAdjust = (fTemp - fWidth) / 2.0f;
        cNewMin[0] -= fAdjust;
        cNewMax[0] += fAdjust;
        fWidth = cNewMax[0] - cNewMin[0];
        if (fWidth < fXDistance)
        {
            fAdjust = (fXDistance - fWidth) / 2.0f;
            cNewMin[0] -= fAdjust;
            cNewMax[0] += fAdjust;
            fWidth = cNewMax[0] - cNewMin[0];
            fTemp = fWidth * fAspect;
            fAdjust = (fTemp - fHeight) / 2.0f;
            cNewMax[1] += fAdjust;
            cNewMin[1] -= fAdjust;
        }
    }
    else
    {
        float fTemp = fWidth * fAspect;
        float fAdjust = (fTemp - fHeight) / 2.0f;
        cNewMax[1] += fAdjust;
        cNewMin[1] -= fAdjust;
        fHeight = cNewMax[1] - cNewMin[1];
        if (fHeight < fYDistance)
        {
            fAdjust = (fYDistance - fHeight) / 2.0f;
            cNewMax[1] += fAdjust;
            cNewMin[1] -= fAdjust;
            fHeight = cNewMax[1] - cNewMin[1];
            fTemp = fHeight / fAspect;
            fAdjust = (fTemp - fWidth) / 2.0f;
            cNewMin[0] -= fAdjust;
            cNewMax[0] += fAdjust;
        }
    }
}



///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::DrawOverlay

void CSpeedTreeForest::DrawOverlay(void)
{
    // overlay extents should be the forest's extents merged with the frustum's extents
    idv::Region cExtents;

    // merge with forest's extents
    cExtents = cExtents ^ idv::Vec3(m_afExtents);
    cExtents = cExtents ^ idv::Vec3(m_afExtents + 3);

    // merge with frustum's extents
    idv::Vec3 cFrustumMin, cFrustumMax;
    m_cCullEngine.GetFrustumExtents(cFrustumMin, cFrustumMax);
    cExtents = cExtents ^ cFrustumMin;
    cExtents = cExtents ^ cFrustumMax;

    // use the center of the frustum, not the camera position
    float afCenter[3];
    m_cCullEngine.GetFrustumCenter(afCenter);

    // adjust the center to work with the ortho projection below
    afCenter[0] -= cExtents.MidValue(0);
    afCenter[1] -= cExtents.MidValue(1);

    // compute view extents based on keeping the frustum centered
    float afCurrentMin[2] = { cExtents.m_cMin[0] + afCenter[0], cExtents.m_cMin[1] + afCenter[1] };
    float afCurrentMax[2] = { cExtents.m_cMax[0] + afCenter[0], cExtents.m_cMax[1] + afCenter[1] };

    // adjust for screen aspect ratio
    float afNewMin[2], afNewMax[2];
    AdjustForAspectRatio(m_uiScreenWidth, m_uiScreenHeight, afCurrentMin, afCurrentMax, afNewMin, afNewMax);

    idv::Transform matOrthoProjection;
    idv::Transform matIdentity;
    matIdentity.Translate(afCenter[0], afCenter[1], 0.0f);
    matIdentity.Scale(m_fOverlayZoomFactor, m_fOverlayZoomFactor, 1.0f);
    matIdentity.Translate(-afCenter[0], -afCenter[1], 0.0f);
#ifdef IDV_OPENGL
    glMatrixMode(GL_PROJECTION);
    glPushMatrix( );
    glLoadIdentity( );
#ifdef PS3
    glOrthof(afNewMin[0], afNewMax[0], afNewMin[1], afNewMax[1], -200000.0f, 200000.0f);
#else
    glOrtho(afNewMin[0], afNewMax[0], afNewMin[1], afNewMax[1], -200000.0f, 200000.0f);
#endif
    glGetFloatv(GL_PROJECTION_MATRIX, matOrthoProjection);
    glPopMatrix( );
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity( );
#endif
#if IDV_DIRECTX9 | _XBOX
    D3DXMatrixOrthoOffCenterLH((D3DXMATRIX*) &matOrthoProjection, afNewMin[0], afNewMax[0], afNewMin[1], afNewMax[1], -200000.0f, 200000.0f);
#endif

    CIdvConstant::UpdateModelviewProjectionMatrix(matOrthoProjection, matIdentity);
    (void) m_conWorldViewProjectionMatrix.SetModelviewProjectionMatrix( );

    (void) m_cOverlayTechnique.Begin( );
#ifndef LOAD_BINARY_SHADERS 
    m_cCullEngine.DrawOverlay(&m_cEffect, &m_conOverlayColor);
#else
    m_cCullEngine.DrawOverlay(NULL, &m_conOverlayColor);
#endif
    (void) m_cOverlayTechnique.End( );
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeForest::DrawCells

void CSpeedTreeForest::DrawCells(void)
{
    (void) m_cOverlayTechnique.Begin( );
#ifndef LOAD_BINARY_SHADERS
    m_cCullEngine.DrawActiveCells(&m_cEffect, &m_conOverlayColor);
#else
    m_cCullEngine.DrawActiveCells(NULL, &m_conOverlayColor);
#endif
    (void) m_cOverlayTechnique.End( );
}
