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
#include "stl_vector.h"
#ifdef IDV_OPENGL
#include "IdvOpenGL.h"
#endif 
#if IDV_DIRECTX9 | _XBOX
#include "IdvDirectX9.h"
#endif
#include "SpeedTreeMemory.h"

typedef unsigned int IdvColorType;


///////////////////////////////////////////////////////////////////////  
//  SpeedTree memory management
//
//  Used to capture allocations in STL containers to report back to
//  SpeedTreeRT's allocation mechanism

DefineAllocator(st_allocator_color);
typedef std::vector<IdvColorType, st_allocator_color<IdvColorType> > st_vector_color;


///////////////////////////////////////////////////////////////////////  
//  class CIdvGeometryBuffer

class CIdvGeometryBuffer
{
public:
                                    CIdvGeometryBuffer( );
virtual                             ~CIdvGeometryBuffer( );

        enum EIndexSize
        {
            UBYTE, USHORT, UINT
        };

        struct SStats
        {
        };

        enum EPrimitiveType
        {
            POINTS, 
            LINE_STRIP, LINE_LOOP, LINES, 
            TRIANGLE_STRIP, TRIANGLE_FAN, TRIANGLES, 
            QUAD_STRIP, QUADS
        };

        enum { c_nMaxTextures = 8 };
        
        // index specification
        void                        AddIndex(long lIndex);
        void                        AddIndices(const int* pIndices, int NumIndices);
        void                        AddIndices(const long* pIndices, long lNumIndices);

        // need “all done” flags
virtual void                        FinishVertices(void) = 0;
        void                        FinishIndices(EIndexSize eIndexType = UINT);

        // queries
virtual int                         NumVertices(void) const = 0;
        int                         NumIndices(void) const                      { return m_vIndices.empty( ) ? m_nNumIndices : int(m_vIndices.size( )); }
        bool                        Empty(void) const                           { return !(NumVertices( ) > 0 && NumIndices( ) > 0); }
        int                         VertexSize(void) const                      { return m_nVertexSize; }

        // during run-time
        void                        EnableFormat(void) const;
static  void                        DisableFormat(void);

        bool                        BindVertexBuffer(bool bEnableFormat = false) const;
static  void                        UnBindVertexBuffer(bool bDisableFormat = false);

        bool                        BindIndexBuffer(void) const;
static  void                        UnBindIndexBuffer(void);

        bool                        DrawIndexed(EPrimitiveType ePrimType, int nStart = 0, int nNumElements = -1) const;
        bool                        DrawArrays(EPrimitiveType ePrimType, int nStart = 0, int nNumElements = -1) const;

#if IDV_DIRECTX9 | _XBOX
static  void                        SetDevice(LPDIRECT3DDEVICE9 pDevice)        { m_pDx = pDevice; }
        LPDIRECT3DVERTEXBUFFER9     GetDx9VertexBuffer(void) const              { return m_dxVertexBuffer; }
        LPDIRECT3DINDEXBUFFER9      GetDx9IndexBuffer(void) const               { return m_dxIndexBuffer; }
#endif

protected:
#ifdef IDV_OPENGL
        GLuint                      m_uiVertexBuffer;
        GLuint                      m_uiIndexBuffer;
#endif
#if IDV_DIRECTX9 | _XBOX
        DWORD                       m_dwFvfVector;
        LPDIRECT3DVERTEXBUFFER9     m_dxVertexBuffer;
        LPDIRECT3DINDEXBUFFER9      m_dxIndexBuffer;
static  LPDIRECT3DDEVICE9           m_pDx;
#endif

        int                         m_nVertexSize;

        int                         m_nMaxCoordSize;
        int                         m_anMaxTexCoordSize[c_nMaxTextures];

#ifdef IDV_OPENGL
        // store the vertex offsets and buffer locations
        int                         m_nCoordOffset;
        int                         m_nColorOffset;
        int                         m_nNormalOffset;
        int                         m_anTexCoordOffsets[c_nMaxTextures];
#endif
#ifdef PS3
        GLuint                      m_uiAttribSet;
#endif

        // index buffer
        st_vector_long              m_vIndices;
        EIndexSize                  m_eIndexType;
        int                         m_nNumIndices;
};


///////////////////////////////////////////////////////////////////////  
//  class CIdvImmediateModeBuffer

class CIdvImmediateModeBuffer : public CIdvGeometryBuffer
{
public:
                                    CIdvImmediateModeBuffer(int nNumVertsHint = 200);

        // defining vertex attributes (OpenGL-like interface)
        void                        Vertex3(float x, float y, float z);
        void                        Vertex3v(const float* pCoords);
        void                        Vertex4(float x, float y, float z, float w);
        void                        Vertex4v(const float* pCoords);

        void                        TexCoord1(int nLayer, float s);
        void                        TexCoord2(int nLayer, float s, float t);
        void                        TexCoord2v(int nLayer, const float* pTexCoords);
        void                        TexCoord3(int nLayer, float s, float t, float u);
        void                        TexCoord3v(int nLayer, const float* pTexCoords);
        void                        TexCoord4(int nLayer, float s, float t, float u, float v);
        void                        TexCoord4v(int nLayer, const float* pTexCoords);

        void                        Normal(float x, float y, float z);
        void                        Normalv(const float* pNormal);

        void                        Color3(float r, float g, float b);
        void                        Color3v(const float* pColor);
        void                        Color4(float r, float g, float b, float a); // convert to unsigned int internally
        void                        Color4v(const float* pColor);
        void                        Color4(IdvColorType uiColor);

virtual int                         NumVertices(void) const;
virtual void                        FinishVertices(void);

private:
        void                        SubmitVertex(int nNumCoords);

protected:
        // store the actual vertex attribute data
        st_vector_float             m_vCoords;
        st_vector_color             m_vColors;
        st_vector_float             m_vNormals;
        st_vector_float             m_avTexCoords[c_nMaxTextures];

private:
        // running state variables
        float                       m_afCurrentCoord[4];
        IdvColorType                m_uiCurrentColor;
        float                       m_afCurrentNormal[3];
        float                       m_afCurrentTexCoord[c_nMaxTextures][4];
        bool                        m_bColorSpecified;
        bool                        m_bNormalSpecified;
        bool                        m_abTexCoordSpecified[c_nMaxTextures];

        // misc
mutable int                         m_nNumVertices;
};


///////////////////////////////////////////////////////////////////////  
//  class CIdvTightlyPackedBuffer

class CIdvTightlyPackedBuffer : public CIdvImmediateModeBuffer
{
public:
        // defining vertex attributes (interface for tightly packed data)
        void                        AddVertices(int nNumVertices, int nNumCoords, const float* pVertices);
        void                        AddTexCoords(int nNumVertices, int nLayer, int nNumCoords, const float* pTexCoords);
        void                        AddNormals(int nNumVertices, const float* pNormals);
        void                        AddColors(int nNumVertices, const float* pColors);
        void                        AddColors(int nNumVertices, const IdvColorType* pColors);
};


///////////////////////////////////////////////////////////////////////  
//  class CIdvInterleavedBuffer

class CIdvInterleavedBuffer : public CIdvGeometryBuffer
{
public:
                                    CIdvInterleavedBuffer(bool bDynamic = true);

        void                        SetStride(int nStride);
        void                        SetVertexOffset(int nCount, int nOffset);
        void                        SetTexCoordOffset(int nLayer, int nCount, int nOffset);
        void                        SetNormalOffset(int nOffset);
        void                        SetColorOffset(int nOffset);
        void                        SetMaxNumVertices(int nNumVertices);
        void                        SetBuffer(const void* pBuffer, int nNumVertices, int nOffset = 0); // nOffset is in bytes

virtual void                        FinishVertices(void) { }
virtual int                         NumVertices(void) const;

private:
        void                        CreateBuffer(void);

        bool                        m_bDynamic;
        int                         m_nNumVertices;
};


///////////////////////////////////////////////////////////////////////  
//  ColorFloatsToUInt definition

inline IdvColorType ColorFloatsToUInt(float r, float g, float b, float a)
{
#ifdef BIG_ENDIAN
    return (int(r * 255.0f) << 24) + (int(g * 255.0f) << 16) + (int(b * 255.0f) << 8) + int(a * 255.0f);
#else
    return int(r * 255.0f) + (int(g * 255.0f) << 8) + (int(b * 255.0f) << 16) + (int(a * 255.0f) << 24);
#endif
}


///////////////////////////////////////////////////////////////////////  
//  CIdvGeometryBuffer::AddIndex

inline void CIdvGeometryBuffer::AddIndex(long lIndex)
{
    m_vIndices.push_back(lIndex);
}


///////////////////////////////////////////////////////////////////////  
//  CIdvGeometryBuffer::AddIndex

inline void CIdvGeometryBuffer::AddIndices(const int* pIndices, int nNumIndices)
{
    st_assert(pIndices);

    m_vIndices.resize(m_vIndices.size( ) + nNumIndices);
    copy(pIndices, pIndices + nNumIndices, m_vIndices.begin( ));
}


///////////////////////////////////////////////////////////////////////  
//  CIdvGeometryBuffer::AddIndices

inline void CIdvGeometryBuffer::AddIndices(const long* pIndices, long lNumIndices)
{
    st_assert(pIndices);

    m_vIndices.resize(m_vIndices.size( ) + lNumIndices);
    copy(pIndices, pIndices + lNumIndices, m_vIndices.begin( ));
}


///////////////////////////////////////////////////////////////////////  
//  CIdvImmediateModeBuffer::Vertex3

inline void CIdvImmediateModeBuffer::Vertex3(float x, float y, float z)
{
    m_nMaxCoordSize = st_max(m_nMaxCoordSize, 3);

    m_afCurrentCoord[0] = x;
    m_afCurrentCoord[1] = y;
    m_afCurrentCoord[2] = z;

    SubmitVertex(3);
}


///////////////////////////////////////////////////////////////////////  
//  CIdvImmediateModeBuffer::Vertex3v

inline void CIdvImmediateModeBuffer::Vertex3v(const float* pCoords)
{
    st_assert(pCoords);

    m_nMaxCoordSize = st_max(m_nMaxCoordSize, 3);

    m_afCurrentCoord[0] = pCoords[0];
    m_afCurrentCoord[1] = pCoords[1];
    m_afCurrentCoord[2] = pCoords[2];

    SubmitVertex(3);
}


///////////////////////////////////////////////////////////////////////  
//  CIdvImmediateModeBuffer::Vertex4

inline void CIdvImmediateModeBuffer::Vertex4(float x, float y, float z, float w)
{
    m_nMaxCoordSize = st_max(m_nMaxCoordSize, 4);

    m_afCurrentCoord[0] = x;
    m_afCurrentCoord[1] = y;
    m_afCurrentCoord[2] = z;
    m_afCurrentCoord[3] = w;

    SubmitVertex(4);
}


///////////////////////////////////////////////////////////////////////  
//  CIdvImmediateModeBuffer::Vertex4v

inline void CIdvImmediateModeBuffer::Vertex4v(const float* pCoords)
{
    st_assert(pCoords);

    m_nMaxCoordSize = st_max(m_nMaxCoordSize, 4);

    m_afCurrentCoord[0] = pCoords[0];
    m_afCurrentCoord[1] = pCoords[1];
    m_afCurrentCoord[2] = pCoords[2];
    m_afCurrentCoord[3] = pCoords[3];

    SubmitVertex(4);
}


///////////////////////////////////////////////////////////////////////  
//  CIdvImmediateModeBuffer::TexCoord1

inline void CIdvImmediateModeBuffer::TexCoord1(int nLayer, float s)
{
    st_assert(nLayer > -1 && nLayer < c_nMaxTextures);

    m_anMaxTexCoordSize[nLayer] = st_max(m_anMaxTexCoordSize[nLayer], 1);

    m_avTexCoords[nLayer].push_back(s);
}


///////////////////////////////////////////////////////////////////////  
//  CIdvImmediateModeBuffer::TexCoord2

inline void CIdvImmediateModeBuffer::TexCoord2(int nLayer, float s, float t)
{
    st_assert(nLayer > -1 && nLayer < c_nMaxTextures);

    m_anMaxTexCoordSize[nLayer] = st_max(m_anMaxTexCoordSize[nLayer], 2);

    m_avTexCoords[nLayer].push_back(s);
    m_avTexCoords[nLayer].push_back(t);
}


///////////////////////////////////////////////////////////////////////  
//  CIdvImmediateModeBuffer::TexCoord2v

inline void CIdvImmediateModeBuffer::TexCoord2v(int nLayer, const float* pTexCoords)
{
    st_assert(nLayer > -1 && nLayer < c_nMaxTextures);
    st_assert(pTexCoords);

    m_anMaxTexCoordSize[nLayer] = st_max(m_anMaxTexCoordSize[nLayer], 2);

    m_avTexCoords[nLayer].push_back(pTexCoords[0]);
    m_avTexCoords[nLayer].push_back(pTexCoords[1]);
}


///////////////////////////////////////////////////////////////////////  
//  CIdvImmediateModeBuffer::TexCoord3

inline void CIdvImmediateModeBuffer::TexCoord3(int nLayer, float s, float t, float u)
{
    st_assert(nLayer > -1 && nLayer < c_nMaxTextures);

    m_anMaxTexCoordSize[nLayer] = st_max(m_anMaxTexCoordSize[nLayer], 3);

    m_avTexCoords[nLayer].push_back(s);
    m_avTexCoords[nLayer].push_back(t);
    m_avTexCoords[nLayer].push_back(u);
}


///////////////////////////////////////////////////////////////////////  
//  CIdvImmediateModeBuffer::TexCoord3v

inline void CIdvImmediateModeBuffer::TexCoord3v(int nLayer, const float* pTexCoords)
{
    st_assert(nLayer > -1 && nLayer < c_nMaxTextures);
    st_assert(pTexCoords);

    m_anMaxTexCoordSize[nLayer] = st_max(m_anMaxTexCoordSize[nLayer], 3);

    m_avTexCoords[nLayer].push_back(pTexCoords[0]);
    m_avTexCoords[nLayer].push_back(pTexCoords[1]);
    m_avTexCoords[nLayer].push_back(pTexCoords[2]);
}


///////////////////////////////////////////////////////////////////////  
//  CIdvImmediateModeBuffer::TexCoord4

inline void CIdvImmediateModeBuffer::TexCoord4(int nLayer, float s, float t, float u, float v)
{
    st_assert(nLayer > -1 && nLayer < c_nMaxTextures);

    m_anMaxTexCoordSize[nLayer] = st_max(m_anMaxTexCoordSize[nLayer], 4);

    m_avTexCoords[nLayer].push_back(s);
    m_avTexCoords[nLayer].push_back(t);
    m_avTexCoords[nLayer].push_back(u);
    m_avTexCoords[nLayer].push_back(v);
}


///////////////////////////////////////////////////////////////////////  
//  CIdvImmediateModeBuffer::TexCoord4v

inline void CIdvImmediateModeBuffer::TexCoord4v(int nLayer, const float* pTexCoords)
{
    st_assert(nLayer > -1 && nLayer < c_nMaxTextures);
    st_assert(pTexCoords);

    m_anMaxTexCoordSize[nLayer] = st_max(m_anMaxTexCoordSize[nLayer], 4);

    m_avTexCoords[nLayer].push_back(pTexCoords[0]);
    m_avTexCoords[nLayer].push_back(pTexCoords[1]);
    m_avTexCoords[nLayer].push_back(pTexCoords[2]);
    m_avTexCoords[nLayer].push_back(pTexCoords[3]);
}


///////////////////////////////////////////////////////////////////////  
//  CIdvImmediateModeBuffer::Normal

inline void CIdvImmediateModeBuffer::Normal(float x, float y, float z)
{
    m_afCurrentNormal[0] = x;
    m_afCurrentNormal[1] = y;
    m_afCurrentNormal[2] = z;

    m_bNormalSpecified = true;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvImmediateModeBuffer::Normalv

inline void CIdvImmediateModeBuffer::Normalv(const float* pNormal)
{
    st_assert(pNormal);

    m_afCurrentNormal[0] = pNormal[0];
    m_afCurrentNormal[1] = pNormal[1];
    m_afCurrentNormal[2] = pNormal[2];

    m_bNormalSpecified = true;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvImmediateModeBuffer::Color3

inline void CIdvImmediateModeBuffer::Color3(float r, float g, float b)
{
    m_uiCurrentColor = ColorFloatsToUInt(r, g, b, 1.0f);

    m_bColorSpecified = true;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvImmediateModeBuffer::Color3v

inline void CIdvImmediateModeBuffer::Color3v(const float* pColor)
{
    st_assert(pColor);

    m_uiCurrentColor = ColorFloatsToUInt(pColor[0], pColor[1], pColor[2], 1.0f);

    m_bColorSpecified = true;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvImmediateModeBuffer::Color4

inline void CIdvImmediateModeBuffer::Color4(float r, float g, float b, float a)
{
    m_uiCurrentColor = ColorFloatsToUInt(r, g, b, a);

    m_bColorSpecified = true;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvImmediateModeBuffer::Color4

inline void CIdvImmediateModeBuffer::Color4(IdvColorType uiColor)
{
    m_uiCurrentColor = uiColor;

    m_bColorSpecified = true;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvImmediateModeBuffer::Color4v

inline void CIdvImmediateModeBuffer::Color4v(const float* pColor)
{
    st_assert(pColor);

    m_uiCurrentColor = ColorFloatsToUInt(pColor[0], pColor[1], pColor[2], pColor[3]);

    m_bColorSpecified = true;
}

