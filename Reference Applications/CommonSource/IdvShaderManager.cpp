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

#include "IdvGlobal.h"
#include "IdvShaderManager.h"
#include "IdvTexture.h"
#include "IdvFilename.h"
#if defined(WIN32)// && defined(IDV_OPENGL)
#include "direct.h"
#endif
using namespace std;


///////////////////////////////////////////////////////////////////////  
//  Static members

#if IDV_DIRECTX9 | _XBOX
LPDIRECT3DDEVICE9 CIdvEffect::m_pDx = NULL;
D3DXMATRIX CIdvConstant::m_matModelviewProjectionMatrix;
#endif

#ifdef IDV_OPENGL
#ifndef LOAD_BINARY_SHADERS
CGcontext CIdvEffect::m_pContext = NULL;
#endif // LOAD_BINARY_SHADERS
idv::Transform CIdvConstant::m_cModelviewProjectionMatrix;
#endif // IDV_OPENGL

#ifdef LOAD_BINARY_SHADERS
#ifdef PS3
#define VertexProfile cgGLGetLatestProfile(CG_GL_VERTEX)
#define FragmentProfile cgGLGetLatestProfile(CG_GL_FRAGMENT)
#else
#define VertexProfile CG_PROFILE_VP40
#define FragmentProfile CG_PROFILE_FP40
#endif
#endif

///////////////////////////////////////////////////////////////////////  
//  CIdvTechnique::CIdvTechnique

CIdvTechnique::CIdvTechnique( )
#ifdef LOAD_BINARY_SHADERS
    : m_cgVertexShader(NULL)
    , m_cgPixelShader(NULL)
#else
#if IDV_DIRECTX9 | _XBOX
    : m_pDx9Effect(NULL)
    , m_hTechnique(NULL)
#endif
#ifdef IDV_OPENGL
    : m_pTechnique(NULL)
    , m_pPass(NULL)
#endif
#endif // LOAD_BINARY_SHADERS
{
}


///////////////////////////////////////////////////////////////////////  
//  CIdvTechnique::~CIdvTechnique

CIdvTechnique::~CIdvTechnique( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CIdvTechnique::Begin

#ifdef LOAD_BINARY_SHADERS
bool CIdvTechnique::Begin(int)
#else
bool CIdvTechnique::Begin(int nPass)
#endif
{
    bool bSuccess = false;

    if (IsValid( ))
    {
#ifdef LOAD_BINARY_SHADERS
        cgGLEnableProfile(VertexProfile);
        cgGLBindProgram(m_cgVertexShader);
        cgGLEnableProfile(FragmentProfile);
        cgGLBindProgram(m_cgPixelShader);

        bSuccess = true;
#else
#if IDV_DIRECTX9 | _XBOX
        if (SUCCEEDED(m_pDx9Effect->SetTechnique(m_hTechnique)))
        {
            unsigned int uiNumPasses = 0;
            if (SUCCEEDED(m_pDx9Effect->Begin(&uiNumPasses, 0)))
            {
                if (nPass < int(uiNumPasses))
                {
                    if (SUCCEEDED(m_pDx9Effect->BeginPass(nPass)))
                        bSuccess = true;
                }
                else
                    IdvError("CIdvTechnique::Begin, only %d passes defined, cannot bind pass %d", uiNumPasses, nPass);
            }
        }
#endif
#ifdef IDV_OPENGL
        m_pPass = cgGetFirstPass(m_pTechnique);
        for (int i = 0; i < nPass; ++i)
            m_pPass = cgGetNextPass(m_pPass);

        if (m_pPass)
        {
            cgSetPassState(m_pPass);
            bSuccess = true;
        }
        else
            IdvError("CIdvTechnique::Begin, pass %d failure", nPass);
#endif
#endif // LOAD_BINARY_SHADERS
    }
    if (!bSuccess)
        IdvError("CIdvTechnique::Begin() failed for [%s]", Name( ).c_str( ));

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvTechnique::Begin

bool CIdvTechnique::End(void)
{
    bool bSuccess = false;

    if (IsValid( ))
    {
#ifdef LOAD_BINARY_SHADERS
        bSuccess = true;
#else // LOAD_BINARY_SHADERS
#if IDV_DIRECTX9 | _XBOX
        if (SUCCEEDED(m_pDx9Effect->EndPass( )))
        {
            if (SUCCEEDED(m_pDx9Effect->End( )))
            {
                bSuccess = true;
            }
        }
#endif // IDV_DIRECTX9 | _XBOX
#ifdef IDV_OPENGL
        m_pPass = NULL;
        bSuccess = true; // cgResetPassState returns no error code
#endif // IDV_OPENGL
#endif // LOAD_BINARY_SHADERS
    }
    if (!bSuccess)
        IdvError("CIdvTechnique::End() failed\n");

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvTechnique::IsValid

bool CIdvTechnique::IsValid(void) const
{
#ifdef LOAD_BINARY_SHADERS
    return m_cgVertexShader && m_cgPixelShader;
#else // LOAD_BINARY_SHADERS
#if IDV_DIRECTX9 | _XBOX
    return m_hTechnique != NULL && m_pDx9Effect != NULL;
#endif
#ifdef IDV_OPENGL
    return m_pTechnique != NULL;
#endif
#endif // LOAD_BINARY_SHADERS
}


///////////////////////////////////////////////////////////////////////  
//  CIdvTechnique::LoadBinary

#ifdef LOAD_BINARY_SHADERS
st_string CIdvTechnique::LoadBinary(const st_string& strVertexShader, const st_string& strPixelShader)
{
    // error code
    st_string strError;

    // make sure the shader files exist
    if (!IdvFileExists(strVertexShader))
        strError = IdvFormatString("Failed to load binary vertex shader: [%s] - file most likely missing", strVertexShader.c_str( ));
    else if (!IdvFileExists(strPixelShader))
        strError = IdvFormatString("Failed to load binary pixel shader: [%s] - file most likely missing", strPixelShader.c_str( ));
    else
    {
        static CGcontext s_cgContext = NULL;
        if (!s_cgContext)
            s_cgContext = cgCreateContext( );

        // load the vertex shader
        if (!strVertexShader.empty( ))
        {
#ifdef PS3
            m_cgVertexShader = cgCreateProgramFromFile(s_cgContext, CG_BINARY, strVertexShader.c_str( ), VertexProfile, NULL, NULL);
#else
            m_cgVertexShader = cgCreateProgramFromFile(s_cgContext, CG_OBJECT, strVertexShader.c_str( ), VertexProfile, NULL, NULL);
#endif
            CGerror cgError = cgGetError( );
            if (cgError != CG_NO_ERROR || !m_cgVertexShader)
            {
                strError = IdvFormatString("Failed to load vertex shader [%s], Cg error code %d (%s)\n", strVertexShader.c_str( ), cgError, cgGetErrorString(cgError)); 
                IdvError(strError.c_str( ));
                m_cgVertexShader = NULL;
            }
            else
            {
                cgGLLoadProgram(m_cgVertexShader);
                cgGLEnableProfile(VertexProfile);
            }
        }

        // load the pixel shader
        if (!strPixelShader.empty( ) && strError.empty( ))
        {
#ifdef PS3
            m_cgPixelShader = cgCreateProgramFromFile(s_cgContext, CG_BINARY, strPixelShader.c_str( ), FragmentProfile, NULL, NULL);
#else
            m_cgPixelShader = cgCreateProgramFromFile(s_cgContext, CG_OBJECT, strPixelShader.c_str( ), FragmentProfile, NULL, NULL);
#endif
            CGerror cgError = cgGetError( );
            if (cgError != CG_NO_ERROR || !m_cgPixelShader)
            {
                strError = IdvFormatString("Failed to load pixel shader[%s], Cg error code %d (%s)\n", strPixelShader.c_str( ), cgError, cgGetErrorString(cgError));
                IdvError(strError.c_str( ));
                m_cgPixelShader = NULL;
            }
            else
            {
                cgGLLoadProgram(m_cgPixelShader);
                cgGLEnableProfile(FragmentProfile);
            }
        }
    }

    if (strError.empty( ))
    {
        printf("successfully loaded shader pair:\n\t[%s]\n\t[%s]\n",
            strVertexShader.c_str( ), strPixelShader.c_str( ));
    }

    return strError;
}
#endif // LOAD_BINARY_SHADERS


///////////////////////////////////////////////////////////////////////  
//  CIdvTechnique::GetVertexConstant

#ifdef LOAD_BINARY_SHADERS
CIdvConstant CIdvTechnique::GetVertexConstant(const char* pName) const
{
    CIdvConstant cConstant(pName);

    if (m_cgVertexShader)
    {
        cConstant.m_pParameter = cgGetNamedParameter(m_cgVertexShader, pName);

        CGerror cgError = cgGetError( );
        if (cgError != CG_NO_ERROR || !cConstant.m_pParameter)
        {
            IdvError("Failed to find vertex constant [%s] - ", pName);
            cConstant.m_pParameter = NULL;
        }
    }

    return cConstant;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvTechnique::GetPixelConstant

CIdvConstant CIdvTechnique::GetPixelConstant(const char* pName) const
{
    CIdvConstant cConstant(pName);

    if (m_cgPixelShader)
    {
        cConstant.m_pParameter = cgGetNamedParameter(m_cgPixelShader, pName);

        CGerror cgError = cgGetError( );
        if (cgError != CG_NO_ERROR || !cConstant.m_pParameter)
        {
            IdvError("Failed to find pixel constant [%s] - ", pName);
            cConstant.m_pParameter = NULL;
        }
    }

    return cConstant;
}

#endif // !LOAD_BINARY_SHADERS


///////////////////////////////////////////////////////////////////////  
//  CIdvConstant::CIdvConstant

CIdvConstant::CIdvConstant(st_string strName) :
#if IDV_DIRECTX9 | _XBOX
      m_pDx9Effect(NULL)
    , m_hParameter(NULL)
#endif
#ifdef IDV_OPENGL
      m_pParameter(NULL)
#endif
    , m_strName(strName)
{
}


///////////////////////////////////////////////////////////////////////  
//  CIdvConstant::~CIdvConstant

CIdvConstant::~CIdvConstant( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CIdvConstant::IsValid

inline bool CIdvConstant::IsValid(void) const
{
#if IDV_DIRECTX9 | _XBOX
    return m_hParameter != NULL && m_pDx9Effect != NULL;
#endif
#ifdef IDV_OPENGL
    return m_pParameter != NULL;
#endif
}


///////////////////////////////////////////////////////////////////////  
//  CIdvConstant::Set1f

bool CIdvConstant::Set1f(float x) const
{
    bool bSuccess = false;

    if (IsValid( ))
    {
#if IDV_DIRECTX9 | _XBOX
        bSuccess = SUCCEEDED(m_pDx9Effect->SetFloat(m_hParameter, x));
#endif
#ifdef IDV_OPENGL
        cgGLSetParameter1f(m_pParameter, x);
        bSuccess = true;
#endif
    }
    else
        IdvWarning("Set1f() failure for [%s]", GetName( ).c_str( ));

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvConstant::Set2f

bool CIdvConstant::Set2f(float x, float y) const
{
    bool bSuccess = false;

    if (IsValid( ))
    {
#if IDV_DIRECTX9 | _XBOX
        D3DXVECTOR4 vVector(x, y, 0.0f, 0.0f); 
        bSuccess = SUCCEEDED(m_pDx9Effect->SetVector(m_hParameter, &vVector));
#endif
#ifdef IDV_OPENGL
        cgGLSetParameter2f(m_pParameter, x, y);
        bSuccess = true;
#endif
    }
    else
        IdvWarning("Set2f() failure for [%s]", GetName( ).c_str( ));

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvConstant::Set2fv

bool CIdvConstant::Set2fv(const float* pValues) const
{
    bool bSuccess = false;

    st_assert(pValues);
    if (IsValid( ))
    {
#if IDV_DIRECTX9 | _XBOX
        D3DXVECTOR4 vVector(pValues[0], pValues[1], 0.0f, 0.0f); 
        bSuccess = SUCCEEDED(m_pDx9Effect->SetVector(m_hParameter, &vVector));
#endif
#ifdef IDV_OPENGL
        cgGLSetParameter2fv(m_pParameter, pValues);
        bSuccess = true;
#endif
    }
    else
        IdvWarning("Set2fv() failure for [%s]", GetName( ).c_str( ));

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvConstant::Set3f

bool CIdvConstant::Set3f(float x, float y, float z) const
{
    bool bSuccess = false;

    if (IsValid( ))
    {
#if IDV_DIRECTX9 | _XBOX
        D3DXVECTOR4 vVector(x, y, z, 0.0f); 
        bSuccess = SUCCEEDED(m_pDx9Effect->SetVector(m_hParameter, &vVector));
#endif
#ifdef IDV_OPENGL
        cgGLSetParameter3f(m_pParameter, x, y, z);
        bSuccess = true;
#endif
    }
    else
        IdvWarning("Set3f() failure for [%s]", GetName( ).c_str( ));

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvConstant::Set3fv

bool CIdvConstant::Set3fv(const float* pValues) const
{
    bool bSuccess = false;

    st_assert(pValues);
    if (IsValid( ))
    {
#if IDV_DIRECTX9 | _XBOX
        D3DXVECTOR4 vVector(pValues[0], pValues[1], pValues[2], 0.0f); 
        bSuccess = SUCCEEDED(m_pDx9Effect->SetVector(m_hParameter, &vVector));
#endif
#ifdef IDV_OPENGL
        cgGLSetParameter3fv(m_pParameter, pValues);
        bSuccess = true;
#endif
    }
    else
        IdvWarning("Set3fv() failure for [%s]", GetName( ).c_str( ));

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvConstant::Set4f

bool CIdvConstant::Set4f(float x, float y, float z, float w) const
{
    bool bSuccess = false;

    if (IsValid( ))
    {
#if IDV_DIRECTX9 | _XBOX
        D3DXVECTOR4 vVector(x, y, z, w); 
        bSuccess = SUCCEEDED(m_pDx9Effect->SetVector(m_hParameter, &vVector));
#endif
#ifdef IDV_OPENGL
        cgGLSetParameter4f(m_pParameter, x, y, z, w);
        bSuccess = true;
#endif
    }
    else
        IdvWarning("Set4f() failure for [%s]", GetName( ).c_str( ));

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvConstant::Set4fv

bool CIdvConstant::Set4fv(const float* pValues) const
{
    bool bSuccess = false;

    st_assert(pValues);
    if (IsValid( ))
    {
#if IDV_DIRECTX9 | _XBOX
        const D3DXVECTOR4* pVector = reinterpret_cast<const D3DXVECTOR4*>(pValues);
        bSuccess = SUCCEEDED(m_pDx9Effect->SetVector(m_hParameter, pVector));
#endif
#ifdef IDV_OPENGL
        cgGLSetParameter4fv(m_pParameter, pValues);
        bSuccess = true;
#endif
    }
    else
        IdvWarning("Set4fv() failure for [%s]", GetName( ).c_str( ));

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvConstant::SetArray4f

bool CIdvConstant::SetArray4f(const float* pArray, int nSize, int nOffset) const
{
    bool bSuccess = false;

    st_assert(pArray);
    if (IsValid( ))
    {
#if IDV_DIRECTX9 | _XBOX
        D3DXHANDLE hParameter = (nOffset > 0) ? m_pDx9Effect->GetParameterElement(m_hParameter, nOffset) : m_hParameter;
        if (hParameter)
            bSuccess = SUCCEEDED(m_pDx9Effect->SetVectorArray(hParameter, (const D3DXVECTOR4*) pArray, nSize));
#endif
#ifdef IDV_OPENGL

#ifdef PS3
        cgGLSetParameterArray4f(m_pParameter, nOffset, nSize, pArray);
#else
        // alternate code (cgGLSetParameterArray4f wasn't reliable in Cg 1.5 beta)
        for (int i = 0; i < nSize; ++i)
        {
            CGparameter pElement = cgGetArrayParameter(m_pParameter, i + nOffset);
            cgGLSetParameter4fv(pElement, pArray + i * 4);
        }
#endif
        bSuccess = true;
#endif
    }
    else
        IdvWarning("Set4f() failure for [%s]", GetName( ).c_str( ));

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvConstant::SetMatrix

bool CIdvConstant::SetMatrix(const float* pMatrix) const
{
    bool bSuccess = false;

    st_assert(pMatrix);
    if (IsValid( ))
    {
#if IDV_DIRECTX9 | _XBOX
        D3DXMATRIX cMatrix(pMatrix);
        bSuccess = SUCCEEDED(m_pDx9Effect->SetMatrixTranspose(m_hParameter, &cMatrix)); 
#endif
#ifdef IDV_OPENGL
        cgGLSetMatrixParameterfc(m_pParameter, pMatrix);
        bSuccess = true;
#endif
    }
    else
        IdvWarning("SetMatrix() failure for [%s]", GetName( ).c_str( ));

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvConstant::SetMatrixTranspose

bool CIdvConstant::SetMatrixTranspose(const float* pMatrix) const
{
    bool bSuccess = false;

    st_assert(pMatrix);
    if (IsValid( ))
    {
#if IDV_DIRECTX9 | _XBOX
        D3DXMATRIX cMatrix(pMatrix);
        bSuccess = SUCCEEDED(m_pDx9Effect->SetMatrix(m_hParameter, &cMatrix)); 
#endif
#ifdef IDV_OPENGL
        cgGLSetMatrixParameterfr(m_pParameter, pMatrix);
        bSuccess = true;
#endif
    }
    else
        IdvWarning("SetMatrix() failure for [%s]", GetName( ).c_str( ));

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvConstant::SetMatrixIndex

bool CIdvConstant::SetMatrixIndex(const float* pMatrix, int nIndex) const
{
    bool bSuccess = false;

    st_assert(pMatrix);
    if (IsValid( ))
    {
#if IDV_DIRECTX9 | _XBOX
        D3DXMATRIX cMatrix(pMatrix);
        D3DXHANDLE hElement = m_pDx9Effect->GetParameterElement(m_hParameter, nIndex);
        st_assert(hElement);
        bSuccess = SUCCEEDED(m_pDx9Effect->SetMatrix(hElement, &cMatrix)); 
#endif
#ifdef IDV_OPENGL

#ifdef PS3
        cgGLSetMatrixParameterArrayfr(m_pParameter, nIndex, 1, pMatrix);
#else
        // alternate code (cgGLSetParameterArray4f wasn't reliable in Cg 1.5 beta 2)
        CGparameter pElement = cgGetArrayParameter(m_pParameter, nIndex);
        cgGLSetMatrixParameterfr(pElement, pMatrix);
#endif
        bSuccess = true;
#endif
    }
    else
        IdvWarning("SetMatrixIndex() failure for [%s]", GetName( ).c_str( ));

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvConstant::SetModelviewProjectionMatrix

bool CIdvConstant::SetModelviewProjectionMatrix(void) const
{
    bool bSuccess = false;

    if (IsValid( ))
    {
#if IDV_DIRECTX9 | _XBOX
        bSuccess = SUCCEEDED(m_pDx9Effect->SetMatrix(m_hParameter, &m_matModelviewProjectionMatrix));
#endif
#ifdef IDV_OPENGL
        cgGLSetMatrixParameterfc(m_pParameter, (const float*) m_cModelviewProjectionMatrix.m_afData);
        bSuccess = true;
#endif
    }
    else
        IdvWarning("SetModelviewProjectionMatrix() failure for [%s]", GetName( ).c_str( ));

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvConstant::UpdateModelviewProjectionMatrix

void CIdvConstant::UpdateModelviewProjectionMatrix(const float* pProjectionMatrix, const float* pModelviewMatrix)
{
#if IDV_DIRECTX9 | _XBOX
    D3DXMATRIX matTmp;
    D3DXMatrixMultiply(&matTmp, (D3DXMATRIX*) pModelviewMatrix, (D3DXMATRIX*) pProjectionMatrix);
    D3DXMatrixTranspose(&m_matModelviewProjectionMatrix, &matTmp);
#endif
#ifdef IDV_OPENGL
    idv::Transform cProjection, cModelview;
    cProjection.Set(pProjectionMatrix);
    cModelview.Set(pModelviewMatrix);
    m_cModelviewProjectionMatrix = cModelview * cProjection;
#endif
}


///////////////////////////////////////////////////////////////////////  
//  CIdvConstant::UpdateModelviewProjectionMatrix

bool CIdvConstant::SetTexture(const CIdvTexture& cTexture, int nSamplerIndex) const
{
    bool bSuccess = false;

    if (IsValid( ))
    {
#ifdef LOAD_BINARY_SHADERS
        cgGLSetTextureParameter(m_pParameter, cTexture.GetOpenGLTexture( ));
        cgGLEnableTextureParameter(m_pParameter);
#else
    #ifdef IDV_DIRECTX9
        UNREFERENCED_PARAMETER(nSamplerIndex);
        bSuccess = SUCCEEDED(m_pDx9Effect->SetTexture(m_hParameter, cTexture.GetDx9Texture( )));
    #endif
    #ifdef _XBOX
        cTexture.Bind(nSamplerIndex);
    #endif

    #if defined(IDV_OPENGL) && !defined(PS3)
        UNREFERENCED_PARAMETER(nSamplerIndex);
        cgGLSetupSampler(m_pParameter, cTexture.GetOpenGLTexture( ));
        bSuccess = true;
    #endif
#endif
    }
    else
        IdvWarning("SetTexture() failure for [%s]", GetName( ).c_str( ));

    return bSuccess;
}


#ifndef LOAD_BINARY_SHADERS

///////////////////////////////////////////////////////////////////////  
//  CIdvEffect::CIdvEffect

CIdvEffect::CIdvEffect( )
#if IDV_DIRECTX9 | _XBOX
    : m_pDx9Effect(NULL)
#endif
#ifdef IDV_OPENGL
    : m_pCgEffect(NULL)
#endif
{
}


///////////////////////////////////////////////////////////////////////  
//  CIdvEffect::~CIdvEffect

CIdvEffect::~CIdvEffect( )
{
    Release( );
}


///////////////////////////////////////////////////////////////////////  
//  CIdvEffect::Load

st_string CIdvEffect::Load(const st_string& strFilename, const st_vector_string& vShaderDefines)
{
#if IDV_DIRECTX9 | _XBOX
    return LoadShaderDirectX(strFilename, vShaderDefines);
#elif IDV_OPENGL
    return LoadShaderOpenGL(strFilename, vShaderDefines);
#else
#error DirectX9 or OpenGL must be #defined
#endif
}


///////////////////////////////////////////////////////////////////////  
//  CIdvEffect::GetTechnique

CIdvTechnique CIdvEffect::GetTechnique(const char* pName) const
{
    CIdvTechnique cTechnique;

    cTechnique.m_strName = pName;
#if IDV_DIRECTX9 | _XBOX
    if (m_pDx9Effect)
    {
        cTechnique.m_pDx9Effect = m_pDx9Effect;
        cTechnique.m_hTechnique = m_pDx9Effect->GetTechniqueByName(pName);
    }
#endif
#ifdef IDV_OPENGL
    if (m_pCgEffect)
    {
        cTechnique.m_pTechnique = cgGetNamedTechnique(m_pCgEffect, pName);
        if (cTechnique.m_pTechnique)
        {
            if (cgValidateTechnique(cTechnique.m_pTechnique) == CG_FALSE)
                IdvError("Technique [%s] validation failed", pName);
        }
    }

#endif

    return cTechnique;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvEffect::GetConstant

CIdvConstant CIdvEffect::GetConstant(const char* pName) const
{
    CIdvConstant cParameter(IdvNoPath(m_strFilename) + "::" + pName);

#if IDV_DIRECTX9 | _XBOX
    if (m_pDx9Effect)
    {
        cParameter.m_pDx9Effect = m_pDx9Effect;
        cParameter.m_hParameter = m_pDx9Effect->GetParameterByName(NULL, pName);
        if (!cParameter.m_hParameter)
            IdvWarning("Failed to find FX constant [%s]\n", cParameter.m_strName.c_str( ));
    }
#endif
#ifdef IDV_OPENGL
    if (m_pCgEffect)
    {
        cParameter.m_pParameter = cgGetNamedEffectParameter(m_pCgEffect, pName);
        if (!cParameter.m_pParameter)
            IdvWarning("Failed to find FX constant [%s]\n", pName);
    }
    else
        IdvWarning("Failed to find FX constant [%s], invalid effect\n", pName);
#endif

    return cParameter;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvEffect::CommitChanges

bool CIdvEffect::CommitChanges(void) const
{
    bool bSuccess = false;

#if IDV_DIRECTX9 | _XBOX
    bSuccess = SUCCEEDED(m_pDx9Effect->CommitChanges( ));
#endif
#ifdef IDV_OPENGL
    bSuccess = true; // CommitChanges isn't necessary under Cg
#endif

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvEffect::Release

void CIdvEffect::Release(void)
{
#if IDV_DIRECTX9 | _XBOX
    IdvSafeRelease(m_pDx9Effect);
#endif
}


///////////////////////////////////////////////////////////////////////  
//  CIdvEffect::LoadShaderDirectX

#if IDV_DIRECTX9 | _XBOX
st_string CIdvEffect::LoadShaderDirectX(const st_string& strFilename, const st_vector_string& vShaderDefines)
{
    st_string strError;

    m_strFilename = strFilename;
    if (IdvFileExists(strFilename))
    {
        ///////////////////////////////////////////////////////////////////////  
        //  DirectX 9.0 / Xbox 360 version

#if IDV_DIRECTX9 | _XBOX
        // shader debugging stuff
        LPD3DXBUFFER pBuffer = NULL;
        DWORD dwShaderFlags = 0;
#ifdef DEBUG_VS
        dwShaderFlags |= D3DXSHADER_DEBUG | D3DXSHADER_SKIPOPTIMIZATION;
#endif

        // setup shader #defines
        st_assert((vShaderDefines.size( ) % 2) == 0); // must be an even number of strings
        D3DXMACRO* pShaderMacros = st_new_array<D3DXMACRO>(vShaderDefines.size( ) / 2 + 1, "CIdvEffect::Load");
        int i = 0;
        for ( ; i < int(vShaderDefines.size( )); i += 2)
        {
            pShaderMacros[i / 2].Name = vShaderDefines[i].c_str( );
            pShaderMacros[i / 2].Definition = vShaderDefines[i + 1].c_str( );
        }
        // terminate the shader array
        pShaderMacros[i / 2].Name = NULL;
        pShaderMacros[i / 2].Definition = NULL;

#ifdef WIN32
        char szCurrentDir[_MAX_PATH];
        _getcwd(szCurrentDir, _MAX_PATH);

        // attempt to load the CgFX file
        _chdir(IdvPath(strFilename).c_str( ));
#endif

        if (FAILED(D3DXCreateEffectFromFileA(m_pDx, strFilename.c_str( ), pShaderMacros, NULL, dwShaderFlags, NULL, &m_pDx9Effect, &pBuffer)))
        {
            if (pBuffer)
            {
                LPVOID pCompileErrors = pBuffer->GetBufferPointer( );
                strError = (const char*) pCompileErrors;
            }
            IdvSafeRelease(m_pDx9Effect);
        }
        st_delete_array<D3DXMACRO>(pShaderMacros, "CIdvEffect::Load");

#ifdef WIN32
        // restore the previous working dir
        _chdir(szCurrentDir);
#endif

#endif
    }
    else
        strError = IdvFormatString("Shader file [%s] does not exist", strFilename.c_str( ));

    return strError;
}
#endif


///////////////////////////////////////////////////////////////////////  
//  CIdvEffect::LoadShaderOpenGL

#ifdef IDV_OPENGL
st_string CIdvEffect::LoadShaderOpenGL(const st_string& strFilename, const st_vector_string& vShaderDefines)
{
    st_string strError;

    m_strFilename = strFilename;
    if (IdvFileExists(strFilename))
    {
        if (!m_pContext)
        {
            m_pContext = cgCreateContext( );
            if (!m_pContext)
                IdvError("Failed to create Cg context - cannot load shader files");
        }

        if (m_pContext)
        {
            cgGLSetManageTextureParameters(m_pContext, CG_TRUE);

#ifdef PS3
            strError = CreatePS3Effect( );
#else
            cgGLRegisterStates(m_pContext);
            cgGLSetOptimalOptions(cgGLGetLatestProfile(CG_GL_VERTEX));
            cgGLSetOptimalOptions(cgGLGetLatestProfile(CG_GL_FRAGMENT));

            // setup shader #defines
            st_assert((vShaderDefines.size( ) % 2) == 0); // must be an even number of strings
            st_vector_string vShaderMacros;
            int i = 0;
            for (i = 0; i < int(vShaderDefines.size( )); i += 2)
                vShaderMacros.push_back(IdvFormatString("-D%s=%s", vShaderDefines[i].c_str( ), vShaderDefines[i + 1].c_str( )).c_str( ));

            // add IDV_OPENGL
            vShaderMacros.push_back("-DIDV_OPENGL");

            // allocate and fill an array of char*
            const char** pShaderMacros = st_new_array<const char*>(vShaderMacros.size( ) + 1, "CIdvEffect::Load, pShaderMacros");
            for (i = 0; i < int(vShaderMacros.size( )); ++i)
                pShaderMacros[i] = vShaderMacros[i].c_str( );
            pShaderMacros[i] = NULL;

            // cgCreateEffectFromFile doesn't seem to want to find include files that aren't in the current working directory even
            // though cgc.exe will find them OK.  We've put a hack here to push the current working directory, change to the shader's
            // directory & compile, then switch back.
#ifdef WIN32
            char szCurrentDir[_MAX_PATH];
            (void) _getcwd(szCurrentDir, _MAX_PATH);

            // attempt to load the CgFX file
            (void) _chdir(IdvPath(strFilename).c_str( ));
#endif
            m_pCgEffect = cgCreateEffectFromFile(m_pContext, strFilename.c_str( ), (const char**) pShaderMacros);
            if (!m_pCgEffect)
            {
                const char* pListing = cgGetLastListing(m_pContext);
                if (pListing)
                    strError = pListing;
                else
                    strError = "Unknown error";
            }
#ifdef WIN32
            // restore the previous working dir
            (void) _chdir(szCurrentDir);
#endif

            // delete the shader macros
            st_delete_array<const char*>(pShaderMacros, "CIdvEffect::Load, pShaderMacros");
#endif // PS3
        }
    }
    else
        strError = IdvFormatString("Shader file [%s] does not exist", strFilename.c_str( ));

    return strError;
}
#endif


///////////////////////////////////////////////////////////////////////  
//  CIdvEffect::CreatePS3Effect

#ifdef PS3
st_string CIdvEffect::CreatePS3Effect(void)
{
    st_string strError;

    if (m_pContext)
    {
        cgGLRegisterStates(m_pContext);
        m_pCgEffect = cgCreateEffect(m_pContext, "ignored", NULL);
    }
    else
        strError = "Cg context not found";

    return strError;
}
#endif

#endif // !LOAD_BINARY_SHADERS


