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

#pragma once
#ifdef IDV_OPENGL
#include "IdvOpenGL.h"
#endif
#if IDV_DIRECTX9 | _XBOX
#include "IdvDirectX9.h"
#endif


///////////////////////////////////////////////////////////////////////  
//  class CIdvState declaration

class CIdvState
{
public:
        enum EAlphaFunc
        {
            LESS,
            GREATER
        };
        enum ECullType
        {
            NONE,
            FRONT,
            BACK
        };
        enum ERenderStyle
        {
            SOLID,
            WIREFRAME
        };

static  void                SetBlending(bool bFlag);
static  void                SetAlphaFunction(EAlphaFunc eFunc, float fValue);   // fValue range = [0.0,255.0]
static  void                SetAlphaTesting(bool bFlag);
static  void                SetFaceCulling(ECullType eCullType);
static  void                SetRenderStyle(ERenderStyle eStyle);

#if IDV_DIRECTX9 | _XBOX
static  void                SetDevice(LPDIRECT3DDEVICE9 pDx)                { m_pDx = pDx; }
static  LPDIRECT3DDEVICE9   Device(void)                                    { return m_pDx; }

static  LPDIRECT3DDEVICE9   m_pDx;
#endif
};
