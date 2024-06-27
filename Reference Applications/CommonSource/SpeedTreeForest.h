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
#include "SpeedTreeInstance.h"
#include "IdvGeometryBuffer.h"
#include "SpeedTreeCullEngine.h"
#include "IdvTexture.h"
#include "IdvShaderManager.h"
#include "SpeedTreeWrapper.h"
#include "IdvGeometryBuffer.h"
#define SPEEDTREE_FRAMEWORK
#include "SpeedTreeMemory.h"
#include "Environment.h"


// forward references
class CSpeedTreeInstance;
class CSpeedTreeWrapper;


///////////////////////////////////////////////////////////////////////  
//  class CSpeedTreeForest declaration

class CSpeedTreeForest
{
public:
        enum EDrawFlags
        {
            BRANCHES    = (1 << 0),
            LEAVES      = (1 << 1),
            FRONDS      = (1 << 2),
            BILLBOARDS  = (1 << 3),
            ALL         = ((1 << 4) - 1)
        };

        struct SDrawStats
        {
            SDrawStats( )
            {
                Reset( );
            }

            void        Reset(void)
            {
                m_nNumTrianglesDrawn = 0;
                m_nNumDrawCalls = 0;
                m_nNumBBs = 0;
                m_nNum3DTrees = 0;
                m_nBufferBinds = 0;
                m_fUpdateTime = 0.0f;
            }

            int         m_nNumTrianglesDrawn;
            int         m_nNumDrawCalls;
            int         m_nNumBBs;
            int         m_nNum3DTrees;
            int         m_nBufferBinds;
            float       m_fUpdateTime;
        };

                                            CSpeedTreeForest( );
                                            ~CSpeedTreeForest( );

        void                                Draw(unsigned int uiFlagVector);
        void                                DrawGround(void);
        void                                DrawOverlay(void);
        const SDrawStats&                   GetDrawStats(void) const                            { return m_sDrawStats; }

        // populate forest with STF file
        bool                                PopulateFromStfFile(const char* pFilename);

        // populate forest randomly (used largely for testing)
        bool                                PopulateRandomly(const float* pExtents, const st_vector_string& vSptFiles, int nNumTrees);


        // populate forest (base trees)
        struct SBaseParams
        {
            SBaseParams( ) :
                m_nSeed(1),
                m_fSize(-1.0f),
                m_fSizeVariance(0.0f)
            {
            }
            ~SBaseParams( ) 
            { 
            }

            st_string           m_strSptFile;
            int                 m_nSeed;
            float               m_fSize;
            float               m_fSizeVariance;
        };

        // populate forest (instances)
        struct SInstanceParams
        {
            SInstanceParams( ) :
                m_nBaseIndex(-1),
                m_fScale(1.0f),
                m_fRotation(0.0f)
            {
                m_afPos[0] = m_afPos[1] = m_afPos[2] = 0.0f;
            }
            ~SInstanceParams( ) 
            { 
            }

            int                 m_nBaseIndex;   // zero-based index into SPT file vector
            float               m_afPos[3];     // (x,y,z) position in world coords
            float               m_fScale;       // scale of tree (1.0 == default scale)
            float               m_fRotation;    // tree rotation in radians
        };


        ///////////////////////////////////////////////////////////////////////  
        //  SpeedTree memory management
        //
        //  Used to capture allocations in STL containers to report back to
        //  SpeedTreeRT's allocation mechanism

        DefineAllocator(st_allocator_baseparams);
        typedef std::vector<CSpeedTreeForest::SBaseParams, st_allocator_baseparams<CSpeedTreeForest::SBaseParams> > st_vector_baseparams;
        DefineAllocator(st_allocator_instparams);
        typedef std::vector<CSpeedTreeForest::SInstanceParams, st_allocator_instparams<CSpeedTreeForest::SInstanceParams> > st_vector_instparams;

        bool                                PopulateManually(const SBaseParams* pBaseTrees, int nNumBaseTrees, const SInstanceParams* pInstances, int nNumInstances, const idv::Region& cExtents = idv::Region( ));

        // settings
        void                                SetShaderFile(const st_string& strFilename)         { m_strMainShaderFile = strFilename; }
        void                                SetSpeedWindFile(const st_string& strFilename)      { m_strSpeedWindFile = strFilename; }
        void                                SetLighting(const float* pLightDir, const float* pLightColor, float fLightScale = 1.0f);
        void                                SetFogParams(float fStart, float fEnd, const float* pColor);
        void                                SetLodLimits(float fNear, float fFar)               { m_fNearLod = fNear; m_fFarLod = fFar; }
        void                                SetTime(float fTime); // in seconds
        void                                ToggleAlphaTesting(void)                            { m_bAlphaTesting = !m_bAlphaTesting; }
        void                                ToggleWireframe(void)                               { m_bWireframe = !m_bWireframe; }
        void                                ToggleNormalMapping(void)                           { m_bNormalMapping = !m_bNormalMapping; }
        void                                ToggleWindStrength(void);
        void                                ToggleVisibility(void)                              { m_bVisible = !m_bVisible; }
        void                                OverlayZoom(float fDirection);

        // camera
        void                                UpdateCamera(const float* pCameraPos, const float* pProjectionMatrix, const float* pModelviewMatrix, bool bOverride = false);
        void                                SetProjection(float fAspectRatio, float fFieldOfView, float fNearClip, float fFarClip); // FOV in degrees
        void                                SetLodDistances(float fNear, float fFar)            { m_cCullEngine.SetLodDistances(fNear, fFar); }
        void                                ForceCameraUpdate(void);
        void                                SetWindowBounds(unsigned int uiWidth, unsigned int uiHeight);

        // queries
        const float*                        Extents(void) const                                 { return m_afExtents; }
        int                                 NumTrees(void) const                                { return int(m_vAllTrees.size( )); }
        bool                                IsVisible(void) const                               { return m_bVisible; }
        const st_vector_stinstance&         GetAllTrees(void) const                             { return m_vAllTrees; }

        // debug
        void                                ToggleDebugFlag(void)                               { m_bDebugFlag = !m_bDebugFlag; }
        void                                DebugSpin(float fAngleInc);

private:
        bool                                InitGraphics(const st_string& strResourcePath);
        void                                BuildBillboardTexcoordsTable(void);

        void                                AddInstance(CSpeedTreeInstance* pTree);
        void                                AddBaseTree(CSpeedTreeWrapper* pTree);
        void                                InitInstanceBuffers(void);
        void                                UploadBillboardData(bool bLeaves, int nPass = 0);
        void                                InitWind(const st_string& strSpeedWindFile);
        void                                UploadWindMatricesToShader(void);
        void                                UploadLeafWindAngles(const CIdvConstant& cParameter);

        void                                DrawBranches(void);
        void                                DrawBranchesWithInstancing(void);
        void                                DrawFronds(void);
        void                                DrawLeafCards(void);
        void                                DrawLeafMeshes(void);
        void                                DrawVertBillboards(int nPass);
#ifdef HORZ_BILLBOARDS
        void                                DrawHorzBillboards(void);
#endif

        // helpful diagnostic tools
        void                                DrawCells(void);
        void                                DrawCullRadii(void);

        void                                LoadTextures(const st_string& strTexturePath);
        bool                                LoadShaders(const CSpeedWind& cWind);
        void                                AdjustExtents(float x, float y, float z);
        void                                UploadLightAdjustment(void);
        bool                                AdjustSelfShadow(bool bGeometryReceivesShadow);

        void                                ResolveCompositeTextureFilenames(const st_string& strPath, st_string& strCompositeDiffuseLeaf, st_string& strCompositeNormalLeaf, st_string strCompositeBB[2], st_string& strCompositeShadow);

        // tree containers
        st_vector_stwrapper_p               m_vBaseTrees;                   // stores the base trees (SPT files) defining a forest (typically 8-12 trees)
        st_vector_stinstance                m_vAllTrees;                    // stores every instance in the forest, sorted by base tree type

        float                               m_afExtents[6];                 // [0-2] = min (xyz), [3-5] = max (xyz)
        float                               m_afLastModelviewMatrix[16];    // stores the modelview and projection matrices in order to determine if
        float                               m_afLastProjectionMatrix[16];   // the camera has actually changes in order to perform another cull

        // lighting
        float                               m_afLightDir[3];                // (xyz) normalized light direction
        float                               m_afLightColor[3];              // (rgb) light color (each component is in range [0,1])
        float                               m_fLightScale;                  // allows global scaling of overall scene lighting

        // geometry
        CIdvTightlyPackedBuffer             m_cBranchGeometry;              // stores the branch geometry for all base trees in a single VB/IB pair
        CIdvTightlyPackedBuffer             m_cFrondGeometry;               // stores the frond geometry for all base trees in a single VB/IB pair
        CIdvTightlyPackedBuffer             m_cLeafCardGeometry;            // stores the leaf card geometry for all base trees in a single VB
        CIdvTightlyPackedBuffer             m_cLeafMeshGeometry;            // stores the leaf mesh geometry for all base trees in a single VB/IB pair

        // textures
        CIdvTexture                         m_texLeavesAndFronds[CSpeedTreeRT::TL_NUM_TEX_LAYERS];  // leaf/frond composite texture for all supported layers (CAD exported)
        CIdvTexture                         m_texBillboards[CSpeedTreeRT::TL_NUM_TEX_LAYERS];       // billboard composite texture for all supported layers (CAD exported)
        CIdvTexture                         m_texSelfShadow;                // shared composite self-shadow, cast onto branch and frond geometry (CAD exported)
        CIdvTexture                         m_texWhite;                     // used in place of missing textures
        CIdvTexture                         m_texNoDetail;                  // white with black alpha plane, used in place of missing detail texture layers
        CIdvTexture                         m_texFlatNormalMap;             // used when normal mapping effect is disabled
        st_vector_float                     m_v360TexCoordTable;            // single table that stores the texcoords into composite billboard map for all of the
                                                                            // base trees in the forest

        // render parameters/flags
        float                               m_fFogStart;                    // distance that linear fog begins
        float                               m_fFogEnd;                      // distance that linear fog ends and continues
        float                               m_afFogColor[4];                // (rgba) fog color (each component is in range [0,1])
        bool                                m_bSelfShadowBound;             // used to track when self shadow is bound/unbound when base tree doesn't accept self shadows
        bool                                m_b360BillboardsSupported;      // true if loaded SPT files have 360-degree billboards, false otherwise
        bool                                m_bAlphaTesting;                // used to toggle alpha testing (debug)
        bool                                m_bWireframe;                   // used to toggle render mode
        bool                                m_bNormalMapping;               // used to toggle normal mapping
        bool                                m_bVisible;                     // used to toggle visibility when multiple forests are drawn
        float                               m_fNearLod;                     // near LOD value used by SpeedTreeRT
        float                               m_fFarLod;                      // far LOD value used by SpeedTreeRT
        bool                                m_bDebugFlag;                   // generic flag used to toggle various debug states

        // render utilities/data
        CSpeedTreeCullEngine                m_cCullEngine;                  // culling object used to determine visibility and LOD
        SDrawStats                          m_sDrawStats;                   // tracking the render stats for the forest
static  CEnvironment*                       m_pEnvironment;
static  int                                 m_nRefCount;

        // wind
        st_string                           m_strSpeedWindFile;             // stores SpeedWind filename, set by application
        CSpeedWind                          m_cSpeedWind;                   // SpeedWind object that controls wind effects for entire forest
        float                               m_fTime;                        // tracks time, used in wind effects (in seconds)
        st_vector_cfloat_p                  m_vWindMatrices;                // vector of float pointers storing wind matrices for each matrix defined by SpeedWind
        st_vector_float                     m_vWindRustleAngles;            // table of leaf rustle angles shared by all base trees
        st_vector_float                     m_vWindRockAngles;              // table of leaf rock angles shared by all base trees
        st_vector_float                     m_vLeafAngleUploadTable;        // used as a repository to batch shader constant uploads
        float                               m_fSelfShadowAdvance;           // used to animate the self-shadow texture based on global wind strength

        // shaders
        st_string                           m_strMainShaderFile;            // stores main shader FX file location, set by application
        st_string                           m_strEnvironmentShaderFile;     // stores auxiliary shader FX file location, set by application
#ifndef LOAD_BINARY_SHADERS
        CIdvEffect                          m_cEffect;                      // abstracts CgFX/HLSL FX effect
#endif
        CIdvTechnique                       m_cBranchTechnique;             // abstracts CgFX/HLSL FX technique for branch geometry
        CIdvTechnique                       m_cFrondTechnique;              // abstracts CgFX/HLSL FX technique for frond geometry
        CIdvTechnique                       m_cLeafCardTechnique;           // abstracts CgFX/HLSL FX technique for leaf card geometry
        CIdvTechnique                       m_cLeafMeshTechnique;           // abstracts CgFX/HLSL FX technique for leaf mesh geometry
        CIdvTechnique                       m_cBillboardTechniques[2];      // abstracts CgFX/HLSL FX technique for vertical billboard geometry
        CIdvTechnique                       m_cHorzBillboardTechnique;      // abstracts CgFX/HLSL FX technique for horizontal billboard geometry
        CIdvTechnique                       m_cOverlayTechnique;            // abstracts CgFX/HLSL FX technique for overlay geometry

        // forest overlay data
        unsigned int                        m_uiScreenWidth;                // set during window initialization by application
        unsigned int                        m_uiScreenHeight;               // set during window initialization by application
        float                               m_fOverlayZoomFactor;

        // all constants below correspond one-to-one with shader constants defined
        // SpeedTree.fx and associated files

        // effect constants (shared by one or more geometry type)
        CIdvConstant                        m_conWorldViewProjectionMatrix;
        CIdvConstant                        m_conTreePos;
        CIdvConstant                        m_conTreeRotationTrig;
        CIdvConstant                        m_conTreeScale;
        CIdvConstant                        m_conWindMatrices;
        CIdvConstant                        m_conWindMatrixOffset;
        CIdvConstant                        m_conLightDir;
        CIdvConstant                        m_conLightColor;
        CIdvConstant                        m_conLightColor_PS;
        CIdvConstant                        m_conAmbientMaterial;
        CIdvConstant                        m_conDiffuseMaterial;
        CIdvConstant                        m_conAmbientScale;
        CIdvConstant                        m_conDiffuseScale;
        CIdvConstant                        m_conSelfShadowMap;
        CIdvConstant                        m_conDiffuseCompositeLeafMap;
        CIdvConstant                        m_conNormalCompositeLeafMap;
        CIdvConstant                        m_conCameraAngles;
        CIdvConstant                        m_conCameraAzimuthTrig;
        CIdvConstant                        m_conSelfShadowMotion;
        CIdvConstant                        m_conFogColor;
        CIdvConstant                        m_conFogParams;
#ifdef INTERMEDIATE_FRONDBRANCH_FADING
        CIdvConstant                        m_conBranchLodValues;
#endif
        CIdvConstant                        m_conOverlayColor;

        // branch-specific constants
        CIdvConstant                        m_conBranchDiffuseMap;
        CIdvConstant                        m_conBranchNormalMap;
#ifdef BRANCH_DETAIL_LAYER
        CIdvConstant                        m_conBranchDetailMap;
#endif

        // leaf-specific constants
        CIdvConstant                        m_conLeafCardAngles;
        CIdvConstant                        m_conLeafCardAngleScalars;
        CIdvConstant                        m_conLeafMeshAngles;
        CIdvConstant                        m_conLeafMeshAngleScalars;
        CIdvConstant                        m_conLeafLightingAdjust;

        // billboard-specific constants
        CIdvConstant                        m_con360TexCoords;
        CIdvConstant                        m_conSpecialAzimuth[2];
        CIdvConstant                        m_conBillboardDiffuseMap;
        CIdvConstant                        m_conBillboardNormalMap;
        CIdvConstant                        m_conLightAdjustment;
        CIdvConstant                        m_conBBNormals[2];
        CIdvConstant                        m_conBBBinormals[2];
        CIdvConstant                        m_conBBTangents[2];
        CIdvConstant                        m_conHorzBillboardFadeValue;
};

DefineAllocator(st_allocator_forest);
typedef std::vector<CSpeedTreeForest*, st_allocator_forest<CSpeedTreeForest*> > st_vector_forestp;
