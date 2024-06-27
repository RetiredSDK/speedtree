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
#include "SpeedTreeWrapper.h"
#include "SpeedTreeInstance.h"
#include "IdvFilename.h"
#include "IdvVectorMath.h"
#include <cmath>
using namespace std;


///////////////////////////////////////////////////////////////////////
//  Constants

const float c_fWindWeightCompressionRange = 0.98f;


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeWrapper::CSpeedTreeWrapper

CSpeedTreeWrapper::CSpeedTreeWrapper( ) :
    m_fCullRadius(0.0f),
    m_bHasBranchGeometry(false),
    m_bHasFrondGeometry(false),
    m_bHasLeafCardGeometry(false),
    m_bHasLeafMeshGeometry(false),
    m_bBranchesReceiveShadows(true),
    m_bFrondsReceiveShadows(true),
#ifdef INTERMEDIATE_FRONDBRANCH_FADING
    m_fBranchFadeDistance(0.2f),
    m_fFrondFadeDistance(0.2f),
#endif
    m_fLeafRockScalar(1.0f),
    m_fLeafRustleScalar(1.0f),
    m_nSortIndex(-1)
{
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeWrapper::~CSpeedTreeWrapper

CSpeedTreeWrapper::~CSpeedTreeWrapper( )
{
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeWrapper::LoadSptFile

st_string CSpeedTreeWrapper::LoadSptFile(const char* pFilename, int nNumWindMatrices, unsigned int nSeed, float fSize, float fSizeVariance)
{
    st_string strError;
    m_strName = pFilename;

    // set number of wind matrices to match SpeedWind
    CSpeedTreeRT::SetNumWindMatrices(nNumWindMatrices);

    // load the tree model
#ifdef REPORT_TIMES
    CIdvTimer cLoadTimer;
    cLoadTimer.Start( );
#endif
    if (CSpeedTreeRT::LoadTree(pFilename))
    {
        printf("Loading SPT file: %s\n", pFilename);
#ifdef REPORT_TIMES
        cLoadTimer.Stop( );
        IdvReport("[%s] load time: %g ms", IdvNoPath(pFilename).c_str( ), cLoadTimer.GetMilliSec( ));
#endif

        // make sure SpeedTreeRT generates normals
        SetBranchLightingMethod(CSpeedTreeRT::LIGHT_DYNAMIC);
        SetLeafLightingMethod(CSpeedTreeRT::LIGHT_DYNAMIC);
        SetFrondLightingMethod(CSpeedTreeRT::LIGHT_DYNAMIC);

        // set the wind method to use the GPU
        SetBranchWindMethod(CSpeedTreeRT::WIND_GPU);
        SetLeafWindMethod(CSpeedTreeRT::WIND_GPU);
        SetFrondWindMethod(CSpeedTreeRT::WIND_GPU);

        // override the size, if necessary
        if (fSize >= 0.0f && fSizeVariance >= 0.0f)
            SetTreeSize(fSize, fSizeVariance);

        // generate tree geometry
#ifdef REPORT_TIMES
        CIdvTimer cComputeTimer;
        cComputeTimer.Start( );
#endif
        if (Compute(NULL, nSeed))
        {
#ifdef REPORT_TIMES
            cComputeTimer.Stop( );
            IdvReport("[%s] compute time: %g ms", IdvNoPath(pFilename).c_str( ), cComputeTimer.GetMilliSec( ));
#endif

            // set the cull radius
            ComputeCullRadius( );

            // make the leaves rock in the wind
            SetLeafRockingState(true);

            // billboard setup
            SetDropToBillboard(true);

            // query & set materials
            m_cBranchMaterial.Set(CSpeedTreeRT::GetBranchMaterial( ));
            m_cLeafMaterial.Set(CSpeedTreeRT::GetLeafMaterial( ));
            m_cFrondMaterial.Set(CSpeedTreeRT::GetFrondMaterial( ));

            // scale ambient values
            GetLightShaderParams(m_sLightScalars);
            m_cBranchMaterial.ScaleAmbient(m_sLightScalars.m_fAmbientScalar);
            m_cLeafMaterial.ScaleAmbient(m_sLightScalars.m_fAmbientScalar);
            m_cFrondMaterial.ScaleAmbient(m_sLightScalars.m_fAmbientScalar);

            // textures (failure doesn't affect bSuccess)
            LoadBranchTextures(IdvPath(pFilename));
        }
        else // tree failed to compute
            strError = IdvFormatString("Internal Error: CSpeedTreeRT::Compute() failed for [%s]", pFilename);

    }
    else // tree failed to load
        strError = CSpeedTreeRT::GetCurrentError( );

    return strError;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeWrapper::MakeInstance

void CSpeedTreeWrapper::MakeInstance(CSpeedTreeInstance* pInstance)
{
    st_assert(pInstance);

    pInstance->SetParent(this);
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeWrapper::SetupBranchGeometry

void CSpeedTreeWrapper::SetupBranchGeometry(CIdvTightlyPackedBuffer& cBuffer, const CSpeedWind& cWind)
{
    // query vertex attribute data
    CSpeedTreeRT::SGeometry sGeometry;
    GetGeometry(sGeometry, SpeedTree_BranchGeometry);
    const SGeometry::SIndexed& sBranches = sGeometry.m_sBranches;

#ifdef INTERMEDIATE_FRONDBRANCH_FADING
    // get transition radius
    m_fBranchFadeDistance = sBranches.m_fLodFadeDistance;
#endif

    // are self-shadow coordinates present?
    m_bBranchesReceiveShadows = sBranches.m_pTexCoords[CSpeedTreeRT::TL_SHADOW] != NULL;

    // mark vertex position in composite buffer
    m_sDrawData.m_nBranchVertexDataStart = cBuffer.NumVertices( );

    if (sBranches.m_nNumVertices > 0)
    {
        // attributes are stored a little strangely in an attempt to pack as many in as possible

        // .xyz = coords
        // .w = self-shadow s-texcoord
        st_assert(sBranches.m_pCoords);
        int i = 0;
        for (i = 0; i < sBranches.m_nNumVertices; ++i)
        {
            float fSelfShadowCoord_S = m_bBranchesReceiveShadows ? sBranches.m_pTexCoords[CSpeedTreeRT::TL_SHADOW][i * 2 + 0] : 0.0f;
            cBuffer.Vertex4(sBranches.m_pCoords[i * 3 + 0], sBranches.m_pCoords[i * 3 + 1], sBranches.m_pCoords[i * 3 + 2], fSelfShadowCoord_S);
        }

        // .xyz = normal
        // .w = self-shadow t-texcoord
        st_assert(sBranches.m_pNormals);
        for (i = 0; i < sBranches.m_nNumVertices; ++i)
        {
            float fSelfShadowCoord_T = m_bBranchesReceiveShadows ? sBranches.m_pTexCoords[CSpeedTreeRT::TL_SHADOW][i * 2 + 1] : 0.0f;
            cBuffer.TexCoord4(0, sBranches.m_pNormals[i * 3 + 0], sBranches.m_pNormals[i * 3 + 1], sBranches.m_pNormals[i * 3 + 2], fSelfShadowCoord_T);
        }

        // .xy = diffuse texcoords (every tree must have a diffuse layer - fatal error if not)
        // .zw = wind parameters
        st_assert(sBranches.m_pTexCoords[CSpeedTreeRT::TL_DIFFUSE]);
        st_assert(sBranches.m_pWindMatrixIndices[0] && sBranches.m_pWindWeights[0] && sBranches.m_pWindMatrixIndices[1] && sBranches.m_pWindWeights[1]);
        for (i = 0; i < sBranches.m_nNumVertices; ++i)
        {
            const float* pDiffuseCoords = sBranches.m_pTexCoords[CSpeedTreeRT::TL_DIFFUSE] + i * 2;
            float fWindMatrixIndex1 = float(int(sBranches.m_pWindMatrixIndices[0][i] * 10.0f / cWind.GetNumWindMatrices( )));
            float fWindMatrixWeight1 = c_fWindWeightCompressionRange * sBranches.m_pWindWeights[0][i];
            float fWindMatrixIndex2 = float(int(sBranches.m_pWindMatrixIndices[1][i] * 10.0f / cWind.GetNumWindMatrices( )));
            float fWindMatrixWeight2 = c_fWindWeightCompressionRange * sBranches.m_pWindWeights[1][i];
            cBuffer.TexCoord4(1, pDiffuseCoords[0], pDiffuseCoords[1], fWindMatrixIndex1 + fWindMatrixWeight1, fWindMatrixIndex2 + fWindMatrixWeight2);
        }

        // normal-map texcoords
        // .xy = detail texcoords
        // .zw = normal-map texcoords
        for (i = 0; i < sBranches.m_nNumVertices; ++i)
        {
#ifdef BRANCH_DETAIL_LAYER
            const float* pDetailCoords = sBranches.m_pTexCoords[CSpeedTreeRT::TL_DETAIL] + i * 2;
#else
            float pDetailCoords[2] = { 0.0f, 0.0f };
#endif
#ifdef BRANCH_NORMAL_MAPPING
            const float* pNormalMapCoords = sBranches.m_pTexCoords[CSpeedTreeRT::TL_NORMAL] + i * 2;
#else
            float pNormalMapCoords[2] = { 0.0f, 0.0f };
#endif
            cBuffer.TexCoord4(2, pDetailCoords[0], pDetailCoords[1], pNormalMapCoords[0], pNormalMapCoords[1]);
        }

        // tangents (binormals are derived in the vertex shader)
        // .xyz = tangent
        // .w = lod fade hint
        for (i = 0; i < sBranches.m_nNumVertices; ++i)
        {
#ifdef BRANCH_NORMAL_MAPPING
            st_assert(sBranches.m_pTangents);
            float afTexCoords[ ] = 
            { 
                sBranches.m_pTangents[i * 3 + 0],
                sBranches.m_pTangents[i * 3 + 1],
                sBranches.m_pTangents[i * 3 + 2]
#ifdef INTERMEDIATE_FRONDBRANCH_FADING
                ,sBranches.m_pLodFadeHints[i]
#endif
            };
#else // BRANCH_NORMAL_MAPPING
            float afTexCoords[ ] = 
            { 
                0.0f, 0.0f, 0.0f
#ifdef INTERMEDIATE_FRONDBRANCH_FADING
                ,sBranches.m_pLodFadeHints[i]
#endif
            };
#endif // BRANCH_NORMAL_MAPPING

#ifdef INTERMEDIATE_FRONDBRANCH_FADING
            cBuffer.TexCoord4v(3, afTexCoords);
#else
            cBuffer.TexCoord3v(3, afTexCoords);
#endif
        }

        // setup indices
        int nNumLods = GetNumBranchLodLevels( );
        if (nNumLods > 0)
        {
            m_sDrawData.m_pBranchIndexDataStart = st_new_array<int>(nNumLods, "CSpeedTreeWrapper::SetupBranchGeometry, m_sDrawData.m_pBranchIndexDataStart");
            m_sDrawData.m_pBranchStripLengths = st_new_array<int>(nNumLods, "CSpeedTreeWrapper::SetupBranchGeometry, m_sDrawData.m_pBranchStripLengths");

            for (int nLod = 0; nLod < nNumLods; ++nLod)
            {
                m_sDrawData.m_pBranchIndexDataStart[nLod] = cBuffer.NumIndices( );

                st_assert(sBranches.m_pNumStrips);
                if (sBranches.m_pNumStrips[nLod] == 1)
                {
                    st_assert(m_sDrawData.m_pBranchStripLengths);
                    st_assert(sBranches.m_pStripLengths[nLod]);
                    m_sDrawData.m_pBranchStripLengths[nLod] = sBranches.m_pStripLengths[nLod][0];

                    int nNumVertsInStrip = sBranches.m_pStripLengths[nLod][0];
                    if (nNumVertsInStrip > 2)
                        m_bHasBranchGeometry = true;
                    for (i = 0; i < nNumVertsInStrip; ++i)
                    {
                        st_assert(sBranches.m_pStrips[nLod][0][i] > -1 && sBranches.m_pStrips[nLod][0][i] < sBranches.m_nNumVertices);
                        cBuffer.AddIndex(sBranches.m_pStrips[nLod][0][i] + m_sDrawData.m_nBranchVertexDataStart);
                    }
                }
                else
                    m_sDrawData.m_pBranchStripLengths[nLod] = 0;
            }
        }
    }
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeWrapper::SetupFrondGeometry

void CSpeedTreeWrapper::SetupFrondGeometry(CIdvTightlyPackedBuffer& cBuffer, const CSpeedWind& cWind)
{
    // query vertex attribute data
    CSpeedTreeRT::SGeometry sGeometry;
    GetGeometry(sGeometry, SpeedTree_FrondGeometry);
    const SGeometry::SIndexed& sFronds = sGeometry.m_sFronds;

#ifdef INTERMEDIATE_FRONDBRANCH_FADING
    // get transition radius
    m_fFrondFadeDistance = sFronds.m_fLodFadeDistance;
#endif

    // are self-shadow coordinates present?
    m_bFrondsReceiveShadows = sFronds.m_pTexCoords[CSpeedTreeRT::TL_SHADOW] != NULL;

    // mark vertex position in composite buffer
    m_sDrawData.m_nFrondVertexDataStart = cBuffer.NumVertices( );

    if (sFronds.m_nNumVertices > 0)
    {
        // .xyz = coords
        // .w = self-shadow s-texcoord
        st_assert(sFronds.m_pCoords);
        int i = 0;
        for (i = 0; i < sFronds.m_nNumVertices; ++i)
        {
            float fSelfShadowCoord_S = m_bFrondsReceiveShadows ? sFronds.m_pTexCoords[CSpeedTreeRT::TL_SHADOW][i * 2 + 0] : 0.0f;
            cBuffer.Vertex4(sFronds.m_pCoords[i * 3 + 0], sFronds.m_pCoords[i * 3 + 1], sFronds.m_pCoords[i * 3 + 2], fSelfShadowCoord_S);
        }

        // .xyz = normal
        // .w = self-shadow t-texcoord
        st_assert(sFronds.m_pNormals);
        for (i = 0; i < sFronds.m_nNumVertices; ++i)
        {
            float fSelfShadowCoord_T = m_bFrondsReceiveShadows ? sFronds.m_pTexCoords[CSpeedTreeRT::TL_SHADOW][i * 2 + 1] : 0.0f;
            cBuffer.TexCoord4(0, sFronds.m_pNormals[i * 3 + 0], sFronds.m_pNormals[i * 3 + 1], sFronds.m_pNormals[i * 3 + 2], fSelfShadowCoord_T);
        }

        // .xy = diffuse texcoords (every tree must have a diffuse layer - fatal error if not)
        // .zw = wind parameters
        st_assert(sFronds.m_pTexCoords[CSpeedTreeRT::TL_DIFFUSE]);
        st_assert(sFronds.m_pWindMatrixIndices[0] && sFronds.m_pWindWeights[0] && sFronds.m_pWindMatrixIndices[1] && sFronds.m_pWindWeights[1]);
        for (i = 0; i < sFronds.m_nNumVertices; ++i)
        {
            const float* pDiffuseCoords = sFronds.m_pTexCoords[CSpeedTreeRT::TL_DIFFUSE] + i * 2;
            float fWindMatrixIndex1 = float(int(sFronds.m_pWindMatrixIndices[0][i] * 10.0f / cWind.GetNumWindMatrices( )));
            float fWindMatrixWeight1 = c_fWindWeightCompressionRange * sFronds.m_pWindWeights[0][i];
            float fWindMatrixIndex2 = float(int(sFronds.m_pWindMatrixIndices[1][i] * 10.0f / cWind.GetNumWindMatrices( )));
            float fWindMatrixWeight2 = c_fWindWeightCompressionRange * sFronds.m_pWindWeights[1][i];
            cBuffer.TexCoord4(1, pDiffuseCoords[0], pDiffuseCoords[1], fWindMatrixIndex1 + fWindMatrixWeight1, fWindMatrixIndex2 + fWindMatrixWeight2);
        }

#if defined(INTERMEDIATE_FRONDBRANCH_FADING) || defined(FROND_NORMAL_MAPPING)
        for (i = 0; i < sFronds.m_nNumVertices; ++i)
        {
#ifdef INTERMEDIATE_FRONDBRANCH_FADING
            st_assert(sFronds.m_pLodFadeHints);
            float fFadeHint = sFronds.m_pLodFadeHints[i];
#else
            float fFadeHint = -1.0f;
#endif
#ifdef FROND_NORMAL_MAPPING
            const float* pTangent = sFronds.m_pTangents + i * 3;
#else
            const float pTangent[3] = { 0.0f, 0.0f, 0.0f };
#endif
            const float afLayerData[4] = { pTangent[0], pTangent[1], pTangent[2], fFadeHint };
            cBuffer.TexCoord4v(2, afLayerData);
        }
#endif

        // query index data
        int nNumLods = GetNumFrondLodLevels( );
        if (nNumLods > 0)
        {
            m_sDrawData.m_pFrondIndexDataStart = st_new_array<int>(nNumLods, "CSpeedTreeWrapper::SetupFrondGeometry, m_sDrawData.m_pFrondIndexDataStart");
            m_sDrawData.m_pFrondStripLengths = st_new_array<int>(nNumLods, "CSpeedTreeWrapper::SetupFrondGeometry, m_sDrawData.m_pFrondStripLengths");
            for (int nLod = 0; nLod < nNumLods; ++nLod)
            {
                m_sDrawData.m_pFrondIndexDataStart[nLod] = cBuffer.NumIndices( );

                st_assert(sFronds.m_pNumStrips);
                if (sFronds.m_pNumStrips[nLod] == 1)
                {
                    st_assert(m_sDrawData.m_pFrondStripLengths);
                    st_assert(sFronds.m_pStripLengths[nLod]);
                    m_sDrawData.m_pFrondStripLengths[nLod] = sFronds.m_pStripLengths[nLod][0];

                    int nNumVertsInStrip = sFronds.m_pStripLengths[nLod][0];
                    if (nNumVertsInStrip > 2)
                        m_bHasFrondGeometry = true;
                    for (i = 0; i < nNumVertsInStrip; ++i)
                    {
                        st_assert(sFronds.m_pStrips[nLod][0][i] > -1 && sFronds.m_pStrips[nLod][0][i] < sFronds.m_nNumVertices);
                        cBuffer.AddIndex(sFronds.m_pStrips[nLod][0][i] + m_sDrawData.m_nFrondVertexDataStart);
                    }
                }
                else
                    m_sDrawData.m_pFrondStripLengths[nLod] = 0;
            }
        }
    }
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeWrapper::SetupBillboardGeometry

void CSpeedTreeWrapper::SetupBillboardGeometry(int nTexCoordsTableOffset)
{
    CSpeedTreeRT::SGeometry sGeometry;
    GetGeometry(sGeometry, SpeedTree_BillboardGeometry);
    SetLodLevel(0.0f);
    UpdateBillboardGeometry(sGeometry);

    // setup vertical billboards
    {
        // init the static parts of the billboard struct
        SSpeedTreeBillboard::SVertex* pVertex = &m_cVertBillboardVertices.m_sVert0;

        // OpenGL and Xenon both support quad primitive rendering - standard DirectX 9.0c does not
        const CSpeedTreeRT::SLightShaderParams& sLightScales = GetLightScales( );
#if IDV_OPENGL | _XBOX
        for (int i = 0; i < 4; ++i)
        {
            // cluster corner
            pVertex->m_afPos[3] = float(i);

            // width & height
            pVertex->m_afTexCoord0[0] = sGeometry.m_s360Billboard.m_fWidth;
            pVertex->m_afTexCoord0[1] = sGeometry.m_s360Billboard.m_fHeight;

            // 360 billboard texcoord table offset
            pVertex->m_afMiscParams[1] = float(nTexCoordsTableOffset);
            pVertex->m_afMiscParams[2] = float(sGeometry.m_s360Billboard.m_nNumImages);
            pVertex->m_afMiscParams[3] = sGeometry.m_s360Billboard.m_fTransitionPercent;

            // light adjustments
            pVertex->m_afLightAdjusts[0] = sLightScales.m_fBillboardBrightSideLightScalar * sLightScales.m_fGlobalLightScalar;
            pVertex->m_afLightAdjusts[1] = sLightScales.m_fBillboardDarkSideLightScalar * sLightScales.m_fGlobalLightScalar;
            pVertex->m_afLightAdjusts[2] = sLightScales.m_fBillboardAmbientScalar;

            ++pVertex;
        }
#endif

#if IDV_DIRECTX9
        const int anVertIndices[6] = { 0, 1, 2, 0, 2, 3 };

        for (int i = 0; i < 6; ++i)
        {
            // cluster corner
            pVertex->m_afPos[3] = float(anVertIndices[i]);

            // width & height
            pVertex->m_afTexCoord0[0] = sGeometry.m_s360Billboard.m_fWidth;
            pVertex->m_afTexCoord0[1] = sGeometry.m_s360Billboard.m_fHeight;

            // 360 billboard texcoord table offset
            pVertex->m_afMiscParams[1] = float(nTexCoordsTableOffset);
            pVertex->m_afMiscParams[2] = float(sGeometry.m_s360Billboard.m_nNumImages);
            pVertex->m_afMiscParams[3] = sGeometry.m_s360Billboard.m_fTransitionPercent;

            // light adjustments
            pVertex->m_afLightAdjusts[0] = sLightScales.m_fBillboardBrightSideLightScalar * sLightScales.m_fGlobalLightScalar;
            pVertex->m_afLightAdjusts[1] = sLightScales.m_fBillboardDarkSideLightScalar * sLightScales.m_fGlobalLightScalar;
            pVertex->m_afLightAdjusts[2] = sLightScales.m_fBillboardAmbientScalar;

            ++pVertex;
        }
#endif
    }

    // setup horizontal billboards
#ifdef HORZ_BILLBOARDS
    {
        // make a copy of the horizontal billboard (it's static, so one copy one time will do it)
        m_sHorzBillboard = sGeometry.m_sHorzBillboard;
        if (!m_sHorzBillboard.m_pCoords)
            printf("WARNING: App configured to render horizontal billboards (#define HORZ_BILLBOARDS), but SPT [%s] has no horizontal billboard data\n",
                IdvNoPath(m_strName.c_str( )).c_str( ));
        else
        {
            // init the static parts of the billboard struct
            SSpeedTreeBillboard::SVertex* pVertex = &m_cHorzBillboardVertices.m_sVert0;

            // OpenGL and Xenon both support quad primitive rendering - standard DirectX 9.0c does not
            const CSpeedTreeRT::SLightShaderParams& sLightScales = GetLightScales( );
#if IDV_OPENGL | _XBOX
            // the four sides of the billboard
            st_assert(m_sHorzBillboard.m_pCoords);
            st_assert(m_sHorzBillboard.m_pTexCoords);

            for (int i = 0; i < 4; ++i)
            {
                // corner & height
#ifdef UPVECTOR_POS_Y
                pVertex->m_afPos[3] = m_sHorzBillboard.m_pCoords[1];
                pVertex->m_afTexCoord0[0] = m_sHorzBillboard.m_pCoords[i * 3 + 0];
                pVertex->m_afTexCoord0[1] = m_sHorzBillboard.m_pCoords[i * 3 + 2];
#else
                pVertex->m_afPos[3] = m_sHorzBillboard.m_pCoords[2];
                pVertex->m_afTexCoord0[0] = m_sHorzBillboard.m_pCoords[i * 3 + 0];
                pVertex->m_afTexCoord0[1] = m_sHorzBillboard.m_pCoords[i * 3 + 1];
#endif

                // 360 billboard texcoord table offset
                pVertex->m_afMiscParams[1] = m_sHorzBillboard.m_pTexCoords[i * 2];
                pVertex->m_afMiscParams[2] = m_sHorzBillboard.m_pTexCoords[i * 2 + 1];
                pVertex->m_afMiscParams[3] = sGeometry.m_s360Billboard.m_fTransitionPercent;

                // light adjustments
                pVertex->m_afLightAdjusts[0] = sLightScales.m_fBillboardBrightSideLightScalar * sLightScales.m_fGlobalLightScalar;
                pVertex->m_afLightAdjusts[1] = sLightScales.m_fBillboardDarkSideLightScalar * sLightScales.m_fGlobalLightScalar;
                pVertex->m_afLightAdjusts[2] = sLightScales.m_fBillboardAmbientScalar;

                ++pVertex;
            }
#endif // IDV_OPENGL | _XBOX

#ifdef IDV_DIRECTX9
            const int anVertIndices[6] = { 0, 1, 2, 0, 2, 3 };

            for (int i = 0; i < 6; ++i)
            {
                int nIndex = anVertIndices[i];

                // corner & height
#ifdef UPVECTOR_POS_Y
                pVertex->m_afPos[3] = m_sHorzBillboard.m_pCoords[1];
                pVertex->m_afTexCoord0[0] = m_sHorzBillboard.m_pCoords[nIndex * 3 + 0];
                pVertex->m_afTexCoord0[1] = m_sHorzBillboard.m_pCoords[nIndex * 3 + 2];
#else
                pVertex->m_afPos[3] = m_sHorzBillboard.m_pCoords[2];
                pVertex->m_afTexCoord0[0] = m_sHorzBillboard.m_pCoords[nIndex * 3 + 0];
                pVertex->m_afTexCoord0[1] = m_sHorzBillboard.m_pCoords[nIndex * 3 + 1];
#endif

                // 360 billboard texcoord table offset
                pVertex->m_afMiscParams[1] = m_sHorzBillboard.m_pTexCoords[nIndex * 2];
                pVertex->m_afMiscParams[2] = m_sHorzBillboard.m_pTexCoords[nIndex * 2 + 1];
                pVertex->m_afMiscParams[3] = sGeometry.m_s360Billboard.m_fTransitionPercent;

                // light adjustments
                pVertex->m_afLightAdjusts[0] = sLightScales.m_fBillboardBrightSideLightScalar * sLightScales.m_fGlobalLightScalar;
                pVertex->m_afLightAdjusts[1] = sLightScales.m_fBillboardDarkSideLightScalar * sLightScales.m_fGlobalLightScalar;
                pVertex->m_afLightAdjusts[2] = sLightScales.m_fBillboardAmbientScalar;

                ++pVertex;
            }
#endif // IDV_DIRECTX9
        }
    }
#endif // HORZ_BILLBOARDS

    // init the dynamic parts of the billboard struct with something
    const float afOrigin[3] = { 0.0f, 0.0f, 0.0f };
    m_cVertBillboardVertices.UpdateVertical(afOrigin, 0.0f, 0.0f, 1.0f);

    // setup the 360 texcoord table for quick shader uploads
    m_sDrawData.m_nNum360Billboards = sGeometry.m_s360Billboard.m_nNumImages;
    if (m_sDrawData.m_nNum360Billboards > 0)
    {
        float* pVecPtr = m_sDrawData.m_pTexCoords360 = st_new_array<float>(m_sDrawData.m_nNum360Billboards * 4, "CSpeedTreForest::InitGraphics, pVecPtr = pBuffer->m_pTexCoords");
        for (int i = 0; i < m_sDrawData.m_nNum360Billboards; ++i)
        {
            // order is max_s, max_t, width_s, height_t
            *pVecPtr++ = sGeometry.m_s360Billboard.m_pTexCoordTable[i * 8];
            *pVecPtr++ = sGeometry.m_s360Billboard.m_pTexCoordTable[i * 8 + 1];

            *pVecPtr++ = sGeometry.m_s360Billboard.m_pTexCoordTable[i * 8] - sGeometry.m_s360Billboard.m_pTexCoordTable[i * 8 + 4];
            *pVecPtr++ = sGeometry.m_s360Billboard.m_pTexCoordTable[i * 8 + 1] - sGeometry.m_s360Billboard.m_pTexCoordTable[i * 8 + 5];
        }
    }
    else
        IdvWarning("No billboard images found in [%s]", GetName( ).c_str( ));
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeWrapper::LoadBranchTextures

void CSpeedTreeWrapper::LoadBranchTextures(const st_string& strTexturePath)
{
    // query the textures from RT
    CSpeedTreeRT::SMapBank sMapBank;
    GetMapBank(sMapBank);

    // try to load any references textures
    for (int nLayer = CSpeedTreeRT::TL_DIFFUSE; nLayer < CSpeedTreeRT::TL_SHADOW; ++nLayer)
    {
        st_string strTextureFilename = IdvNoPath(sMapBank.m_pBranchMaps[nLayer]);
        if (!strTextureFilename.empty( ))
        {
            st_string strFullPath = strTexturePath + strTextureFilename;

            // this sample is only looking for TL_DIFFUSE and TL_NORMAL, but your app can use any of the layers
            if (nLayer == TL_DIFFUSE || nLayer == TL_NORMAL)
            {
                if (!m_texBranchMaps[nLayer].Load(strFullPath))
                    IdvWarning("Failed to load branch layer [%s] texture: '%s' for [%s]", GetTextureLayerName(ETextureLayers(nLayer)), strFullPath.c_str( ), m_strName.c_str( ));
            }
#ifdef BRANCH_DETAIL_LAYER
            else if (nLayer == TL_DETAIL)
            {
                if (!m_texBranchMaps[nLayer].Load(strFullPath))
                    IdvWarning("Failed to load branch layer [%s] texture: '%s' for [%s]", GetTextureLayerName(ETextureLayers(nLayer)), strFullPath.c_str( ), m_strName.c_str( ));
            }
#endif
            else
                IdvWarning("This app is configured to ignore the branch detail layer '%s' for [%s]", strFullPath.c_str( ), m_strName.c_str( ));
        }
    }
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeWrapper::FillMissingTexcoords
//
//  Because the three basic geometry types (leaves, branches, & fronds) each
//  share their respective vertex buffers, it's important that each tree
//  have the same vertex attributes.  If an expected layer is missing, some
//  sort of dummy/filler data is provided here, and a warning is issued.

void CSpeedTreeWrapper::FillMissingTexcoords(CIdvTightlyPackedBuffer& cBuffer, const st_string& strLayerName, int nLayer, int nNumCoords, int nNumVerts)
{
    IdvWarning("Missing texcoords for %s in [%s]", strLayerName.c_str( ), m_strName.c_str( ));

    st_vector_float vEmptyLayer(nNumVerts * nNumCoords, 0.0f);
    cBuffer.AddTexCoords(nNumVerts, nLayer, nNumCoords, &vEmptyLayer[0]);
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeWrapper::SetupLeafCardGeometry

void CSpeedTreeWrapper::SetupLeafCardGeometry(CIdvTightlyPackedBuffer& cBuffer, const CSpeedWind& cWind)
{
    const int nNumLeafLods = GetNumLeafLodLevels( );
    m_sDrawData.m_nLeafNumLods = nNumLeafLods;
    m_sDrawData.m_pLeafCardVertexDataStart = st_new_array<int>(nNumLeafLods, "CSpeedTreeWrapper::SetupLeafGeometry, m_sDrawData.m_pLeafCardVertexDataStart");
    m_sDrawData.m_pLeafCardVertexDataLengths = st_new_array<int>(nNumLeafLods, "CSpeedTreeWrapper::SetupLeafGeometry, m_sDrawData.m_pLeafCardVertexDataLengths");

    CSpeedTreeRT::SGeometry sGeometry;
    GetGeometry(sGeometry, SpeedTree_LeafGeometry);
    for (int nLod = 0; nLod < nNumLeafLods; ++nLod)
    {
        const SGeometry::SLeaf& sLeaves = sGeometry.m_pLeaves[nLod];

        // grab these parameters for shader uploading later
        if (nLod == 0)
        {
            m_fLeafRockScalar = sLeaves.m_fLeafRockScalar;
            m_fLeafRustleScalar = sLeaves.m_fLeafRustleScalar;
        }

        const int nNumLeaves = sLeaves.m_nNumLeaves;
        m_sDrawData.m_pLeafCardVertexDataStart[nLod] = cBuffer.NumVertices( );

        // make sure the required components are intact
        st_assert(sLeaves.m_pCards);
        st_assert(sLeaves.m_pWindMatrixIndices[0] && sLeaves.m_pWindMatrixIndices[1] && sLeaves.m_pWindWeights[0] && sLeaves.m_pWindWeights[1]);

        // OpenGL and Xenon both support quad primitive rendering - standard DirectX 9.0c does not
        const int nNumSpeedWindAngles = cWind.GetNumLeafAngleMatrices( );
#if IDV_OPENGL | _XBOX
        m_sDrawData.m_pLeafCardVertexDataLengths[nLod] = 4 * nNumLeaves;
        for (int nLeaf = 0; nLeaf < nNumLeaves; ++nLeaf)
        {
            const SGeometry::SLeaf::SCard* pCard = sLeaves.m_pCards + sLeaves.m_pLeafCardIndices[nLeaf];
            if (!pCard->m_pMesh)
            {
                m_bHasLeafCardGeometry = true;

                for (int nCorner = 0; nCorner < 4; ++nCorner)
                {
                    // xy = diffuse texcoords
                    // zw = compressed wind parameters
                    float fWindMatrixIndex1 = float(int(sLeaves.m_pWindMatrixIndices[0][nLeaf] * 10.0f / cWind.GetNumWindMatrices( )));
                    float fWindMatrixWeight1 = c_fWindWeightCompressionRange * sLeaves.m_pWindWeights[0][nLeaf];
                    float fWindMatrixIndex2 = float(int(sLeaves.m_pWindMatrixIndices[1][nLeaf] * 10.0f / cWind.GetNumWindMatrices( )));
                    float fWindMatrixWeight2 = c_fWindWeightCompressionRange * sLeaves.m_pWindWeights[1][nLeaf];
                    cBuffer.TexCoord4(0, pCard->m_pTexCoords[nCorner * 2], pCard->m_pTexCoords[nCorner * 2 + 1], fWindMatrixIndex1 + fWindMatrixWeight1, fWindMatrixIndex2 + fWindMatrixWeight2);

                    // tex layer 1: .x = width, .y = height, .z = pivot x, .w = pivot.y
                    cBuffer.TexCoord4(1, pCard->m_fWidth, pCard->m_fHeight, pCard->m_afPivotPoint[0] - 0.5f, pCard->m_afPivotPoint[1] - 0.5f);

                    // tex layer 2: .x = angle.x, .y = angle.y, .z = leaf angle index [0,c_nNumSpeedWindAngles-1], .w = dimming
                    cBuffer.TexCoord4(2, idv::DegToRad(pCard->m_afAngleOffsets[0]), idv::DegToRad(pCard->m_afAngleOffsets[1]),
                                         float(nLeaf % nNumSpeedWindAngles), sLeaves.m_pDimming[nLeaf]);

#ifdef LEAF_NORMAL_MAPPING
                    // tangent for normal mapping
                    cBuffer.TexCoord3v(3, sLeaves.m_pTangents + nLeaf * 12 + nCorner * 3);
#endif

                    // normal
                    cBuffer.Normalv(sLeaves.m_pNormals + nLeaf * 12 + nCorner * 3);

                    // coordinate
                    cBuffer.Vertex4(sLeaves.m_pCenterCoords[nLeaf * 3 + 0], sLeaves.m_pCenterCoords[nLeaf * 3 + 1], sLeaves.m_pCenterCoords[nLeaf * 3 + 2], float(nCorner));
                }
            }
        }
#endif

        // Standard DirectX 9.0c does not support quad primitive rendering so we'll need to setup a triangle list
#ifdef IDV_DIRECTX9
        m_sDrawData.m_pLeafCardVertexDataLengths[nLod] = 6 * nNumLeaves;
        const int anVertIndices[6] = { 0, 1, 2, 0, 2, 3 };

        for (int nLeaf = 0; nLeaf < nNumLeaves; ++nLeaf)
        {
            const SGeometry::SLeaf::SCard* pCard = sLeaves.m_pCards + sLeaves.m_pLeafCardIndices[nLeaf];
            if (!pCard->m_pMesh)
            {
                m_bHasLeafCardGeometry = true;

                for (int nVert = 0; nVert < 6; ++nVert)
                {
                    int nCorner = anVertIndices[nVert];

                    // xy = diffuse texcoords
                    // zw = compressed wind parameters
                    float fWindMatrixIndex1 = float(int(sLeaves.m_pWindMatrixIndices[0][nLeaf] * 10.0f / cWind.GetNumWindMatrices( )));
                    float fWindMatrixWeight1 = c_fWindWeightCompressionRange * sLeaves.m_pWindWeights[0][nLeaf];
                    float fWindMatrixIndex2 = float(int(sLeaves.m_pWindMatrixIndices[1][nLeaf] * 10.0f / cWind.GetNumWindMatrices( )));
                    float fWindMatrixWeight2 = c_fWindWeightCompressionRange * sLeaves.m_pWindWeights[1][nLeaf];
                    cBuffer.TexCoord4(0, pCard->m_pTexCoords[nCorner * 2], pCard->m_pTexCoords[nCorner * 2 + 1], fWindMatrixIndex1 + fWindMatrixWeight1, fWindMatrixIndex2 + fWindMatrixWeight2);

                    // tex layer 1: .x = width, .y = height, .z = pivot x, .w = pivot.y
                    cBuffer.TexCoord4(1, pCard->m_fWidth, pCard->m_fHeight, pCard->m_afPivotPoint[0] - 0.5f, pCard->m_afPivotPoint[1] - 0.5f);

                    // tex layer 2: .x = angle.x, .y = angle.y, .z = leaf angle index [0,c_nNumSpeedWindAngles-1], .w = dimming
                    cBuffer.TexCoord4(2, idv::DegToRad(pCard->m_afAngleOffsets[0]), idv::DegToRad(pCard->m_afAngleOffsets[1]),
                                         float(nLeaf % nNumSpeedWindAngles), sLeaves.m_pDimming[nLeaf]);

#ifdef LEAF_NORMAL_MAPPING
                    // tangent for normal mapping
                    cBuffer.TexCoord3v(3, sLeaves.m_pTangents + nLeaf * 12 + nCorner * 3);
#endif

                    // normal
                    cBuffer.Normalv(sLeaves.m_pNormals + nLeaf * 12 + nCorner * 3);

                    // coordinate
                    cBuffer.Vertex4(sLeaves.m_pCenterCoords[nLeaf * 3 + 0], sLeaves.m_pCenterCoords[nLeaf * 3 + 1], sLeaves.m_pCenterCoords[nLeaf * 3 + 2], float(nCorner));
                }
            }
        }
#endif
    }
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeWrapper::SetupLeafMeshGeometry

void CSpeedTreeWrapper::SetupLeafMeshGeometry(CIdvTightlyPackedBuffer& cBuffer, const CSpeedWind& cWind)
{
    const int nNumSpeedWindAngles = cWind.GetNumLeafAngleMatrices( );

    CSpeedTreeRT::SGeometry sGeometry;
    GetGeometry(sGeometry, SpeedTree_LeafGeometry);

    if (sGeometry.m_nNumLeafLods > 0)
    {
        m_sDrawData.m_pLeafMeshIndexDataStart = st_new_array<int>(sGeometry.m_nNumLeafLods, "CSpeedTreeWrapper::SetupLeafMeshGeometry, m_sDrawData.m_pLeafMeshIndexDataStart");
        m_sDrawData.m_pLeafMeshTriListLengths = st_new_array<int>(sGeometry.m_nNumLeafLods, "CSpeedTreeWrapper::SetupLeafMeshGeometry, m_sDrawData.m_pLeafMeshTriListLengths");

        for (int nLod = 0; nLod < sGeometry.m_nNumLeafLods; ++nLod)
        {
            m_sDrawData.m_pLeafMeshIndexDataStart[nLod] = cBuffer.NumIndices( );

            const CSpeedTreeRT::SGeometry::SLeaf* pLod = sGeometry.m_pLeaves + nLod;

            m_sDrawData.m_pLeafMeshTriListLengths[nLod] = 0;
            for (int nLeaf = 0; nLeaf < pLod->m_nNumLeaves; ++nLeaf)
            {
                const CSpeedTreeRT::SGeometry::SLeaf::SCard* pCard = pLod->m_pCards + pLod->m_pLeafCardIndices[nLeaf];
                const CSpeedTreeRT::SGeometry::SLeaf::SMesh* pMesh = pCard->m_pMesh;

                // does this cluster have a mesh associated with it?
                if (pMesh)
                {
                    const int c_nVertexTableStart = cBuffer.NumVertices( );

                    // setup the vertex attributes
                    for (int nVertex = 0; nVertex < pMesh->m_nNumVertices; ++nVertex)
                    {
                        float fWindMatrixIndex1 = float(int(pLod->m_pWindMatrixIndices[0][nLeaf] * 10.0f / cWind.GetNumWindMatrices( )));
                        float fWindMatrixWeight1 = c_fWindWeightCompressionRange * pLod->m_pWindWeights[0][nLeaf];
                        float fWindMatrixIndex2 = float(int(pLod->m_pWindMatrixIndices[1][nLeaf] * 10.0f / cWind.GetNumWindMatrices( )));
                        float fWindMatrixWeight2 = c_fWindWeightCompressionRange * pLod->m_pWindWeights[1][nLeaf];

                        // tex layer 0: .xy = diffuse texcoords, .z = leaf angle index [0,c_nNumSpeedWindAngles-1], .w = dimming
                        cBuffer.TexCoord4(0, pMesh->m_pTexCoords[nVertex * 2], pMesh->m_pTexCoords[nVertex * 2 + 1],
                                             float(nLeaf % nNumSpeedWindAngles), pLod->m_pDimming[nLeaf]);

                        // orientation vectors
#ifdef UPVECTOR_POS_Y
                        cBuffer.TexCoord3(1, pLod->m_pTangents[nLeaf * 12 + 0], pLod->m_pNormals[nLeaf * 12 + 0], pLod->m_pBinormals[nLeaf * 12 + 0]);
                        cBuffer.TexCoord3(2, pLod->m_pTangents[nLeaf * 12 + 2], pLod->m_pNormals[nLeaf * 12 + 2], pLod->m_pBinormals[nLeaf * 12 + 2]);
#else
                        cBuffer.TexCoord3(1, pLod->m_pTangents[nLeaf * 12 + 0], pLod->m_pBinormals[nLeaf * 12 + 0], pLod->m_pNormals[nLeaf * 12 + 0]);
                        cBuffer.TexCoord3(2, pLod->m_pTangents[nLeaf * 12 + 2], pLod->m_pBinormals[nLeaf * 12 + 2], pLod->m_pNormals[nLeaf * 12 + 2]);
#endif

                        // offset (used to position the mesh on the tree)
                        cBuffer.TexCoord4(3, pLod->m_pCenterCoords[nLeaf * 3 + 0], pLod->m_pCenterCoords[nLeaf * 3 + 1], pLod->m_pCenterCoords[nLeaf * 3 + 2], fWindMatrixIndex2 + fWindMatrixWeight2);

#ifdef LEAF_NORMAL_MAPPING
                        // tangent for normal mapping
                        cBuffer.TexCoord3v(4, pMesh->m_pTangents + nVertex * 3);
#endif

                        // normal
                        cBuffer.Normalv(pMesh->m_pNormals + nVertex * 3);

                        // coordinate (3d coord of leaf mesh)
                        cBuffer.Vertex4(pMesh->m_pCoords[nVertex * 3 + 0], pMesh->m_pCoords[nVertex * 3 + 1], pMesh->m_pCoords[nVertex * 3 + 2], fWindMatrixIndex1 + fWindMatrixWeight1);
                    }

                    // setup the triangle indices
                    for (int nIndex = 0; nIndex < pMesh->m_nNumIndices; ++nIndex)
                        cBuffer.AddIndex(c_nVertexTableStart + pMesh->m_pIndices[nIndex]);
                    m_sDrawData.m_pLeafMeshTriListLengths[nLod] += pMesh->m_nNumIndices;

                    if (pMesh->m_nNumIndices > 2 && pMesh->m_nNumVertices > 0)
                        m_bHasLeafMeshGeometry = true;
                }
            }
        }
    }
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeWrapper::ComputeCullRadius

void CSpeedTreeWrapper::ComputeCullRadius(void)
{
    // query dimensions
    float afBoundingBox[6];
    GetBoundingBox(afBoundingBox);

    // find tree's center since culling algorithm will use a spherical test
#ifdef UPVECTOR_POS_y
    idv::Vec3 cCenter(0.0f, 0.5f * (afBoundingBox[1] + afBoundingBox[4]), 0.0f);
#else
    idv::Vec3 cCenter(0.0f, 0.0f, 0.5f * (afBoundingBox[2] + afBoundingBox[5]));
#endif

    // compare the distance from the center to the eight corners of the bounding
    // box and use the longest
    float fLongestDistance = 0.0f;
    for (int x = 0; x < 2; ++x)
    {
        for (int y = 0; y < 2; ++y)
        {
            for (int z = 0; z < 2; ++z)
            {
                idv::Vec3 cCorner(afBoundingBox[x * 3], afBoundingBox[y * 3 + 1], afBoundingBox[z * 3 + 2]);

                float fDistance = cCorner.Distance(cCenter);
                fLongestDistance = st_max(fDistance, fLongestDistance);
            }
        }
    }

    m_fCullRadius = fLongestDistance;
}



