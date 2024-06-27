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
#include "IdvGlobal.h"
#ifdef IDV_DIRECTX9
#include <d3d9.h>
#include <d3dx9.h>
#include <dxerr.h>
#endif
#ifdef _XBOX
#include <xtl.h>
#include <xgraphics.h>
#include <dxerr9.h>
#endif
#include <cassert>
#include <cstdio>

//#define IGNORE_DX9_ERRORS


#if IDV_DIRECTX9 | _XBOX


///////////////////////////////////////////////////////////////////////  
//  IdvWideToAnsi

inline const char* IdvWideToAnsi(const WCHAR* pWideString)
{
    st_assert(pWideString);

    static char szDestination[1024];
    (void) WideCharToMultiByte(CP_ACP, 0, pWideString, -1, szDestination, 1023 * sizeof(char), NULL, NULL);

    return szDestination;
}


///////////////////////////////////////////////////////////////////////  
//  Dx9Verify

inline void Dx9Verify(HRESULT hResult)
{
#ifdef IGNORE_DX9_ERRORS
#else
    if (FAILED(hResult))
    {
        fprintf(stderr, "DirectX 9.0 call failed in file %s, line %d\n", __FILE__, __LINE__);

#ifdef IDV_DIRECTX9
        const char* pError = DXGetErrorStringA(hResult);
#endif
#ifdef _XBOX
        const char* pError = DXGetErrorString9(hResult);
#endif
        fprintf(stderr, "DirectX 9.0 error code: [%s]\n", pError);
    }

#ifdef _DEBUG
    st_assert(!FAILED(hResult));
#endif
#endif
}


///////////////////////////////////////////////////////////////////////  
//  IdvSafeRelease (macro)

#define IdvSafeRelease(p)      { if (p) { (p)->Release(); (p) = NULL; } }


#endif // IDV_DIRECTX9 | _XBOX

