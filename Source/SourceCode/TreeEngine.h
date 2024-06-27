///////////////////////////////////////////////////////////////////////  
//  TreeEngine.h
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
//      http://www.idvinc.com

#pragma once

#include "LibGlobals_Source/IdvGlobals.h"
#include "LibFilename_Source/IdvFilename.h"
#include "LibVector_Source/IdvVector.h"
#include "LibSpline_Source/IdvSpline.h"
#include "LibRandom_Source/IdvRandom.h"
#include "StructsInfo.h"
#include "StructsSupport.h"
#include "FileAccess.h"
#include "FrondEngine.h"
#include "BillboardLeaf.h"


// forward references
class CBillboardLeaf;
struct SIdvLeaf;
class CIndexedGeometry;
class CLightingEngine;

const int c_nBankSize = 1000;


///////////////////////////////////////////////////////////////////////  
//  class stCamera declaration

class stCamera
{
public:
        stCamera( );
        stCamera(const stCamera& cTrans);
virtual ~stCamera( ) { }

        // geometry/position related functions
static  void                    SetCamera(const stVec3& cCameraPos, const stVec3& cCameraDir)   { m_cCameraPos = cCameraPos; m_cCameraDir = cCameraDir; }
static  void                    GetCamera(stVec3& cCameraPos, stVec3& cCameraDir)               { cCameraPos = m_cCameraPos; cCameraDir = m_cCameraDir; }

        const stVec3&           GetPosition(void) const                                         { return m_cPosition; }
        void                    SetPosition(const stVec3& cPosition)                            { m_cPosition = cPosition; }

virtual const stCamera&         operator=(const stCamera& cRight);

protected:      
static  stVec3                  m_cCameraPos;               // one camera position shared by the entire library
static  stVec3                  m_cCameraDir;               // one camera direction shared by the entire library
        stVec3                  m_cPosition;                // one position for each derived instance
};


///////////////////////////////////////////////////////////////////////  
//  class CBranch declaration

class CBranch;
DefineAllocator(st_allocator_branches);
typedef std::vector<CBranch*, st_allocator_branches<CBranch*> > st_vector_branches;

class CBranch
{
friend class CTreeEngine;

public:
        CBranch(CBranch* pParent, bool bDestroyChildren = true);
        ~CBranch( );

        // branch construction
        CFrondEngine::EConditions   Compute(unsigned int nSeed, float fSize, int nLevel, const stVec3& cBasePos, float fPercentOfParent, const stRotTransform& cBaseTransform, const stVec3& cZAxis, CIndexedGeometry* pBranchGeometry, st_vector_leaves& vLeaves, float fWindWeight1, float fWindWeight2, int nWindGroup1, int nWindGroup2, float fParentRadius, bool bUseRoots = false);
        void                        ComputeBud(const CBranch* pParent, float fSize, int nLevel, const stVec3& cBasePos, float fPercentOfParent, const stRotTransform& cBaseTransform, const stVec3& cZAxis, st_vector_leaves& vLeaves, float fWindWeight1, float fWindWeight2, int nWindGroup1, int nWindGroup2) const;
        void                        ComputeLod(int nDiscreteLodLevel, CIndexedGeometry* pGeometry) const;

        // wind
static  void                        SetBranchLevelForWeighting(int nLevel)                  { m_nWeightLevel = nLevel; }

        // LOD
        void                        BuildBranchVector(st_vector_branches& vAllBranches);
static  void                        SortBranchVector(st_vector_branches& vAllBranches);

        // utility
static  void                        AddBranchInfo(const SIdvBranchInfo* pInfo)              { m_vBranchInfo.push_back(pInfo); }
static  void                        ClearBranchInfo(void)                                   { m_vBranchInfo.clear( ); }
static  const st_vector_const_branchinfo& GetBranchInfoVector(void)                         { return m_vBranchInfo; }
        const CBranch*              GetParent(void) const                                   { return m_pParent; }
        float                       GetVolume(void) const                                   { return m_fVolume; }
        float                       GetFuzzyVolume(void) const                              { return m_fFuzzyVolume; }
        void                        SetFuzzyVolume(const float fVolume)                     { m_fFuzzyVolume = fVolume; }
static  void                        Parse(CTreeFileAccess& cFile);
static  void                        SetComputeLeaves(bool bState)                           { m_bComputeLeaves = bState; }
static  void                        SetLeafData(SIdvLeafInfo* pLeafInfo)                    { m_pLeafInfo = pLeafInfo; }
static  void                        SetLightingEngine(CLightingEngine* pLightingEngine)     { m_pLightingEngine = pLightingEngine; }
static  void                        SetFrondEngine(CFrondEngine* pFrondEngine)              { m_pFrondEngine = pFrondEngine; }
static  void                        SetRoundRobinWindLevel(int nLevel)                      { m_nRoundRobinWindLevel = nLevel; }
static  void                        SetPropagateFlexibility(bool bState)                    { m_bPropagateFlexibility = bState; }
static  bool                        GetPropagateFlexibility( )                              { return m_bPropagateFlexibility; }

        CBranch&                    operator=(const CBranch& cRight);

private:
        // leaf-related
        void                        BuildBlossomVectors(void);
        bool                        IsBlossom(const CBranch* pParent, float fPercentOfParent) const;
        void                        MakeLeaf(const stVec3& cPos, float fPercent, const CBranch* pParent, const stVec3& cNormal, const stVec3& cParentDir, float fWindWeight, float fWindWeight2, int nWindGroup1, int nWindGroup2, st_vector_leaves& vLeaves) const;
        bool                        RoomForLeaf(const stVec3& cPos, int nTextureIndex, const st_vector_leaves& vLeaves) const;

        // utility
        void                        BuildCrossSection(const SIdvBranchVertex* pVertex, float fBranchProgress, int nSegments, CIndexedGeometry* pBranchGeometry, const SIdvBranchInfo& sInfo, float fWindWeight1, float fWindWeight2, int nWindGroup1, int nWindGroup2, float fTileOffset, const SIdvBranchInfo* pInfo, float fMaxPossibleRadius, float fSize, float fBaseRadius, float fLength, bool bNegateTwist) const;
        void                        ComputeBranchNormals(CIndexedGeometry* pBranchGeometry, const stBezierSpline* pLightSeamProfile, float fLightSeamBias);
        void                        ComputeVertexWeights(float& fWeight1, float& fWeight2, int nLevel, float fProgress, float fFlexibility);
        void                        ComputeVolume(void);
        void                        FillBranch(SIdvBranch& sBranch, float fLength) const;
        float                       ComputeForkPercent(float fPercent, float fLength, float fForkAngle, int nForkLimit) const;
        float                       GetPercentOfParent(void) const                          { return m_fPercentOfParent; }
        bool                        Prune(const CBranch* pParent, float fPercentOfParent, unsigned int uiLevel, float fDistancePercent) const;
        void                        ComputeTexCoords(float& fS, float& fT, const SIdvBranchInfo::STexCoordControls& sControls, float fRadialProgress, float fBranchProgress, float fTileOffset, float fBaseRadius, float fLength, bool bNegateTwist) const;

        // frond related
        CFrondEngine::EConditions   PassesFrondRules(const CBranch* pParent, float fPercentOfParent) const;

        // flare related
        void                        ComputeFlareEntries(const SIdvBranchInfo& sInfo);

        CBranch*                    m_pParent;                  // parent branch
        float                       m_fPercentOfParent;         // percentage along parent branch's length this branch was created
        st_vector_sbranches         m_vChildren;                // list of this branch's children

        SIdvBranchVertex*           m_pVertices;                // used by leaf bud
        int                         m_nNumVertices;             // used by leaf bud (can be replaced by constant)

        unsigned short              m_usCrossSectionSegments;   // radial resolution
        int                         m_nStartingOffset;          // marks the start of this branch's data within the branch geometry class
        float                       m_fVolume;                  // volume of this branch's geometry
        float                       m_fFuzzyVolume;             // fuzzy volume used to influence sorting order for lod branch removal
        bool                        m_bDestroyChildren;

        enum { c_nMaxStackPoints = 10 };
        SIdvBranchVertex            m_asVerticesStack[c_nMaxStackPoints];
        bool                        m_bUsingVertexStack;

        // flare related
        st_vector_branchflare       m_vFlares;                  // branch flares

        // the following variables are static because they need to be shared
        // across all branch levels for a single tree
static  int                         m_nRefCount;
static  st_vector_int               m_vBlossoms;                // a list of which leaf textures represent blossoms
static  st_vector_int               m_vNonBlossoms;             // a list of which leaf textures do not represent blossoms

static  st_vector_const_branchinfo  m_vBranchInfo;              // all of the user-specified information for creating each branch level
static  bool                        m_bComputeLeaves;           // set to false is leaves have been read from file, true is they should be generated
static  SIdvLeafInfo*               m_pLeafInfo;                // pointer to struct with leaf information
static  st_vector_leaves            m_vLocalLeaves;             // a list of leaves assigned to a single branch - used in BRANCH collision mode

static  CLightingEngine*            m_pLightingEngine;          // pointer to SpeedTreeRT lighting engine, used to set static branch colors
static  stRandom                    m_cRandom;                  // common single random number generator            
static  int                         m_nWeightLevel;             // level at which wind effect takes place
static  int                         m_nRoundRobinWindLevel;     // used to ensure a good wind matrix distribution

        // frond
static  CFrondEngine*               m_pFrondEngine;             // pointer to SpeedTreeRT frond engine

        // wind
static  bool                        m_bPropagateFlexibility;    // allow levels higher than the wind level to retain flexibility

        // floor
static  bool                        m_bFloorEnabled;            // enables/disables floor limiting
static  float                       m_fFloor;                   // controls the lower limit of branch vertices
static  int                         m_nFloorLevel;              // the level at and above which floor limiting can occur
static  float                       m_fFloorExponent;           // controls floor influence as branches grow out
static  float                       m_fFloorBias;               // controls what percentage of the segment's length can go below the floor

        // cluster support
static  int                         m_nFirstBranchLevel;        // determines the branch level above which radius limiting is enforced

        // root support
static  SRootSupportInfo*           m_pRootSupport;

        // roughness
static  float                       m_afRandomBank[c_nBankSize];
static  int                         m_nRandomIndex; 

        // frequency
static  int                         m_nFrequencyIndex;

        // mesh rotation
static  int                         m_nMeshRotateIndex;
};


///////////////////////////////////////////////////////////////////////  
//  class CTreeEngine declaration

class CTreeEngine : public stCamera 
{
public:
        CTreeEngine(CIndexedGeometry* pBranchGeometry);
virtual ~CTreeEngine( );

        // tree construction
        void                        Compute(float fLeafSizeIncreaseFactor);
        void                        Clone(CTreeEngine* pClone, const stVec3& cPos, unsigned int nSeed = 0) const;
        void                        OverrideTreeSize(float fNewSize, float fNewVariance);

        bool                        Parse(CTreeFileAccess& cFile);
        void                        ParseTreeInfo(CTreeFileAccess& cFile);
        void                        Save(CTreeFileAccess& cFile, bool bSaveLeafLods = false) const;

        void                        FreeTransientData(void);
        bool                        TransientDataIntact(void) const                 { return m_bTransientDataIntact; }

        // leaves
        st_vector_leaves*           GetAllLeaves(void) const                        { return m_pLeafLods; }
        const SIdvLeafInfo&         GetLeafInfo(void) const                         { return m_sLeafInfo; }
        const st_vector_leaftexture& GetLeafTextures(void) const                    { return m_sLeafInfo.m_vLeafTextures; }
        void                        InitTables(void)                                { m_sLeafInfo.InitTables(int(m_sLeafInfo.m_vLeafTextures.size( ))); }
        void                        SetNumLeafRockingGroups(int nCount)             { m_sLeafInfo.m_nNumRockingGroups = nCount; }

        // wind
        const SIdvWindInfo&         GetWindData(void) const                         { return m_sWindInfo; }
        void                        SetBranchLevelForWeighting(int nLevel)          { m_nBranchesWeightLevel = nLevel; }
        int                         GetBranchLevelForWeighting( )                   { return m_nBranchesWeightLevel; }
        void                        SetWind(float fStrength);
        void                        SetWindData(const SIdvWindInfo& sWindInfo)      { m_sWindInfo = sWindInfo; }

        // utility
        void                        ClearBranchInfo(void)                           { m_vBranchInfo.clear( ); }
        const st_vector_branchinfo& GetBranchInfoVector(void)                       { return m_vBranchInfo; }
        const st_string&            GetBranchTextureFilename(void) const            { return m_sTreeInfo.m_strBranchTextureFilename; }
        int                         GetSeed(void) const                             { return m_sTreeInfo.m_nSeed; }
        void                        GetSize(float& fSize, float& fVariance);
        void                        SetSize(float fSize, float fVariance);
        void                        SetBranchTextureFilename(const char* pFilename);
        void                        SetLeafTextureFilename(unsigned int nLeafMapIndex, const char* pFilename);
        void                        SynchTexCoords(void);

        // meshes
        void                        AddMeshInfo(SIdvMeshInfo* pMeshInfo);
        const SIdvMeshInfo*         GetMeshInfo(int iIndex) const;
        void                        ClearMeshInfo(void);

        // lod
        float                       ComputeLod(void);
        void                        SetLod(float fLod)                              { m_fLod = fLod; }
        float                       GetLod(void) const                              { return m_fLod; }
        void                        GetLodLimits(float& fNear, float& fFar) const;
        void                        SetLodLimits(float fNear, float fFar);

        int                         GetNumLeafLodLevels(void) const                 { return m_sLeafInfo.m_nNumLeafLodLevels; }
        void                        SetNumLeafLodLevels(unsigned int nNumLevels)    { m_sLeafInfo.m_nNumLeafLodLevels = nNumLevels; }
        unsigned int                GetNumBranchLodLevels(void) const               { return m_nNumBranchLodLevels; }
        void                        SetNumBranchLodLevels(unsigned int nNumLevels)  { m_nNumBranchLodLevels = nNumLevels; }

        void                        SetBranchVolumeLimits(float fMin, float fMax)   { m_fMinBranchVolumePercent = fMin; m_fMaxBranchVolumePercent = fMax; }

        void                        SetPropagateFlexibility(bool bState)            { m_bPropagateFlexibility = bState; }
        bool                        GetPropagateFlexibility(void) const             { return m_bPropagateFlexibility; }

        // file I/O
        void                        SaveLodInfo(CTreeFileAccess& cFile) const;
        void                        ParseLodInfo(CTreeFileAccess& cFile);
        void                        SaveTextureControls(CTreeFileAccess& cFile) const;
        void                        ParseTextureControls(CTreeFileAccess& cFile);
        void                        SaveFlareInfo(CTreeFileAccess& cFile) const;
        void                        ParseFlareInfo(CTreeFileAccess& cFile);
        void                        SaveFlareSeed(CTreeFileAccess& cFile) const;
        void                        ParseFlareSeed(CTreeFileAccess& cFile);
        void                        SaveLightSeamReductionInfo(CTreeFileAccess& cFile) const;
        void                        ParseLightSeamReductionInfo(CTreeFileAccess& cFile);
        void                        SaveSupplementalLeafPlacementInfo(CTreeFileAccess& cFile) const;
        void                        ParseSupplementalLeafPlacementInfo(CTreeFileAccess& cFile);
        void                        SaveSupplementalBranchInfo(CTreeFileAccess& cFile) const;
        void                        ParseSupplementalBranchInfo(CTreeFileAccess& cFile);
        void                        InitMissingBranchProfiles(void);
        void                        SaveFloorInfo(CTreeFileAccess& cFile) const;
        void                        ParseFloorInfo(CTreeFileAccess& cFile);
        void                        SaveLeafNormalSmoothing(CTreeFileAccess& cFile) const;
        void                        ParseLeafNormalSmoothing(CTreeFileAccess& cFile);
        void                        SaveClusterInfo(CTreeFileAccess& cFile) const;
        void                        ParseClusterInfo(CTreeFileAccess& cFile);
        void                        SaveRootSupportInfo(CTreeFileAccess& cFile) const;
        void                        ParseRootSupportInfo(CTreeFileAccess& cFile);
        void                        SaveTexCoordControls(CTreeFileAccess& cFile) const;
        void                        ParseTexCoordControls(CTreeFileAccess& cFile);
        void                        SaveMeshes(CTreeFileAccess& cFile) const;
        void                        ParseMeshes(CTreeFileAccess& cFile);
        void                        SaveLeafMeshInfo(CTreeFileAccess& cFile) const;
        void                        ParseLeafMeshInfo(CTreeFileAccess& cFile);
        void                        SaveSupplementalGlobalInfo(CTreeFileAccess& cFile) const;
        void                        ParseSupplementalGlobalInfo(CTreeFileAccess& cFile);

        // general
        void                        SetSeed(unsigned int nSeed);

private:
        // branch
        void                        AddBranchInfo(SIdvBranchInfo* pInfo)                        { m_vBranchInfo.push_back(pInfo); }
        void                        BuildBranchLods(void);
        void                        ParseBranchInfo(CTreeFileAccess& cFile);
        void                        SaveBranchInfo(CTreeFileAccess& cFile) const;
        void                        SetBranchTexture(const st_string& strFilename)              { m_sTreeInfo.m_strBranchTextureFilename = strFilename; }

        // leaves
        void                        BuildLeafLods(float fLeafSizeIncreaseFactor);
        void                        ParseLeafCluster(CTreeFileAccess& cFile);
        void                        SaveLeafCluster(CTreeFileAccess& cFile) const;
        void                        SetLeafTextures(const st_vector_leaftexture& vTextures)     { m_sLeafInfo.m_vLeafTextures = vTextures; }

        // general
        float                       m_fBillboardSize;                   // size, based on m_cExtents, of a single billboard at lowest LOD
        float                       m_fLod;                             // current LOD value 0.0 = minimum LOD, 1.0 = maximum LOD
        float                       m_fOverrideTreeSize;                // value to use for tree size instead of loaded
        float                       m_fOverrideTreeVariance;            // value to use for tree size variance instead of loaded
        stRandom                    m_cRandom;                          // general-use random number generator
        bool                        m_bTransientDataIntact;             // flag set to true when DeleteTransientData() is invoked
        SIdvTreeInfo                m_sTreeInfo;                        // structure containing a few tree-defining variables

        // branch
        CBranch*                    m_pBranch;                          // pointer to trunk branch, the top of the branch tree
        CIndexedGeometry*           m_pBranchGeometry;                  // branch geometry container class
        st_vector_branchinfo        m_vBranchInfo;                      // SpeedTreeCAD branch information for each level of the branches
        unsigned int                m_nNumBranchLodLevels;              // number of discrete branch levels of detail

        // leaves
        st_vector_leaves            m_vBillboardLeaves;                 // vector of all leaves created for tree (highest LOD level)
        SIdvLeafInfo                m_sLeafInfo;                        // structure containing information that define the leaves
        st_vector_leaves*           m_pLeafLods;                        // an array of vectors that contain all of the discrete leaf LODs
        bool                        m_bParsedLeafLods;                  // flag indicating if the leaf LODs were read from the SPT file

        // lod
        float                       m_fMinBranchVolumePercent;          // the last branch LOD's total volume should not be below this
        float                       m_fMaxBranchVolumePercent;          // the first branch LOD's total volume should not be below this
        float                       m_fLeafReductionPercentage;         // percentage of leaves removed at each lod level
        float                       m_fBranchReductionFuzziness;        // controls how strictly branch volume is used for branch removal (0 = branch volume, 1 = random)
        float                       m_fLargeBranchPercent;              // all branches within this percent of the biggest branch will remain at highest lod

        // wind
        int                         m_nBranchesWeightLevel;             // wind effects begin at this branch level
        SIdvWindInfo                m_sWindInfo;                        // structure containing wind information
        bool                        m_bPropagateFlexibility;            // causes branches and fronds higher than the wind level to continue flexibility computations

        // floor
        bool                        m_bFloorEnabled;                    // toggles flooring
        float                       m_fFloor;                           // the height coordinate at which flooring is performed
        int                         m_nFloorLevel;                      // branches at or above this level are floored
        float                       m_fFloorExponent;                   // controls flooring influence along a branch
        float                       m_fFloorBias;                       // controls how much flooring is applied

        // cluster support
        int                         m_nFirstBranchLevel;                // first level at which branches are visible

        // root support
        SRootSupportInfo*           m_pRootSupport;                     // root specific data (branch level data + extras)

        // meshes
        st_vector_meshinfo          m_vMeshInfo;                        // mesh info for the meshes used in place of leaves

        // rotation
        float                       m_fTreeAngle;
};
