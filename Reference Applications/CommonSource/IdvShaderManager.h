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
#ifdef IDV_OPENGL
#include "IdvOpenGL.h"
#endif
#if IDV_DIRECTX9 | _XBOX
#include "IdvDirectX9.h"
#include <D3dx9shader.h>
#endif
#include <cassert>
#include "stl_vector.h"
#include "SpeedTreeMemory.h"
#include "IdvVectorMath.h"


// forward references
class CIdvTexture;


///////////////////////////////////////////////////////////////////////  
//  class CIdvConstant

class CIdvConstant
{
public:
friend  class CIdvEffect;
friend  class CIdvTechnique;

                            CIdvConstant(st_string strName = st_string("Uninitialized"));
                            ~CIdvConstant( );

        bool                Set1f(float x) const;
        bool                Set2f(float x, float y) const;
        bool                Set2fv(const float* pValues) const;
        bool                Set3f(float x, float y, float z) const;
        bool                Set3fv(const float* pValues) const;
        bool                Set4f(float x, float y, float z, float w) const;
        bool                Set4fv(const float* pValues) const;
        bool                SetArray2f(const float* pArray, int nSize) const;
        bool                SetArray4f(const float* pArray, int nSize, int nOffset = 0) const;
        bool                SetMatrix(const float* pMatrix) const;
        bool                SetMatrixTranspose(const float* pMatrix) const;
        bool                SetMatrixIndex(const float* pMatrix, int nIndex) const;
        bool                SetModelviewProjectionMatrix(void) const;
static  void                UpdateModelviewProjectionMatrix(const float* pProjectionMatrix, const float* pModelviewMatrix);
        bool                SetTexture(const CIdvTexture& cTexture, int nSamplerIndex = 0) const;

        bool                IsValid(void) const;
        const st_string&    GetName(void) const     { return m_strName; }

private:
#if IDV_DIRECTX9 | _XBOX
        LPD3DXEFFECT        m_pDx9Effect;
        D3DXHANDLE          m_hParameter;
static  D3DXMATRIX          m_matModelviewProjectionMatrix;
#endif
#ifdef IDV_OPENGL
        CGparameter         m_pParameter;
static  idv::Transform      m_cModelviewProjectionMatrix;
#endif
        st_string           m_strName;
};


///////////////////////////////////////////////////////////////////////  
//  class CIdvTechnique

class CIdvTechnique
{
public:
friend  class CIdvEffect;

                            CIdvTechnique( );
                            ~CIdvTechnique( );

        bool                Begin(int nPass = 0);
        bool                End(void);
        bool                IsValid(void) const;
        const st_string&    Name(void) const        { return m_strName; }

#ifdef LOAD_BINARY_SHADERS
        st_string           LoadBinary(const st_string& stVertexShader, const st_string& stPixelShader);
        CIdvConstant        GetVertexConstant(const char* pName) const;
        CIdvConstant        GetPixelConstant(const char* pName) const;
#endif

private:
        st_string           m_strName;

#ifdef LOAD_BINARY_SHADERS
        CGprogram           m_cgVertexShader;
        CGprogram           m_cgPixelShader;
#else

#if IDV_DIRECTX9 | _XBOX
        LPD3DXEFFECT        m_pDx9Effect;
        D3DXHANDLE          m_hTechnique;
#endif
#ifdef IDV_OPENGL
        CGtechnique         m_pTechnique;
        CGpass              m_pPass;
#endif

#endif
};


///////////////////////////////////////////////////////////////////////  
//  class CIdvEffect

#ifndef LOAD_BINARY_SHADERS
class CIdvEffect
{
public:
                            CIdvEffect( );
                            ~CIdvEffect( );

        st_string           Load(const st_string& stFilename, const st_vector_string& vShaderDefines);

        CIdvTechnique       GetTechnique(const char* pName) const;
        CIdvConstant        GetConstant(const char* pName) const;

        bool                CommitChanges(void) const;
        void                Release(void);

#if IDV_DIRECTX9 | _XBOX
static  void                SetDevice(LPDIRECT3DDEVICE9 pDx)                { m_pDx = pDx; }
static  LPDIRECT3DDEVICE9   GetDevice(void)                                 { return m_pDx; }
#endif

private:
#ifdef PS3
        st_string           CreatePS3Effect(void);
#endif

        st_string           m_strFilename;

#if IDV_DIRECTX9 | _XBOX
        st_string           LoadShaderDirectX(const st_string& strFilename, const st_vector_string& vShaderDefines);
        LPD3DXEFFECT        m_pDx9Effect;
static  LPDIRECT3DDEVICE9   m_pDx;
#endif
#ifdef IDV_OPENGL
        st_string           LoadShaderOpenGL(const st_string& strFilename, const st_vector_string& vShaderDefines);
        CGeffect            m_pCgEffect;
static  CGcontext           m_pContext;
#endif
};
#endif // !LOAD_BINARY_SHADERS

