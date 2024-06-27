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
#if IDV_DIRECTX9 | _XBOX
#include "IdvDirectX9.h"
#endif
#ifdef IDV_OPENGL
#include "IdvDDS.h"
#endif
#include "stl_vector.h"
#include "SpeedTreeMemory.h"


///////////////////////////////////////////////////////////////////////  
//  class CIdvTexture declaration

class CIdvTexture
{
public:
                            CIdvTexture( );
                            ~CIdvTexture( );

        enum EFilter
        {
            FILTER_NONE, FILTER_LINEAR_MIPMAP_LINEAR, FILTER_LINEAR
        };

        const st_string&    Filename(void) const                            { return m_strFilename; }
        bool                Load(const st_string& strFilename, EFilter eMin = FILTER_LINEAR_MIPMAP_LINEAR, EFilter eMag = FILTER_LINEAR);
        bool                LoadWhite(void)                                 { return LoadSolidRGBATexture("white", 0xffffffff); }
        bool                LoadBlack(void)                                 { return LoadSolidRGBATexture("black", 0xff000000); }
        bool                LoadSolidRGBATexture(const st_string& strName, unsigned int uiColor);
        void                Bind(int nLayer = 0) const;
        bool                IsValid(void) const;
static  void                Disable(int nLayer = 0);

#if IDV_DIRECTX9 | _XBOX
static  void                SetDevice(LPDIRECT3DDEVICE9 pDx)                { m_pDx = pDx; }
        LPDIRECT3DTEXTURE9  GetDx9Texture(void) const                       { return m_pTexture; }
#endif
#ifdef IDV_OPENGL
        GLuint              GetOpenGLTexture(void) const                    { return m_pTexture; }
#endif

private:
        st_string           m_strFilename;
#ifdef IDV_OPENGL
        GLuint              m_pTexture;
#endif

#if IDV_DIRECTX9 | _XBOX
static  LPDIRECT3DDEVICE9   m_pDx;
        LPDIRECT3DTEXTURE9  m_pTexture;
        _D3DTEXTUREFILTERTYPE m_eMinFilter;
        _D3DTEXTUREFILTERTYPE m_eMagFilter;
#endif

        ///////////////////////////////////////////////////////////////////////  
        //  Texture caching mechanism

static  int                 SearchCache(const st_string& strFilename);

        struct SCacheEntry
        {
            SCacheEntry( ) :
                m_nRefCount(0),
                m_pTexture(0)
            {
            }

            st_string           m_strFilename;
            int                 m_nRefCount;
#if IDV_DIRECTX9 | _XBOX
            LPDIRECT3DTEXTURE9  m_pTexture;
#endif
#ifdef IDV_OPENGL
            GLuint              m_pTexture;
#endif
        };


        ///////////////////////////////////////////////////////////////////////  
        //  SpeedTree memory management
        //
        //  Used to capture allocations in STL containers to report back to
        //  SpeedTreeRT's allocation mechanism

        DefineAllocator(st_allocator_texcache);
        typedef std::vector<SCacheEntry, st_allocator_texcache<SCacheEntry> > st_vector_texcache;

        // should use a std::map here, but it's awfully messy specifying a custom allocator
static  st_vector_texcache* m_pCache;
};
