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

#include "IdvState.h"


// static members

#if IDV_DIRECTX9 | _XBOX
LPDIRECT3DDEVICE9 CIdvState::m_pDx = NULL;
#endif


///////////////////////////////////////////////////////////////////////  
//  CIdvState::SetBlending

void CIdvState::SetBlending(bool bFlag)
{
    if (bFlag)
    {
#ifdef IDV_OPENGL
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
#if IDV_DIRECTX9 | _XBOX
        st_assert(m_pDx);
        m_pDx->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        m_pDx->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        m_pDx->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
#endif
    }
    else
    {
#ifdef IDV_OPENGL
        glDisable(GL_BLEND);
#endif
#if IDV_DIRECTX9 | _XBOX
        st_assert(m_pDx);
        m_pDx->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
#endif
    }
}


///////////////////////////////////////////////////////////////////////  
//  CIdvState::SetAlphaFunction

void CIdvState::SetAlphaFunction(EAlphaFunc eFunc, float fValue)
{
#ifdef IDV_OPENGL
    if (eFunc == GREATER)
        glAlphaFunc(GL_GREATER, fValue / 255.0f);
    else
        glAlphaFunc(GL_LESS, fValue / 255.0f);
#endif

#if IDV_DIRECTX9 | _XBOX
    st_assert(m_pDx);
    m_pDx->SetRenderState(D3DRS_ALPHAREF, DWORD(fValue));
    if (eFunc == GREATER)
        m_pDx->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
    else
        m_pDx->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_LESS);
#endif
}


///////////////////////////////////////////////////////////////////////  
//  CIdvState::SetAlphaTesting

void CIdvState::SetAlphaTesting(bool bFlag)
{
#ifdef IDV_OPENGL
    if (bFlag)
        glEnable(GL_ALPHA_TEST);
    else
        glDisable(GL_ALPHA_TEST);
#endif

#if IDV_DIRECTX9 | _XBOX
    st_assert(m_pDx);
    m_pDx->SetRenderState(D3DRS_ALPHATESTENABLE, bFlag ? TRUE : FALSE);
#endif
}


///////////////////////////////////////////////////////////////////////  
//  CIdvState::SetFaceCulling

void CIdvState::SetFaceCulling(ECullType eCullType)
{
#ifdef IDV_OPENGL
    if (eCullType == NONE)
        glDisable(GL_CULL_FACE);
    else 
    {
        glEnable(GL_CULL_FACE);
        if (eCullType == BACK)
            glCullFace(GL_BACK);
        else
            glCullFace(GL_FRONT);
    }
#endif

#if IDV_DIRECTX9 | _XBOX
    st_assert(m_pDx);
    if (eCullType == BACK)
        m_pDx->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
    else if (eCullType == FRONT)
        m_pDx->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    else
        m_pDx->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
#endif
}


///////////////////////////////////////////////////////////////////////  
//  CIdvState::SetRenderStyle

void CIdvState::SetRenderStyle(ERenderStyle eStyle)
{
    if (eStyle == WIREFRAME)
    {
#ifdef IDV_OPENGL
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
#endif

#if IDV_DIRECTX9 | _XBOX
        st_assert(m_pDx);
        m_pDx->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
#endif
    }
    else
    {
#ifdef IDV_OPENGL
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif

#if IDV_DIRECTX9 | _XBOX
        st_assert(m_pDx);
        m_pDx->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
#endif
    }
}

