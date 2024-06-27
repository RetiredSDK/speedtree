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

#include "stl_vector.h"
#include "Environment.h"
#include "IdvGlobal.h"
#include "IdvState.h"
#include "IdvFilename.h"
#include "UpVector.h"
using namespace std;


///////////////////////////////////////////////////////////////////////  
//  Constants

const int c_nGroundRes = 10;
const float c_fTexRepeat = 40.0f;


///////////////////////////////////////////////////////////////////////  
//  SVertex

struct SVertex
{
    void    Set(float x, float y, float z)
    {
        m_afCoords[0] = x;
        m_afCoords[1] = y;
        m_afCoords[2] = z;
        m_afCoords[3] = 1.0f;
        m_afTexCoords[0] = x / c_fTexRepeat;
#ifdef UPVECTOR_POS_Y
        m_afTexCoords[1] = z / c_fTexRepeat;
#else
        m_afTexCoords[1] = y / c_fTexRepeat;
#endif
    }

    float   m_afCoords[4];
    float   m_afTexCoords[2];
};


///////////////////////////////////////////////////////////////////////  
//  CEnvironment::CEnvironment

CEnvironment::CEnvironment( ) :
    m_nNumVerts(0),
    m_fFogStart(-1.0f),
    m_fFogEnd(-1.0f)
{
}


///////////////////////////////////////////////////////////////////////  
//  CEnvironment::~CEnvironment

CEnvironment::~CEnvironment( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CEnvironment::IsInitialized

bool CEnvironment::IsInitialized(void) const
{
    return m_cGeometry.NumVertices( ) > 0;
}


///////////////////////////////////////////////////////////////////////  
//  CEnvironment::Init

bool CEnvironment::Init(const st_string& strResourcePath, 
                        const st_string& strShaderFile, 
                        float fFogStart, 
                        float fFogEnd,
                        const float* pFogColor)
{
    bool bSuccess = false;

    // fog
    m_fFogStart = fFogStart;
    m_fFogEnd = fFogEnd;
    m_cFogColor.Set(pFogColor[0], pFogColor[1], pFogColor[2]);

    // load textures
    st_string strGroundFilename = strResourcePath + "Ground.dds";
    bSuccess = m_texGround.Load(strGroundFilename);
    if (!bSuccess)
        IdvWarning("Failed to load texture [%s]\n", strGroundFilename.c_str( ));

    // init geometry
    SetupIndices( );
    m_cGeometry.SetStride(sizeof(SVertex));
    m_cGeometry.SetVertexOffset(4, 0);
    m_cGeometry.SetTexCoordOffset(0, 2, 4 * sizeof(float));
    m_cGeometry.SetMaxNumVertices(c_nGroundRes * c_nGroundRes);

    // load shader
    bSuccess = LoadShader(strShaderFile);

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CEnvironment::Draw
//
//  The ground can't be drawn as a single quad due to per-vertex fog computations.

void CEnvironment::Draw(const float* pExtents)
{
    // setup the render state
    CIdvState::SetBlending(true);
    CIdvState::SetAlphaTesting(false);

    // update shader constants
    (void) m_conWorldViewProjectionMatrix.SetModelviewProjectionMatrix( );
    (void) m_conTexture.SetTexture(m_texGround);
    (void) m_conColor.Set4f(1.0f, 1.0f, 1.0f, 1.0f);
    (void) m_conFogColor.Set3fv(m_cFogColor);
    (void) m_conFogParams.Set3f(m_fFogStart, m_fFogEnd, m_fFogEnd - m_fFogStart);
#ifndef LOAD_BINARY_SHADERS
    (void) m_cEffect.CommitChanges( );
#endif

    // update geometry buffers
    static SVertex asGrid[c_nGroundRes * c_nGroundRes];
    float fDeltaX = (pExtents[3] - pExtents[0]) / float(c_nGroundRes - 1);
#ifdef UPVECTOR_POS_Y
    float fDeltaZ = (pExtents[5] - pExtents[2]) / float(c_nGroundRes - 1);
#else
    float fDeltaY = (pExtents[4] - pExtents[1]) / float(c_nGroundRes - 1);
#endif

    SVertex* pVertex = asGrid;
#ifdef UPVECTOR_POS_Y
    float z = pExtents[2];
#else
    float y = pExtents[1];
#endif
    for (int i = 0; i < c_nGroundRes; ++i)
    {
        float x = pExtents[0];
        for (int j = 0; j < c_nGroundRes; ++j)
        {
#ifdef UPVECTOR_POS_Y
            pVertex->Set(x, 0.0f, z);
#else
            pVertex->Set(x, y, 0.0f);
#endif
            x += fDeltaX;
            ++pVertex;
        }
#ifdef UPVECTOR_POS_Y
        z += fDeltaZ;
#else
        y += fDeltaY;
#endif
    }
    st_assert(pVertex - asGrid == c_nGroundRes * c_nGroundRes);
    m_cGeometry.SetBuffer((void*) asGrid, c_nGroundRes * c_nGroundRes);

    m_cTechnique.Begin( );
    if (m_cGeometry.BindVertexBuffer(true))
    {
        if (m_cGeometry.BindIndexBuffer( ))
        {
            (void) m_cGeometry.DrawIndexed(CIdvGeometryBuffer::TRIANGLE_STRIP, 0, m_nNumVerts);
            (void) m_cGeometry.UnBindIndexBuffer( );
        }
        (void) m_cGeometry.UnBindVertexBuffer(true);
    }
    m_cTechnique.End( );

    // restore the render state
    CIdvState::SetBlending(false);
    CIdvState::SetAlphaTesting(true);
}


///////////////////////////////////////////////////////////////////////  
//  CEnvironment::LoadShader

#ifndef LOAD_BINARY_SHADERS
bool CEnvironment::LoadShader(const st_string& strShaderFile)
{
    bool bSuccess = false;

    // setup shader #defines - vector is formatted in pairs of STL strings - first of pair is macro name, second is macro value
    st_vector_string vShaderDefines;
#ifdef UPVECTOR_POS_Y
    vShaderDefines.push_back("UPVECTOR_POS_Y");
    vShaderDefines.push_back("Doesn't matter");
#endif
#ifdef IDV_OPENGL
    vShaderDefines.push_back("IDV_OPENGL");
    vShaderDefines.push_back("Doesn't matter");
#endif
#ifdef IDV_DIRECTX9
    vShaderDefines.push_back("IDV_DIRECTX9");
    vShaderDefines.push_back("Doesn't matter");
#endif
    if (m_fFogStart != -1.0f && m_fFogEnd != -1.0f)
    {
        vShaderDefines.push_back("USE_FOG");
        vShaderDefines.push_back("Doesn't matter");
    }

    // load shader
    st_string strShaderError = m_cEffect.Load(strShaderFile, vShaderDefines);
    if (strShaderError.empty( ))
    {
        // find techniques
        m_cTechnique = m_cEffect.GetTechnique("Ground");
        st_assert(m_cTechnique.IsValid( ));

        // find constants
        m_conWorldViewProjectionMatrix = m_cEffect.GetConstant("g_mModelViewProj");
        m_conColor = m_cEffect.GetConstant("g_vGroundColor");
#ifdef IDV_OPENGL
        m_conTexture = m_cEffect.GetConstant("samGroundMap");
#else
        m_conTexture = m_cEffect.GetConstant("g_tGroundMap");
#endif
        m_conFogColor = m_cEffect.GetConstant("g_vFogColor");
        m_conFogParams = m_cEffect.GetConstant("g_vFogParams");

        bSuccess = true;
    }
    else
        IdvError("Failed to load FX file [%s]: %s\n", strShaderFile.c_str( ), strShaderError.c_str( ));

    return bSuccess;
}

#else // !LOAD_BINARY_SHADERS

// PLAYSTATION3 version
bool CEnvironment::LoadShader(const st_string& strShaderFile)
{
    bool bSuccess = false;

    // load shaders
    st_string strPath = IdvPath(strShaderFile);

#ifdef IDV_OPENGL
#ifdef PS3
    strPath += "/PS3";
#else
    strPath += "/PC_OpenGL";
#endif
#endif

    printf("Load shaders from path [%s]\n", strPath.c_str( ));

    // ground
    st_string strError = m_cTechnique.LoadBinary(strPath + "/Ground.vpo", strPath + "/Ground.fpo");

    if (!strError.empty( ))
    {
        printf("failed to load shaders: %s\n", strError.c_str( ));
    }
    else
    {
        // shared parameters
        m_conWorldViewProjectionMatrix = m_cTechnique.GetVertexConstant("g_mModelViewProj");
        m_conColor = m_cTechnique.GetPixelConstant("g_vGroundColor");
#ifdef IDV_OPENGL
        m_conTexture = m_cTechnique.GetPixelConstant("samGroundMap");
#else
        m_conTexture = m_cTechnique.GetPixelConstant("g_tGroundMap");
#endif
        m_conFogColor = m_cTechnique.GetPixelConstant("g_vFogColor");
        m_conFogParams = m_cTechnique.GetVertexConstant("g_vFogParams");

        bSuccess = true;
    }

    return bSuccess;
}
#endif


///////////////////////////////////////////////////////////////////////  
//  CEnvironment::SetupIndices

void CEnvironment::SetupIndices(void)
{
    int nIndex = 0;
    vector<int> vIndices;
    for (int i = 0; i < c_nGroundRes - 1; ++i)
    {
        // one row
        for (int j = 0; j < c_nGroundRes; ++j)
        {
            vIndices.push_back(nIndex);
            vIndices.push_back(nIndex + c_nGroundRes);
            ++nIndex;
        }

        // stitch
        if (i < c_nGroundRes - 1)
        {
            int nLastIndexPrevStrip = vIndices.back( );
            int nFirstIndexSecondStrip = nIndex;
            int nSecondIndexSecondStrip = nIndex + c_nGroundRes;
            vIndices.push_back(nLastIndexPrevStrip);
            vIndices.push_back(nFirstIndexSecondStrip);
            vIndices.push_back(nFirstIndexSecondStrip);
            vIndices.push_back(nSecondIndexSecondStrip);
        }
    }

    m_nNumVerts = int(vIndices.size( ));
    m_cGeometry.AddIndices(&vIndices[0], m_nNumVerts);
    m_cGeometry.FinishIndices( );
}



