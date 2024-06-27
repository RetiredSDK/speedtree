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

#include "IdvTexture.h"
#include "IdvDirectX9.h"
#include "IdvFilename.h"
#ifdef IDV_OPENGL
#ifdef PS3
#include "DDSLoader.h"
#else
#include "IdvDDS.h"
#endif
#endif


// static members
CIdvTexture::st_vector_texcache* CIdvTexture::m_pCache = NULL;
#if IDV_DIRECTX9 | _XBOX
using namespace std;
LPDIRECT3DDEVICE9 CIdvTexture::m_pDx = NULL;
#endif


///////////////////////////////////////////////////////////////////////  
//  CIdvTexture::CIdvTexture

CIdvTexture::CIdvTexture( ) :
      m_pTexture(0)
#if IDV_DIRECTX9 | _XBOX
    , m_eMinFilter(D3DTEXF_LINEAR)
    , m_eMagFilter(D3DTEXF_LINEAR)
#endif
{ 
}


///////////////////////////////////////////////////////////////////////  
//  CIdvTexture::~CIdvTexture

CIdvTexture::~CIdvTexture( )
{
    if (m_pTexture && m_pCache)
    {
        int nCacheIndex = SearchCache(m_strFilename);
        if (nCacheIndex < 0)
        {
            IdvInternal("Cannot find texture [%s] in cache to delete", m_strFilename.c_str( ));
        }
        else
        {
            if (--(*m_pCache)[nCacheIndex].m_nRefCount == 0)
            {
#if IDV_DIRECTX9 | _XBOX
                IdvSafeRelease((*m_pCache)[nCacheIndex].m_pTexture);
#endif
#ifdef IDV_OPENGL
                if (m_pTexture)
                {
                    glDeleteTextures(1, &m_pTexture);
                    m_pTexture = 0;
                }
#endif
                (void) m_pCache->erase(m_pCache->begin( ) + nCacheIndex);
                if (m_pCache->empty( ))
                {
                    st_delete<st_vector_texcache>(m_pCache, "CIdvTexture::~CIdvTexture, m_pCache");
                    m_pCache = NULL;
                }
            }
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  CIdvTexture::Load

bool CIdvTexture::Load(const st_string& strFilename, EFilter eMin, EFilter eMag)
{
    bool bSuccess = false;

    m_strFilename = strFilename;
    if (!m_strFilename.empty( ))
    {
        int nCacheIndex = SearchCache(m_strFilename);
        if (nCacheIndex < 0)
        {

#ifdef REPORT_TIMES
            CIdvTimer cTimer;
            cTimer.Start( );
#endif

#if IDV_DIRECTX9 | _XBOX
            // m_eMinFilter & m_eMagFilter are used in CIdvTexture::Bind()
            m_eMinFilter = D3DTEXF_LINEAR;
            if (eMin == FILTER_NONE)
                m_eMinFilter = D3DTEXF_POINT;

            m_eMagFilter = D3DTEXF_LINEAR;
            if (eMag == FILTER_NONE)
                m_eMagFilter = D3DTEXF_POINT;

            bSuccess = D3DXCreateTextureFromFileExA(m_pDx, m_strFilename.c_str( ), 
                                                    D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, 
                                                    D3DPOOL_MANAGED, D3DX_FILTER_TRIANGLE | D3DX_FILTER_MIRROR, 
                                                    D3DX_FILTER_TRIANGLE | D3DX_FILTER_MIRROR, 0, NULL, NULL, &m_pTexture) == S_OK;
#endif
#ifdef IDV_OPENGL
            GLenum eMinFilter = GL_LINEAR_MIPMAP_LINEAR;
            if (eMin == FILTER_NONE)
                eMinFilter = GL_NEAREST;
            else if (eMin == FILTER_LINEAR)
                eMinFilter = GL_LINEAR;

            GLenum eMagFilter = GL_LINEAR;
            if (eMag == FILTER_NONE)
                eMagFilter = GL_NEAREST;
            else if (eMag == FILTER_LINEAR)
                eMagFilter = GL_LINEAR;

#ifdef PS3
            std::string strTmp = m_strFilename.c_str( );
            m_pTexture = CDDSLoader::LoadFromFile(strTmp);
#else
            m_pTexture = CIdvDDS::LoadFromFile(m_strFilename, eMinFilter, eMagFilter, true);
#endif
            bSuccess = m_pTexture != 0;
#endif

#ifdef REPORT_TIMES
            cTimer.Stop( );
            IdvReport("[%s] load time: %g ms", IdvNoPath(m_strFilename).c_str( ), cTimer.GetMilliSec( ));
#endif

            // cache for possible future use
            if (bSuccess && m_pTexture)
            {
                if (!m_pCache)
                    m_pCache = new ("CIdvTexture::Load, m_pCache") st_vector_texcache;

                SCacheEntry sNewEntry;
                sNewEntry.m_nRefCount = 1;
                sNewEntry.m_pTexture = m_pTexture;
                sNewEntry.m_strFilename = strFilename;
                m_pCache->push_back(sNewEntry);
            }
        }
        else
        {
            (*m_pCache)[nCacheIndex].m_nRefCount++;
            m_pTexture = (*m_pCache)[nCacheIndex].m_pTexture;
            if (m_pTexture)
                bSuccess = true;
        }
    }

    if (!bSuccess)
        (void) LoadWhite( );

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvTexture::Bind

void CIdvTexture::Bind(int nLayer) const
{
#ifdef IDV_OPENGL
    glActiveTextureARB(GL_TEXTURE0_ARB + nLayer);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, m_pTexture);
    glActiveTextureARB(GL_TEXTURE0_ARB);
#endif

#if IDV_DIRECTX9 | _XBOX
    Dx9Verify(m_pDx->SetTexture(nLayer, m_pTexture));
    if (nLayer > -1)
    {
        // The DirectX debug spew will likely tag these as redundant, but we can't make assumptions about
        // what the customer's engine is doing outside of this function.  Change as needed.
        Dx9Verify(m_pDx->SetSamplerState(nLayer, D3DSAMP_MINFILTER, m_eMinFilter));
        Dx9Verify(m_pDx->SetSamplerState(nLayer, D3DSAMP_MIPFILTER, m_eMinFilter));
        Dx9Verify(m_pDx->SetSamplerState(nLayer, D3DSAMP_MAGFILTER, m_eMagFilter));
    }
#ifdef IDV_DIRECTX9
    if (nLayer != 0)
        m_pDx->SetTextureStageState(nLayer, D3DTSS_COLOROP, D3DTOP_MODULATE);
#endif
#endif
}


///////////////////////////////////////////////////////////////////////  
//  CIdvTexture::IsValid

bool CIdvTexture::IsValid(void) const
{
#ifdef IDV_OPENGL
    return m_pTexture != 0;
#endif

#if IDV_DIRECTX9 | _XBOX
    return m_pTexture != NULL;
#endif
}


///////////////////////////////////////////////////////////////////////  
//  CIdvTexture::Disable

void CIdvTexture::Disable(int nLayer)
{
#ifdef IDV_OPENGL
    glActiveTextureARB(GL_TEXTURE0_ARB + nLayer);
    glDisable(GL_TEXTURE_2D);
    glActiveTextureARB(GL_TEXTURE0_ARB);
#endif

#if IDV_DIRECTX9 | _XBOX
    m_pDx->SetTexture(nLayer, NULL);
#endif
}


///////////////////////////////////////////////////////////////////////  
//  CIdvTexture::SearchCache

int CIdvTexture::SearchCache(const st_string& strFilename)
{
    int nIndex = -1;

    if (m_pCache)
    {
        int nCount = int(m_pCache->size( ));
        for (int i = 0; i < nCount; ++i)
            if ((*m_pCache)[i].m_strFilename == strFilename)
                return i;
    }

    return nIndex;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvTexture::D3dColorFill
//
//  This function is to fill up the DirectX texture with whatever color we want

#if IDV_DIRECTX9 | _XBOX
VOID WINAPI D3dColorFill(D3DXVECTOR4* pOut, const D3DXVECTOR2* /*pTexCoord*/, const D3DXVECTOR2* /*pTexelSize*/, LPVOID pData)
{
   D3DCOLORVALUE* pColor = (D3DCOLORVALUE*)pData;
   *pOut = D3DXVECTOR4(pColor->r, pColor->g, pColor->b, pColor->a);
}
#endif


///////////////////////////////////////////////////////////////////////  
//  CIdvTexture::LoadSolidRGBATexture

bool CIdvTexture::LoadSolidRGBATexture(const st_string& strName, unsigned int uiColor)
{
    bool bSuccess = false;

    m_strFilename = strName;
    int nCacheIndex = SearchCache(strName);
    if (nCacheIndex < 0)
    {
        const unsigned int c_uiTextureSize = 4;

#ifdef REPORT_TIMES
        CIdvTimer cTimer;
        cTimer.Start( );
#endif

#if IDV_DIRECTX9 | _XBOX
        m_eMinFilter = D3DTEXF_POINT;
        m_eMagFilter = D3DTEXF_POINT;
        D3DCOLORVALUE d3dColor;
        d3dColor.r = float((uiColor & 0xff000000) >> 24) / 255.0f;
        d3dColor.g = float((uiColor & 0x00ff0000) >> 16) / 255.0f;
        d3dColor.b = float((uiColor & 0x0000ff00) >> 8) / 255.0f;
        d3dColor.a = float((uiColor & 0x000000ff) >> 0) / 255.0f;
        bSuccess = D3DXCreateTexture(m_pDx, c_uiTextureSize, c_uiTextureSize, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &m_pTexture) == S_OK &&
                   D3DXFillTexture(m_pTexture, D3dColorFill, &d3dColor) == S_OK;
#endif
#ifdef IDV_OPENGL
        glGenTextures(1, &m_pTexture);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, m_pTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        st_vector_uint vTexture;
#ifndef PS3
        uiColor = ((uiColor & 0x000000ff) << 24) + 
                  ((uiColor & 0x0000ff00) << 8) + 
                  ((uiColor & 0x00ff0000) >> 8) + 
                  ((uiColor & 0xff000000) >> 24);
#endif
        vTexture.resize(c_uiTextureSize * c_uiTextureSize, uiColor);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, c_uiTextureSize, c_uiTextureSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, &vTexture[0]);
    
        bSuccess = m_pTexture != 0;
#endif

#ifdef REPORT_TIMES
        cTimer.Stop( );
        IdvReport("[%s] load time: %g ms", strName.c_str( ), cTimer.GetMilliSec( ));
#endif

        // cache for possible future use
        if (bSuccess && m_pTexture)
        {
            if (!m_pCache)
                m_pCache = new ("CIdvTexture::Load, m_pCache") st_vector_texcache;

            SCacheEntry sNewEntry;
            sNewEntry.m_nRefCount = 1;
            sNewEntry.m_pTexture = m_pTexture;
            sNewEntry.m_strFilename = strName;
            m_pCache->push_back(sNewEntry);
        }
    }
    else
    {
        (*m_pCache)[nCacheIndex].m_nRefCount++;
        m_pTexture = (*m_pCache)[nCacheIndex].m_pTexture;
        if (m_pTexture)
            bSuccess = true;
    }

    return bSuccess;
}


