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
#include "IdvGeometryBuffer.h"
#include "SpeedTreeMemory.h"
#include <cassert>
using namespace std;


// static variables
#if IDV_DIRECTX9 | _XBOX
LPDIRECT3DDEVICE9 CIdvGeometryBuffer::m_pDx = NULL;
#endif


///////////////////////////////////////////////////////////////////////  
//  CIdvGeometryBuffer::CIdvGeometryBuffer

CIdvGeometryBuffer::CIdvGeometryBuffer( ) :
#ifdef IDV_OPENGL
    m_uiVertexBuffer(0),
    m_uiIndexBuffer(0),
#endif
#if IDV_DIRECTX9 | _XBOX
    m_dwFvfVector(0),
    m_dxVertexBuffer(NULL),
    m_dxIndexBuffer(NULL),
#endif
    m_nVertexSize(-1),
    m_nMaxCoordSize(0),
#ifdef IDV_OPENGL
    m_nCoordOffset(-1),
    m_nColorOffset(-1),
    m_nNormalOffset(-1),
#endif
#ifdef PS3
    m_uiAttribSet(0),
#endif
    m_eIndexType(CIdvGeometryBuffer::UINT),
    m_nNumIndices(0)
{
    // setup the attribute max counts
    int i =0;
    for (i = 0; i < c_nMaxTextures; ++i)
        m_anMaxTexCoordSize[i] = 0;

#ifdef IDV_OPENGL
    // set the the offsets and buffers positions to -1 since 0 is a valid value
    for (i = 0; i < c_nMaxTextures; ++i)
        m_anTexCoordOffsets[i] = -1;
#endif
}


///////////////////////////////////////////////////////////////////////  
//  CIdvGeometryBuffer::~CIdvGeometryBuffer

CIdvGeometryBuffer::~CIdvGeometryBuffer( )
{
#if IDV_DIRECTX9 | _XBOX
    IdvSafeRelease(m_dxVertexBuffer);
    IdvSafeRelease(m_dxIndexBuffer);
#endif
}


///////////////////////////////////////////////////////////////////////  
//  CIdvGeometryBuffer::FinishIndices

void CIdvGeometryBuffer::FinishIndices(EIndexSize eIndexType)
{
    st_assert(!m_vIndices.empty( ));
    m_eIndexType = eIndexType;

    // convert the buffer down to the specified size
    m_nNumIndices = int(m_vIndices.size( ));
    st_vector_byte vElementsUChar;
    st_vector_ushort vElementUShort;
    st_vector_uint vElementsUInt;
    void* pRawBufferPtr = NULL;

    int i = 0;
    int nElementSize = 0;
    switch (eIndexType)
    {
    case UBYTE:
        nElementSize = sizeof(unsigned char);
        vElementsUChar.resize(m_nNumIndices);
        for (i = 0; i < m_nNumIndices; ++i)
            vElementsUChar[i] = static_cast<unsigned char>(m_vIndices[i]);
        pRawBufferPtr = &(vElementsUChar[0]);
        break;
    case USHORT:
        nElementSize = sizeof(unsigned short);
        vElementUShort.resize(m_nNumIndices);
        for (i = 0; i < m_nNumIndices; ++i)
            vElementUShort[i] = static_cast<unsigned short>(m_vIndices[i]);
        pRawBufferPtr = &(vElementUShort[0]);
        break;
    case UINT:
        nElementSize = sizeof(unsigned int);
        vElementsUInt.resize(m_nNumIndices);
        for (i = 0; i < m_nNumIndices; ++i)
            vElementsUInt[i] = static_cast<unsigned int>(m_vIndices[i]);
        pRawBufferPtr = &(vElementsUInt[0]);
        break;
    default:
        IdvInternal("Invalid switch/case in CIdvGeometryBuffer::FinishIndices()");
    }

#ifdef IDV_OPENGL
    glGenBuffersARB(1, &m_uiIndexBuffer);
    glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_uiIndexBuffer);
    glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, GLsizeiptrARB(m_nNumIndices* nElementSize), &m_vIndices[0], GL_STATIC_DRAW_ARB);

    // unbind this buffer
    glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
#endif

#if IDV_DIRECTX9 | _XBOX
    // create the DX9 index buffer
    st_assert(m_pDx);
    Dx9Verify(m_pDx->CreateIndexBuffer(m_nNumIndices * nElementSize, D3DUSAGE_WRITEONLY, D3DFMT_INDEX32, D3DPOOL_DEFAULT, &m_dxIndexBuffer, NULL));

    // make sure the buffer's not bound when updating
    Dx9Verify(m_pDx->SetIndices(NULL));

    // fill the buffer
    void* pIndexBuffer = NULL;
    Dx9Verify(m_dxIndexBuffer->Lock(0, 0, reinterpret_cast<void**>(&pIndexBuffer), 0));
    st_assert(int(m_vIndices.size( )) == m_nNumIndices);
    memcpy(pIndexBuffer, &m_vIndices[0], m_nNumIndices * nElementSize);
    Dx9Verify(m_dxIndexBuffer->Unlock( ));
#endif

    // empty the client-side copy now that a copy has been sent to the driver
    m_vIndices.clear( );
}


///////////////////////////////////////////////////////////////////////  
//  CIdvGeometryBuffer::EnableFormat

void CIdvGeometryBuffer::EnableFormat(void) const
{
#if IDV_OPENGL

#ifndef PS3
    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
#endif

    // activate the attributes as necessary
    if (m_nCoordOffset > -1)
        glEnableClientState(GL_VERTEX_ARRAY);
    if (m_nColorOffset > -1)
        glEnableClientState(GL_COLOR_ARRAY);
    if (m_nNormalOffset > -1)
        glEnableClientState(GL_NORMAL_ARRAY);
    for (int i = 0; i < c_nMaxTextures; ++i)
    {
        if (m_anTexCoordOffsets[i] > -1)
        {
            glClientActiveTextureARB(GL_TEXTURE0_ARB + i);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        }
    }

    // make sure we leave the first layer as active
    glClientActiveTextureARB(GL_TEXTURE0_ARB);
#endif

#if IDV_DIRECTX9 | _XBOX
    Dx9Verify(m_pDx->SetFVF(m_dwFvfVector));
#endif
}


///////////////////////////////////////////////////////////////////////  
//  CIdvGeometryBuffer::DisableFormat

void CIdvGeometryBuffer::DisableFormat(void)
{
#if IDV_OPENGL
#ifdef PS3
    // activate the attributes as necessary
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    for (int i = c_nMaxTextures - 1; i > -1; --i)
    {
        glClientActiveTextureARB(GL_TEXTURE0_ARB + i);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
#else
    glPopClientAttrib( );
#endif
#endif

#if IDV_DIRECTX9 | _XBOX
    //Dx9Verify(m_pDx->SetStreamSource(0, NULL, 0, 0));
#endif
}


///////////////////////////////////////////////////////////////////////  
//  CIdvGeometryBuffer::BindVertexBuffer

bool CIdvGeometryBuffer::BindVertexBuffer(bool bEnableFormat) const
{
    bool bSuccess = false;

#ifdef IDV_OPENGL
    if (bEnableFormat)
        EnableFormat( );

    if (m_uiVertexBuffer && m_nVertexSize > 0)
    {
        // bind the ARB object
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_uiVertexBuffer);

        const unsigned char* pBase = NULL;

        // activate the attributes as necessary
        if (m_nCoordOffset > -1)
            glVertexPointer(m_nMaxCoordSize, GL_FLOAT, m_nVertexSize, pBase + m_nCoordOffset);
        if (m_nColorOffset > -1)
            glColorPointer(4, GL_UNSIGNED_BYTE, m_nVertexSize, pBase + m_nColorOffset);
        if (m_nNormalOffset > -1)
            glNormalPointer(GL_FLOAT, m_nVertexSize, pBase + m_nNormalOffset);
        for (int i = 0; i < c_nMaxTextures; ++i)
        {
            if (m_anTexCoordOffsets[i] > -1)
            {
                glClientActiveTextureARB(GL_TEXTURE0_ARB + i);
                glTexCoordPointer(m_anMaxTexCoordSize[i], GL_FLOAT, m_nVertexSize, pBase + m_anTexCoordOffsets[i]);
                glClientActiveTextureARB(GL_TEXTURE0_ARB);
            }
        }

        bSuccess = true;
    }
#endif // IDV_OPENGL

#if IDV_DIRECTX9 | _XBOX
    if (bEnableFormat)
        EnableFormat( );

    st_assert(m_pDx);
    st_assert(m_dwFvfVector);
    st_assert(m_dxVertexBuffer);

    if (m_dxVertexBuffer && m_nVertexSize > 0)
    {
        if (bEnableFormat)
            Dx9Verify(m_pDx->SetFVF(m_dwFvfVector));

        Dx9Verify(m_pDx->SetStreamSource(0, m_dxVertexBuffer, 0, m_nVertexSize));

        bSuccess = true;
    }
#endif

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvGeometryBuffer::DrawIndexed

bool CIdvGeometryBuffer::DrawIndexed(EPrimitiveType ePrimType, int nStart, int nNumElements) const
{
    bool bSuccess = false;

    if (nNumElements > 0)
    {
        if (nNumElements == -1)
            nNumElements = m_nNumIndices;

#ifdef IDV_OPENGL
        // primitive type
        GLenum eOpenGLPrimitive = GL_TRIANGLE_STRIP;
        switch (ePrimType)
        {
        case TRIANGLE_STRIP:
            eOpenGLPrimitive = GL_TRIANGLE_STRIP;
            break;
        case POINTS:
            eOpenGLPrimitive = GL_POINTS;
            break;
        case LINE_STRIP:
            eOpenGLPrimitive = GL_LINE_STRIP;
            break;
        case LINE_LOOP:
            eOpenGLPrimitive = GL_LINE_LOOP;
            break;
        case LINES:
            eOpenGLPrimitive = GL_LINES;
            break;
        case TRIANGLE_FAN:
            eOpenGLPrimitive = GL_TRIANGLE_FAN;
            break;
        case TRIANGLES:
            eOpenGLPrimitive = GL_TRIANGLES;
            break;
        case QUAD_STRIP:
            eOpenGLPrimitive = GL_QUAD_STRIP;
            break;
        case QUADS:
            eOpenGLPrimitive = GL_QUADS;
            break;
        default:
            IdvInternal("Invalid primitive switch/case in CIdvGeometryBuffer::DrawIndexed()");
        }

        // index data type
        GLenum eIndexType = GL_UNSIGNED_INT;
        int nElementSize = sizeof(unsigned int);
        switch (m_eIndexType)
        {
        case UBYTE:
            eIndexType = GL_UNSIGNED_BYTE;
            nElementSize = sizeof(unsigned char);
            break;
        case USHORT:
            eIndexType = GL_UNSIGNED_SHORT;
            nElementSize = sizeof(unsigned short);
            break;
        case UINT:
            eIndexType = GL_UNSIGNED_INT;
            nElementSize = sizeof(unsigned int);
            break;
        default:
            IdvInternal("Invalid index type switch/case in CIdvGeometryBuffer::DrawIndexed()");
        }

        if (m_uiIndexBuffer && m_uiVertexBuffer)
        {
            const char* pBase = NULL;
            glDrawElements(eOpenGLPrimitive, nNumElements, eIndexType, pBase + nStart * nElementSize);

            bSuccess = true;
        }
#endif

#if IDV_DIRECTX9 | _XBOX
        st_assert(m_pDx);
        st_assert(NumVertices( ) > 0);

        if (m_dxIndexBuffer && nNumElements > 2)
        {
            // primitive type
            switch (ePrimType)
            {
            case TRIANGLE_STRIP:
                Dx9Verify(m_pDx->DrawIndexedPrimitive(D3DPT_TRIANGLESTRIP, 0, 0, NumVertices( ), nStart, nNumElements - 2));
                break;
            case TRIANGLES:
                Dx9Verify(m_pDx->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, NumVertices( ), nStart, nNumElements / 3));
                break;
            default:
                IdvInternal("Invalid switch/case in CIdvGeometryBuffer::DrawIndexed()");
            }

            bSuccess = true;
        }
#endif
    }
    else
        bSuccess = true; // rendering 0 vertices is not a failure

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvGeometryBuffer::DrawArrays

bool CIdvGeometryBuffer::DrawArrays(EPrimitiveType ePrimType, int nStart, int nNumElements) const
{
    bool bSuccess = false;

    if (nNumElements > 0)
    {
#ifdef IDV_OPENGL
        // primitive type
        GLenum eOpenGLPrimitive = GL_TRIANGLE_STRIP;
        switch (ePrimType)
        {
        case QUADS:
            eOpenGLPrimitive = GL_QUADS;
            break;
        case TRIANGLES:
            eOpenGLPrimitive = GL_TRIANGLES;
            break;
        case POINTS:
            eOpenGLPrimitive = GL_POINTS;
            break;
        case LINE_STRIP:
            eOpenGLPrimitive = GL_LINE_STRIP;
            break;
        case LINE_LOOP:
            eOpenGLPrimitive = GL_LINE_LOOP;
            break;
        case LINES:
            eOpenGLPrimitive = GL_LINES;
            break;
        case TRIANGLE_STRIP:
            eOpenGLPrimitive = GL_TRIANGLE_STRIP;
            break;
        case TRIANGLE_FAN:
            eOpenGLPrimitive = GL_TRIANGLE_FAN;
            break;
        case QUAD_STRIP:
            eOpenGLPrimitive = GL_QUAD_STRIP;
            break;
        default:
            IdvInternal("Invalid switch/case in CIdvGeometryBuffer::DrawArrays()");
        }

        glDrawArrays(eOpenGLPrimitive, nStart, nNumElements);
        
        bSuccess = true;
#endif

#if IDV_DIRECTX9 | _XBOX
        // primitive type
        D3DPRIMITIVETYPE eDx9Primitive = D3DPT_TRIANGLELIST;
        switch (ePrimType)
        {
#if _XBOX
        case QUADS:
            eDx9Primitive = D3DPT_QUADLIST;
            nNumElements /= 4;
            break;
#endif
        case TRIANGLES:
            eDx9Primitive = D3DPT_TRIANGLELIST;
            nNumElements /= 3;
            break;
        case LINES:
            eDx9Primitive = D3DPT_LINELIST;
            nNumElements /= 2;
            break;
        case LINE_STRIP:
            eDx9Primitive = D3DPT_LINESTRIP;
            nNumElements -= 1;
            break;
        case TRIANGLE_FAN:
            eDx9Primitive = D3DPT_TRIANGLEFAN;
            nNumElements -= 2;
            break;
        case POINTS:
            eDx9Primitive = D3DPT_POINTLIST;
            break;
        default:
            IdvInternal("Invalid switch/case in CIdvGeometryBuffer::DrawArrays()");
        }

        Dx9Verify(m_pDx->DrawPrimitive(eDx9Primitive, nStart, nNumElements));

        bSuccess = true;
#endif
    }
    else
        bSuccess = true; // rendering 0 vertices is not a failure

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvGeometryBuffer::UnBindVertexBuffer

void CIdvGeometryBuffer::UnBindVertexBuffer(bool bDisableFormat)
{
#ifdef IDV_OPENGL
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

    if (bDisableFormat)
        DisableFormat( );
#endif

#if IDV_DIRECTX9 | _XBOX
    UNREFERENCED_PARAMETER(bDisableFormat);
    Dx9Verify(m_pDx->SetStreamSource(0, NULL, 0, 0));
#endif
}


///////////////////////////////////////////////////////////////////////  
//  CIdvGeometryBuffer::BindIndexBuffer

bool CIdvGeometryBuffer::BindIndexBuffer(void) const
{
    bool bSuccess = false;

#ifdef IDV_OPENGL
    if (m_uiIndexBuffer)
    {
        glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_uiIndexBuffer);
        bSuccess = true;
    }
#endif
#if IDV_DIRECTX9 | _XBOX
    if (m_dxIndexBuffer)
    {
        Dx9Verify(m_pDx->SetIndices(m_dxIndexBuffer));
        bSuccess = true;
    }

#endif

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvGeometryBuffer::UnBindIndexBuffer

void CIdvGeometryBuffer::UnBindIndexBuffer(void)
{
#ifdef IDV_OPENGL
    glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
#endif

#if IDV_DIRECTX9 | _XBOX
    Dx9Verify(m_pDx->SetIndices(NULL));
#endif
}


///////////////////////////////////////////////////////////////////////  
//  CIdvImmediateModeBuffer::CIdvImmediateModeBuffer

CIdvImmediateModeBuffer::CIdvImmediateModeBuffer(int nNumVertsHint) :
    m_nNumVertices(0)
{
    // to reduce memory fragmentation that can occur during excessive use of 
    // vector::push_back(), we're using reserve with a hint
    if (nNumVertsHint > 0)
    {
        m_vCoords.reserve(nNumVertsHint * 4);
        m_vColors.reserve(nNumVertsHint);
        m_vNormals.reserve(nNumVertsHint * 3);

        for (int i = 0; i < c_nMaxTextures; ++i)
            m_avTexCoords[i].reserve(nNumVertsHint * 4);
    }

    // setup the initial states
    m_afCurrentCoord[0] = m_afCurrentCoord[1] = m_afCurrentCoord[2] = 0.0f;
    m_afCurrentCoord[3] = 1.0f;
    m_uiCurrentColor = ColorFloatsToUInt(0.0f, 0.0f, 0.0f, 1.0f);
    m_afCurrentNormal[0] = m_afCurrentNormal[1] = 0.0f;
    m_afCurrentNormal[2] = 1.0f;
    int i = 0;
    for (i = 0; i < c_nMaxTextures; ++i)
        for (int j = 0; j < 4; ++j)
            m_afCurrentTexCoord[i][j] = 0.0f;

    m_bColorSpecified = false;
    m_bNormalSpecified = false;
    for (i = 0; i < c_nMaxTextures; ++i)
        m_abTexCoordSpecified[i] = false;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvImmediateModeBuffer::FinishVertices

void CIdvImmediateModeBuffer::FinishVertices(void)
{
    // how many vertices are there? (assumes that coords are always present)
    m_nNumVertices = NumVertices( );
    st_assert(m_nNumVertices > -1);

    // sanity test: all buffers should either have the same number of vertices or they should be empty
    if (m_nMaxCoordSize > 0)
    {
        st_assert(m_vColors.empty( ) || int(m_vColors.size( )) == m_nNumVertices);
        st_assert(m_vNormals.empty( ) || int(m_vNormals.size( )) / 3 == m_nNumVertices);
        for (int i = 0; i < c_nMaxTextures; ++i)
        {
            st_assert(m_avTexCoords[i].empty( ) || int(m_avTexCoords[i].size( )) / m_anMaxTexCoordSize[i] == m_nNumVertices);
        }
    }
    else
        IdvWarning("CIdvImmediateModeBuffer::FinishVertices, m_nMaxCoordSize < 1, cannot run sanity test\n");

    // which vertex attributes are active?
    m_nVertexSize = 0;
#if IDV_DIRECTX9 | _XBOX
    m_dwFvfVector = 0;
    int nNumActiveTextureLayers = 0;
#endif
    if (m_nMaxCoordSize > 0)
    {
        st_assert(m_nMaxCoordSize == 3 || m_nMaxCoordSize == 4);
        m_nVertexSize += m_nMaxCoordSize * sizeof(float);
#if IDV_DIRECTX9 | _XBOX
        m_dwFvfVector |= (m_nMaxCoordSize == 3) ? D3DFVF_XYZ : D3DFVF_XYZW;
#endif
    }
    if (!m_vColors.empty( ))
    {
        m_nVertexSize += sizeof(IdvColorType);
#if IDV_DIRECTX9 | _XBOX
        m_dwFvfVector |= D3DFVF_DIFFUSE;
#endif
    }
    if (!m_vNormals.empty( ))
    {
        m_nVertexSize += 3 * sizeof(float);
#if IDV_DIRECTX9 | _XBOX
        m_dwFvfVector |= D3DFVF_NORMAL;
#endif
    }
    int i = 0;
    for (i = 0; i < c_nMaxTextures; ++i)
    {
        if (m_anMaxTexCoordSize[i] > 0)
        {
            m_nVertexSize += m_anMaxTexCoordSize[i] * sizeof(float);
#if IDV_DIRECTX9 | _XBOX
            if (m_anMaxTexCoordSize[i] == 1)
                m_dwFvfVector |= D3DFVF_TEXCOORDSIZE1(i);
            else if (m_anMaxTexCoordSize[i] == 2)
                m_dwFvfVector |= D3DFVF_TEXCOORDSIZE2(i);
            else if (m_anMaxTexCoordSize[i] == 3)
                m_dwFvfVector |= D3DFVF_TEXCOORDSIZE3(i);
            else if (m_anMaxTexCoordSize[i] == 4)
                m_dwFvfVector |= D3DFVF_TEXCOORDSIZE4(i);
            else
                IdvInternal("Invalid switch/case in CIdvGeometryBuffer::FinishVertices()");
            ++nNumActiveTextureLayers;
#endif
        }
    }

#if IDV_DIRECTX9 | _XBOX
    if (nNumActiveTextureLayers > 0)
        m_dwFvfVector |= DWORD(nNumActiveTextureLayers) << D3DFVF_TEXCOUNT_SHIFT;
#endif
    st_assert(m_nVertexSize > 0);

    // build a continuous buffer, interleaved or tightly packed
    const int c_nBufferSize = m_nVertexSize * m_nNumVertices;
    unsigned char* pWholeBuffer = st_new_array<unsigned char>(c_nBufferSize, "CIdvImmediateModeBuffer::FinishVertices, pWholeBuffer");
    unsigned char* pBufferPtr = pWholeBuffer;

    for (int nVertex = 0; nVertex < m_nNumVertices; ++nVertex)
    {
        // copy coords
#ifdef IDV_OPENGL
        m_nCoordOffset = (nVertex == 0) ? int(pBufferPtr - pWholeBuffer) : m_nCoordOffset;
#endif
        int nSize = m_nMaxCoordSize * sizeof(float);
        memcpy(pBufferPtr, &(m_vCoords[nVertex * m_nMaxCoordSize]), nSize);
        pBufferPtr += nSize;

        // copy colors
        if (!m_vColors.empty( ))
        {
#ifdef IDV_OPENGL
            m_nColorOffset = (nVertex == 0) ? int(pBufferPtr - pWholeBuffer) : m_nColorOffset;
#endif
            nSize = sizeof(IdvColorType);
            memcpy(pBufferPtr, &(m_vColors[nVertex]), nSize);
            pBufferPtr += nSize;
        }

        // copy normals
        if (!m_vNormals.empty( ))
        {
#ifdef IDV_OPENGL
            m_nNormalOffset = (nVertex == 0) ? int(pBufferPtr - pWholeBuffer) : m_nNormalOffset;
#endif
            nSize = 3 * sizeof(float);
            memcpy(pBufferPtr, &(m_vNormals[nVertex * 3]), nSize);
            pBufferPtr += nSize;
        }

        // copy texcoords
        for (i = 0; i < c_nMaxTextures; ++i)
        {
            if (!m_avTexCoords[i].empty( ) && m_anMaxTexCoordSize[i] > 0)
            {
#ifdef IDV_OPENGL
                m_anTexCoordOffsets[i] = (nVertex == 0) ? int(pBufferPtr - pWholeBuffer) : m_anTexCoordOffsets[i];
#endif
                nSize = m_anMaxTexCoordSize[i] * sizeof(float);
                memcpy(pBufferPtr, &(m_avTexCoords[i][nVertex * m_anMaxTexCoordSize[i]]), nSize);
                pBufferPtr += nSize;
            }
        }
    }

    // make sure we didn't overflow 
    st_assert(int(pBufferPtr - pWholeBuffer) == c_nBufferSize);

#ifdef IDV_OPENGL
    // generate the ARB vertex buffer
    glGenBuffersARB(1, &m_uiVertexBuffer);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_uiVertexBuffer);
    glBufferDataARB(GL_ARRAY_BUFFER_ARB, GLsizeiptrARB(c_nBufferSize), pWholeBuffer, GL_STATIC_DRAW_ARB);

    // unbind this buffer
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
#endif

#if IDV_DIRECTX9 | _XBOX
    // generate the DX9 vertex buffer
    st_assert(m_pDx);
    Dx9Verify(m_pDx->CreateVertexBuffer(c_nBufferSize, D3DUSAGE_WRITEONLY, m_dwFvfVector, D3DPOOL_DEFAULT, &m_dxVertexBuffer, NULL));

    // make sure the buffer's not bound when updating
    Dx9Verify(m_pDx->SetStreamSource(0, NULL, 0, 0));

    void* pVertexBufferContents = NULL;
    Dx9Verify(m_dxVertexBuffer->Lock(0, 0, reinterpret_cast<void**>(&pVertexBufferContents), 0));
    memcpy(pVertexBufferContents, pWholeBuffer, c_nBufferSize);
    Dx9Verify(m_dxVertexBuffer->Unlock( ));
#endif

    // discard all client-side copies since we've sent a copy to the graphics driver
    m_vCoords.reserve(0);
    m_vCoords.clear( );
    m_vColors.reserve(0);
    m_vColors.clear( );
    m_vNormals.reserve(0);
    m_vNormals.clear( );
    for (i = 0; i < c_nMaxTextures; ++i)
    {
        m_avTexCoords[i].clear( );
        m_avTexCoords[i].reserve(0);
    }

    st_delete_array<unsigned char>(pWholeBuffer, "CIdvImmediateModeBuffer::FinishVertices, pWholeBuffer");
}


///////////////////////////////////////////////////////////////////////  
//  CIdvImmediateModeBuffer::SubmitVertex

void CIdvImmediateModeBuffer::SubmitVertex(int nNumCoords)
{
    m_vCoords.push_back(m_afCurrentCoord[0]);
    m_vCoords.push_back(m_afCurrentCoord[1]);
    m_vCoords.push_back(m_afCurrentCoord[2]);
    if (nNumCoords == 4)
        m_vCoords.push_back(m_afCurrentCoord[3]);

    // gotta do a lot more
    if (m_bColorSpecified)
    {
        m_vColors.push_back(m_uiCurrentColor);
    }

    if (m_bNormalSpecified)
    {
        m_vNormals.push_back(m_afCurrentNormal[0]);
        m_vNormals.push_back(m_afCurrentNormal[1]);
        m_vNormals.push_back(m_afCurrentNormal[2]);
    }

    for (int i = 0; i < c_nMaxTextures; ++i)
    {
        if (m_abTexCoordSpecified[i])
        {
            for (int j = 0; j < m_anMaxTexCoordSize[i]; ++j)
                m_avTexCoords[i].push_back(m_afCurrentTexCoord[i][j]);
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  CIdvImmediateModeBuffer::NumVertices

int CIdvImmediateModeBuffer::NumVertices(void) const
{
    // how many vertices are there? (assumes that coords are always present)
    if (m_nMaxCoordSize > 0 && m_vCoords.size( ) > 0)
        m_nNumVertices = int(m_vCoords.size( )) / m_nMaxCoordSize;

    return m_nNumVertices;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvInterleavedBuffer::CIdvInterleavedBuffer

CIdvInterleavedBuffer::CIdvInterleavedBuffer(bool bDynamic) :
    m_bDynamic(bDynamic),
    m_nNumVertices(0)
{
}


///////////////////////////////////////////////////////////////////////  
//  CIdvInterleavedBuffer::SetStride

void CIdvInterleavedBuffer::SetStride(int nStride)
{
    st_assert(nStride > 0);

    m_nVertexSize = nStride;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvInterleavedBuffer::SetVertexOffset

void CIdvInterleavedBuffer::SetVertexOffset(int nCount, int nOffset)
{
    st_assert(nCount == 3 || nCount == 4);
    st_assert(nOffset > -1);

    m_nMaxCoordSize = nCount;
#ifdef IDV_OPENGL
    m_nCoordOffset = nOffset;
#endif
#if IDV_DIRECTX9 | _XBOX
    UNREFERENCED_PARAMETER(nOffset);
    m_dwFvfVector |= (nCount == 3) ? D3DFVF_XYZ : D3DFVF_XYZW;
#endif
}


///////////////////////////////////////////////////////////////////////  
//  CIdvInterleavedBuffer::SetTexCoordOffset

void CIdvInterleavedBuffer::SetTexCoordOffset(int nLayer, int nCount, int nOffset)
{
    st_assert(nLayer > -1 && nLayer < c_nMaxTextures);
    st_assert(nCount > 0 && nCount < 5);
    st_assert(nOffset > -1);

    m_anMaxTexCoordSize[nLayer] = nCount;
#ifdef IDV_OPENGL
    m_anTexCoordOffsets[nLayer] = nOffset;
#endif
#if IDV_DIRECTX9 | _XBOX
    UNREFERENCED_PARAMETER(nOffset);
    if (nCount == 1)
        m_dwFvfVector |= D3DFVF_TEXCOORDSIZE1(nLayer);
    else if (nCount == 2)
        m_dwFvfVector |= D3DFVF_TEXCOORDSIZE2(nLayer);
    else if (nCount == 3)
        m_dwFvfVector |= D3DFVF_TEXCOORDSIZE3(nLayer);
    else if (nCount == 4)
        m_dwFvfVector |= D3DFVF_TEXCOORDSIZE4(nLayer);
    else
        IdvInternal("Invalid else clause in CIdvGeometryBuffer::SetTexCoordOffset()");
#endif
}


///////////////////////////////////////////////////////////////////////  
//  CIdvInterleavedBuffer::SetNormalOffset

void CIdvInterleavedBuffer::SetNormalOffset(int nOffset)
{
    st_assert(nOffset > -1);

#ifdef IDV_OPENGL
    m_nNormalOffset = nOffset;
#endif
#if IDV_DIRECTX9 | _XBOX
    UNREFERENCED_PARAMETER(nOffset);
    m_dwFvfVector |= D3DFVF_NORMAL;
#endif
}


///////////////////////////////////////////////////////////////////////  
//  CIdvInterleavedBuffer::SetColorOffset

void CIdvInterleavedBuffer::SetColorOffset(int nOffset)
{
    st_assert(nOffset > -1);

#ifdef IDV_OPENGL
    m_nColorOffset = nOffset;
#endif
#if IDV_DIRECTX9 | _XBOX
    UNREFERENCED_PARAMETER(nOffset);
    m_dwFvfVector |= D3DFVF_DIFFUSE;
#endif
}


///////////////////////////////////////////////////////////////////////  
//  CIdvInterleavedBuffer::SetBuffer

void CIdvInterleavedBuffer::SetBuffer(const void* pBuffer, int nNumVertices, int nOffset)
{
    st_assert(m_nVertexSize > -1);
    st_assert(nNumVertices <= NumVertices( ));

#ifdef IDV_OPENGL
    if (!m_uiVertexBuffer)
        CreateBuffer( );

    st_assert(m_uiVertexBuffer);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_uiVertexBuffer);
    if (nOffset == 0)
        glBufferDataARB(GL_ARRAY_BUFFER_ARB, GLsizeiptrARB(nNumVertices * m_nVertexSize), pBuffer, m_bDynamic ? GL_DYNAMIC_DRAW_ARB : GL_STATIC_DRAW_ARB);
    else
        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, GLintptrARB(nOffset), GLsizeiptrARB(nNumVertices * m_nVertexSize), pBuffer);

    // unbind
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
#endif
#if IDV_DIRECTX9 | _XBOX
    st_assert(m_dxVertexBuffer);

    void* pVertexBufferContents = NULL;
    const int nUpdateSize = nNumVertices * m_nVertexSize;
#ifdef _XBOX
    Dx9Verify(m_dxVertexBuffer->Lock(0, 0, reinterpret_cast<void**>(&pVertexBufferContents), 0));
#else
    Dx9Verify(m_dxVertexBuffer->Lock(nOffset, nUpdateSize, reinterpret_cast<void**>(&pVertexBufferContents), 0));
#endif
    memcpy(pVertexBufferContents, pBuffer, nUpdateSize);
    Dx9Verify(m_dxVertexBuffer->Unlock( ));
#endif
}


///////////////////////////////////////////////////////////////////////  
//  CIdvInterleavedBuffer::NumVertices

int CIdvInterleavedBuffer::NumVertices(void) const
{
    return m_nNumVertices;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvInterleavedBuffer::SetMaxNumVertices

void CIdvInterleavedBuffer::SetMaxNumVertices(int nNumVertices)
{
    m_nNumVertices = nNumVertices;

#if IDV_DIRECTX9 | _XBOX
    if (!m_dxVertexBuffer)
        CreateBuffer( );
#endif
#ifdef IDV_OPENGL
    if (!m_uiVertexBuffer)
        CreateBuffer( );
    st_vector_byte vEmpty(nNumVertices * m_nVertexSize);
    SetBuffer(&vEmpty[0], nNumVertices);
#endif
}


///////////////////////////////////////////////////////////////////////  
//  CIdvInterleavedBuffer::CreateBuffer

void CIdvInterleavedBuffer::CreateBuffer(void)
{
#ifdef IDV_OPENGL
    // generate the ARB buffer
    glGenBuffersARB(1, &m_uiVertexBuffer);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_uiVertexBuffer);

    // unbind this buffer
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
#endif

#if IDV_DIRECTX9 | _XBOX
    // finish defining the flexible vertex format (need to know how many active texture layers)
    DWORD dwNumLayers = 0;
    for (int i = 0; i < c_nMaxTextures; ++i)
    {
        if (m_anMaxTexCoordSize[i] > 0)
            ++dwNumLayers;
    }
    if (dwNumLayers > 0)
        m_dwFvfVector |= dwNumLayers << D3DFVF_TEXCOUNT_SHIFT;

    // generate the DX9 vertex buffer
    st_assert(m_pDx);
    const int c_nBufferSize = NumVertices( ) * m_nVertexSize;
#ifdef IDV_DIRECTX9
    Dx9Verify(m_pDx->CreateVertexBuffer(c_nBufferSize, D3DUSAGE_WRITEONLY, m_dwFvfVector, D3DPOOL_DEFAULT, &m_dxVertexBuffer, NULL));
#endif
#ifdef _XBOX
    Dx9Verify(m_pDx->CreateVertexBuffer(c_nBufferSize, D3DUSAGE_WRITEONLY, m_dwFvfVector, D3DPOOL_DEFAULT, &m_dxVertexBuffer, NULL));
#endif
#endif
}


///////////////////////////////////////////////////////////////////////  
//  CIdvTightlyPackedBuffer::AddVertices

void CIdvTightlyPackedBuffer::AddVertices(int nNumVertices, int nNumCoords, const float* pVertices)
{
    st_assert(pVertices);

    if (nNumVertices > 0 && nNumCoords > 0)
    {
        // adjust running maxcoord size
        m_nMaxCoordSize = nNumCoords;

        int nPreCopySize = int(m_vCoords.size( ));
        m_vCoords.resize(nPreCopySize + nNumCoords * nNumVertices);

        memcpy(&m_vCoords[nPreCopySize], pVertices, nNumVertices * nNumCoords * sizeof(float));
    }
}


///////////////////////////////////////////////////////////////////////  
//  CIdvTightlyPackedBuffer::AddTexCoords

void CIdvTightlyPackedBuffer::AddTexCoords(int nNumVertices, int nLayer, int nNumCoords, const float* pTexCoords)
{
    st_assert(pTexCoords);
    st_assert(nLayer > -1 && nLayer < c_nMaxTextures);

    if (nNumVertices > 0 && nNumCoords > 0)
    {
        // adjust running maxcoord size
        m_anMaxTexCoordSize[nLayer] = nNumCoords;

        int nPreCopySize = int(m_avTexCoords[nLayer].size( ));
        m_avTexCoords[nLayer].resize(nPreCopySize + nNumCoords * nNumVertices);

        memcpy(&m_avTexCoords[nLayer][nPreCopySize], pTexCoords, nNumVertices * nNumCoords * sizeof(float));
    }
}


///////////////////////////////////////////////////////////////////////  
//  CIdvTightlyPackedBuffer::AddNormals

void CIdvTightlyPackedBuffer::AddNormals(int nNumVertices, const float* pNormals)
{
    st_assert(pNormals);

    if (nNumVertices > 0)
    {
        int nPreCopySize = int(m_vNormals.size( ));
        m_vNormals.resize(nPreCopySize + 3 * nNumVertices);

        memcpy(&m_vNormals[nPreCopySize], pNormals, nNumVertices * 3 * sizeof(float));
    }
}


///////////////////////////////////////////////////////////////////////  
//  CIdvTightlyPackedBuffer::AddColors

void CIdvTightlyPackedBuffer::AddColors(int nNumVertices, const float* pColors)
{
    st_assert(pColors);

    if (nNumVertices > 0)
    {
        int nPreCopySize = int(m_vColors.size( ));
        m_vColors.resize(nPreCopySize + nNumVertices);

        for (int i = 0; i < nNumVertices; ++i)
            m_vColors[nPreCopySize++] = ColorFloatsToUInt(pColors[0], pColors[1], pColors[2], pColors[3]);
    }
}


///////////////////////////////////////////////////////////////////////  
//  CIdvTightlyPackedBuffer::AddColors

void CIdvTightlyPackedBuffer::AddColors(int nNumVertices, const IdvColorType* pColors)
{
    st_assert(pColors);

    if (nNumVertices > 0)
    {
        int nPreCopySize = int(m_vColors.size( ));
        m_vColors.resize(nPreCopySize + nNumVertices);

        memcpy(&m_vColors[nPreCopySize], pColors, nNumVertices * sizeof(IdvColorType));
    }
}



