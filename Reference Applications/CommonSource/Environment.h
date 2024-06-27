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
#include "IdvOpenGL.h"
#include "IdvTexture.h"
#include "IdvGeometryBuffer.h"
#include "IdvShaderManager.h"


// forward references
class CSpeedTreeCullEngine;


///////////////////////////////////////////////////////////////////////  
//  class CEnvironment

class CEnvironment
{
public:
        CEnvironment( );
        ~CEnvironment( );

        bool                    IsInitialized(void) const;
        bool                    Init(const st_string& strResourcePath, 
                                     const st_string& strShaderFile, 
                                     float fFogStart, 
                                     float fFogEnd,
                                     const float* pFogColor);
        void                    Draw(const float* pExtents);

private:
        bool                    LoadShader(const st_string& strShaderFile);
        void                    SetupIndices(void);

        CIdvTexture             m_texGround;                        // grid texture (alpha plane is used)
        CIdvInterleavedBuffer   m_cGeometry;                        // dynamic buffer used for moving ground
        int                     m_nNumVerts;                        // total size of the geometry buffer

#ifndef LOAD_BINARY_SHADERS
        CIdvEffect              m_cEffect;                          // ground shader (effect)
#endif
        CIdvTechnique           m_cTechnique;                       // ground shader (specific ground technique)
        CIdvConstant            m_conWorldViewProjectionMatrix;     // shader var: mirrors the world/projection matrix from the forest
        CIdvConstant            m_conColor;                         // shader var: ground modulated color
        CIdvConstant            m_conTexture;                       // shader var: ground texture
        CIdvConstant            m_conFogColor;                      // shader var: mirrors forest fog color
        CIdvConstant            m_conFogParams;                     // shader var: mirrors forest fog params

        // fog parameters
        float                   m_fFogStart;
        float                   m_fFogEnd;
        idv::Vec3               m_cFogColor;
};

