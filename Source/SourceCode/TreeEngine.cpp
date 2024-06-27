///////////////////////////////////////////////////////////////////////
//  TreeEngine.cpp
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

#include "TreeEngine.h"
#include "WindEngine.h"
#include "LeafLod.h"
#include "IndexedGeometry.h"
using namespace std;


// constants
const char* g_pszHeader = "__IdvSpt_02_";


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::CTreeEngine definition

CTreeEngine::CTreeEngine(CIndexedGeometry* pBranchGeometry) :
    m_fBillboardSize(1.0f),
    m_fLod(1.0f),
    m_fOverrideTreeSize(-1.0f),
    m_fOverrideTreeVariance(0.0f),
    m_bTransientDataIntact(true),
    m_pBranch(NULL),
    m_pBranchGeometry(NULL),
    m_nNumBranchLodLevels(6),
    m_pLeafLods(NULL),
    m_bParsedLeafLods(false),
    m_fMinBranchVolumePercent(0.5f),
    m_fMaxBranchVolumePercent(1.0f),
    m_fLeafReductionPercentage(0.3f),
    m_fBranchReductionFuzziness(0.0f),
    m_fLargeBranchPercent(0.05f),
    m_nBranchesWeightLevel(1),
    m_bPropagateFlexibility(false),
    m_bFloorEnabled(false),
    m_fFloor(0.0f),
    m_nFloorLevel(1),
    m_fFloorExponent(1.0f),
    m_fFloorBias(1.0f),
    m_nFirstBranchLevel(0),
    m_pRootSupport(NULL),
    m_fTreeAngle(0.0f)
{
    m_pBranchGeometry = pBranchGeometry;

    m_pRootSupport = new ("CTreeEngine::CTreeEngine, m_pRootSupport") SRootSupportInfo;
    SetWind(0.25f);

    if (CSpeedTreeRT::GetMemoryPreference( ) == CSpeedTreeRT::MP_FAVOR_LESS_FRAGMENTATION)
        m_vBillboardLeaves.reserve(200);
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::~CTreeEngine definition

CTreeEngine::~CTreeEngine( )
{
    m_pBranch = NULL;
    m_pBranchGeometry = NULL;
    m_pRootSupport = NULL;
    m_pLeafLods = NULL;
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::Compute definition

void CTreeEngine::Compute(float fLeafSizeIncreaseFactor)
{
    CBranch::SetComputeLeaves(!m_bParsedLeafLods);

    // allocate needed memory
    m_pBranch = new ("CTreeEngine::Compute, m_pBranch") CBranch(NULL);
    if (!m_bParsedLeafLods)
        m_pLeafLods = st_new_array<st_vector_leaves>(m_sLeafInfo.m_nNumLeafLodLevels, "CTreeEngine::Compute, m_pLeafLods");

    // setup branches for later computing
    CBranch::SetBranchLevelForWeighting(m_nBranchesWeightLevel);
    CBranch::ClearBranchInfo( );
    CBranch::SetLeafData(&m_sLeafInfo);

    unsigned int i = 0;
    for (i = 0; i < m_vBranchInfo.size( ); ++i)
        CBranch::AddBranchInfo(m_vBranchInfo[i]);

    CBranch::m_bPropagateFlexibility = m_bPropagateFlexibility;
    CBranch::m_bFloorEnabled = m_bFloorEnabled;
    CBranch::m_fFloor =m_fFloor;
    CBranch::m_nFloorLevel = m_nFloorLevel;
    CBranch::m_fFloorExponent = m_fFloorExponent;
    CBranch::m_fFloorBias = m_fFloorBias;
    CBranch::m_nFirstBranchLevel = m_nFirstBranchLevel;
    CBranch::m_pRootSupport = m_pRootSupport;

    // fill up the random bank
    CBranch::m_nRandomIndex = 0;
    CBranch::m_nFrequencyIndex = 0;
    m_cRandom.Reseed(m_sTreeInfo.m_nSeed);
    for (i = 0; i < (unsigned int) c_nBankSize; ++i)
        CBranch::m_afRandomBank[i] = m_cRandom.GetUniform(0.0f, 1.0f);

    // determine tree size using tree's seed
    m_cRandom.Reseed(m_sTreeInfo.m_nSeed);
    float fTreeSize = m_cRandom.GetUniform(m_sTreeInfo.m_fSize - m_sTreeInfo.m_fSizeVariance,
                                           m_sTreeInfo.m_fSize + m_sTreeInfo.m_fSizeVariance);
    srand(m_sTreeInfo.m_nFlareSeed);

    // adjust relative leaf sizes
    if (m_sTreeInfo.m_fSize > 0.0f)
        for (unsigned int j = 0; j < m_sLeafInfo.m_vLeafTextures.size( ); ++j)
        {
            SIdvLeafTexture& sLeaf = m_sLeafInfo.m_vLeafTextures[j];
            sLeaf.m_cSizeUsed[0] = m_sTreeInfo.m_fSize * sLeaf.m_cSize[0];
            sLeaf.m_cSizeUsed[1] = m_sTreeInfo.m_fSize * sLeaf.m_cSize[1];
        }

    // compute the branches and leaves
    m_cRandom.Reseed(m_sTreeInfo.m_nSeed);
    m_pBranchGeometry->SetNumLodLevels(static_cast<unsigned short>(GetNumBranchLodLevels( )));
    m_pBranchGeometry->ResetStripCounter( );
    m_pBranch->SetRoundRobinWindLevel(m_sTreeInfo.m_nSeed);

    stRotTransform stInitial;
    stInitial.RotateZ(m_fTreeAngle);

    (void) m_pBranch->Compute(m_sTreeInfo.m_nSeed,
                              fTreeSize,
                              0,                                // starting level, 0 = trunk
                              GetPosition( ),                   // grow tree from this position
                              0.0f,                             // percent of parent (0.0 since there's no parent for trunk)
                              stInitial,                        // identity rotation matrix
                              stVec3(0.0f, 0.0f, 1.0f),         // perturbed z-axis (starts as straight z)
                              m_pBranchGeometry,
                              m_vBillboardLeaves,
                              1.0f,                             // wind weight1
                              1.0f,                             // wind weight2
                              m_sTreeInfo.m_nSeed,              // wind1 group
                              m_sTreeInfo.m_nFlareSeed,         // wind2 group
                              -1.0f);                           // parent radius (no parent, so -1)

    // build the discrete branch levels-of-detail
    BuildBranchLods( );

    // if the leaf LODs were not already read from the file, build them here
    if (!m_bParsedLeafLods)
        BuildLeafLods(fLeafSizeIncreaseFactor);
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::Clone definition

void CTreeEngine::Clone(CTreeEngine* pClone, const stVec3& cPos, unsigned int nSeed) const
{
    // copy the branchinfo vector members
    st_vector_branchinfo( ).swap(pClone->m_vBranchInfo);
    for (unsigned int i = 0; i < m_vBranchInfo.size( ); ++i)
    {
        //lint -esym(429,pInfo)  { pInfo is not freed or returned from Clone(), but it
        //is stored in pClone->m_vBranchInfo for later deletion }
        SIdvBranchInfo* pInfo = new ("CTreeEngine::Clone, pInfo") SIdvBranchInfo;
        *pInfo = *m_vBranchInfo[i];
        pClone->m_vBranchInfo.push_back(pInfo);
    }

    // copy the rest of the important member variables
    pClone->SetPosition(cPos);
    pClone->m_nBranchesWeightLevel = m_nBranchesWeightLevel;
    pClone->m_sTreeInfo = m_sTreeInfo;
    pClone->m_sLeafInfo = m_sLeafInfo;
    pClone->m_sLeafInfo.m_pLeafTexCoords = NULL;
    pClone->m_sLeafInfo.m_pLeafVertexes = NULL;
    pClone->m_sLeafInfo.m_pTimeOffsets = NULL;
    pClone->m_sWindInfo = m_sWindInfo;
    pClone->SetBranchTexture(m_sTreeInfo.m_strBranchTextureFilename);
    pClone->SetLeafTextures(GetLeafTextures( ));
    pClone->m_fMinBranchVolumePercent = m_fMinBranchVolumePercent;
    pClone->m_fMaxBranchVolumePercent = m_fMaxBranchVolumePercent;
    pClone->m_fLod = m_fLod;

    // compute this new cloned tree based on the parameters set above
    pClone->SetSeed(nSeed);
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::OverrideTreeSize definition

void CTreeEngine::OverrideTreeSize(float fNewSize, float fNewVariance)
{
    m_fOverrideTreeSize = fNewSize;
    m_fOverrideTreeVariance = fNewVariance;
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::Parse definition

bool CTreeEngine::Parse(CTreeFileAccess& cFile)
{
    bool bSuccess = false;

    int nToken = cFile.ParseToken( );
    if (nToken != File_BeginFile)
        throw(IdvFileError("missing begin_file token"));

    st_string strHeader;
    cFile.ParseString(strHeader);
    if (strHeader != st_string(g_pszHeader))
        throw(IdvFileError("not a valid SpeedTree SPT file"));

    nToken = cFile.ParseToken( );
    do
    {
        switch (nToken)
        {
        case File_BeginTreeInfo:
            ParseTreeInfo(cFile);
            break;
        case File_BeginGeneralLeafInfo:
            m_sLeafInfo.Parse(cFile);
            break;
        case File_BeginWindInfo:
            m_sWindInfo.Parse(cFile);
            break;
        default:
            throw(IdvFileError("malformed SpeedTree SPT file"));
        }
        nToken = cFile.ParseToken( );
    } while (nToken != File_EndFile);

    if (!cFile.EndOfFile( ) && cFile.PeekToken( ) == File_BeginLeafCluster)
    {
        (void) cFile.ParseToken( );
        ParseLeafCluster(cFile);
    }

    SetBranchTexture(m_sTreeInfo.m_strBranchTextureFilename);
    SetLeafTextures(GetLeafTextures( ));

    if (m_fOverrideTreeSize > -1.0f)
        SetSize(m_fOverrideTreeSize, m_fOverrideTreeVariance);
    bSuccess = true;

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::Save definition
//
//  Saves a binary description of the tree in the unsigned char memory block it
//  returns.  If this block is saved directly to a binary file, SpeedTreeCAD will
//  read it.  The block can also be embedded in a client-defined structure or
//  memory block.

void CTreeEngine::Save(CTreeFileAccess& cFile, bool bSaveLeafLods) const
{
    cFile.SaveToken(File_BeginFile);

    // magic id header
    cFile.SaveString(g_pszHeader);

    // general tree info
    cFile.SaveToken(File_BeginTreeInfo);

    cFile.SaveToken(File_Tree_BranchTexture);
    cFile.SaveString(m_sTreeInfo.m_strBranchTextureFilename);

    cFile.SaveToken(File_Tree_Far);
    cFile.SaveFloat(m_sTreeInfo.m_fFar);

    cFile.SaveToken(File_Tree_LOD);
    cFile.SaveBool(false); // m_bLOD is no longer used

    cFile.SaveToken(File_Tree_Near);
    cFile.SaveFloat(m_sTreeInfo.m_fNear);

    cFile.SaveToken(File_Tree_RandomSeed);
    cFile.SaveInt(m_sTreeInfo.m_nSeed);

    cFile.SaveToken(File_Tree_Size);
    cFile.SaveFloat(m_sTreeInfo.m_fSize);

    cFile.SaveToken(File_Tree_RenderStyle); // no longer used
    cFile.SaveInt(0);

    cFile.SaveToken(File_Tree_Size_Variance);
    cFile.SaveFloat(m_sTreeInfo.m_fSizeVariance);

    SaveBranchInfo(cFile);

    cFile.SaveToken(File_EndTreeInfo);

    // leaf info
    m_sLeafInfo.Save(cFile);

    // wind info
    m_sWindInfo.Save(cFile);

    cFile.SaveToken(File_EndFile);

    // leaf lod data
    if (bSaveLeafLods)
        SaveLeafCluster(cFile);
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::FreeTransientData definition
//
//  This function deletes memory that will not be needed once the tree is
//  loaded and ready to render.

void CTreeEngine::FreeTransientData(void)
{
    if (TransientDataIntact( ))
    {
        // will recursively delete all branches in the tree
        st_delete<CBranch>(m_pBranch, "CTreeEngine::FreeTransientData, m_pBranch");

        // delete all of the discrete LOD leaves
        for (int i = 0; i < m_sLeafInfo.m_nNumLeafLodLevels; ++i)
            if (m_pLeafLods)
                st_vector_leaves( ).swap(m_pLeafLods[i]);
        st_delete_array<st_vector_leaves >(m_pLeafLods, "CTreeEngine::FreeTransientData, m_pLeafLods");

        // delete all associated branch info structures
        for (unsigned int j = 0; j < m_vBranchInfo.size( ); ++j)
            st_delete<SIdvBranchInfo>(m_vBranchInfo[j], "CTreeEngine::FreeTransientData, m_vBranchInfo[j]");
        st_vector_branchinfo( ).swap(m_vBranchInfo);

        // don't need roots info anymore
        st_delete<SRootSupportInfo>(m_pRootSupport, "CTreeEngine::FreeTransientData, m_pRootSupport");

        ClearMeshInfo( );

        // space reserved in the constructor
        st_vector_leaves( ).swap(m_vBillboardLeaves);

        // make sure than functions that need this data know it's gone
        m_bTransientDataIntact = false;
    }
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::SetWind definition

void CTreeEngine::SetWind(float fStrength)
{
    m_sWindInfo.m_fStrength = fStrength;

    float fLeafFrequency = c_fStrengthToFrequencyRatio * fStrength * m_sWindInfo.m_cLeafFactors[1];
    float fLeafThrow = c_fStrengthToThrowRatio * fStrength * m_sWindInfo.m_cLeafFactors[0];

    m_sWindInfo.m_cLeafOscillation.Set(-fLeafThrow, fLeafThrow, fLeafFrequency);
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::GetSize definition

void CTreeEngine::GetSize(float& fSize, float& fVariance)
{
    fSize = m_sTreeInfo.m_fSize;
    fVariance = m_sTreeInfo.m_fSizeVariance;
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::SetSize definition

void CTreeEngine::SetSize(float fSize, float fVariance)
{
    m_sTreeInfo.m_fSize = fSize;
    m_sTreeInfo.m_fSizeVariance = fVariance;
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::ComputeLod definition

float CTreeEngine::ComputeLod(void)
{
    float fDistance = m_cCameraPos.Distance(m_cPosition);

    m_fLod = 1.0f - (fDistance - m_sTreeInfo.m_fNear) / (m_sTreeInfo.m_fFar - m_sTreeInfo.m_fNear);

    if (m_fLod > 1.0f)
        m_fLod = 1.0f;
    else if (m_fLod < 0.0f)
        m_fLod = 0.0f;

    return m_fLod;
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::GetLodLimits definition

void CTreeEngine::GetLodLimits(float& fNear, float& fFar) const
{
    fNear = m_sTreeInfo.m_fNear;
    fFar = m_sTreeInfo.m_fFar;
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::SetLodLimits definition

void CTreeEngine::SetLodLimits(float fNear, float fFar)
{
    m_sTreeInfo.m_fNear = fNear;
    m_sTreeInfo.m_fFar = fFar;
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::ParseTreeInfo definition

void CTreeEngine::ParseTreeInfo(CTreeFileAccess& cFile)
{
    int nToken = cFile.ParseToken( );
    do
    {
        switch (nToken)
        {
            case File_Tree_BranchTexture:
                cFile.ParseString(m_sTreeInfo.m_strBranchTextureFilename);
                m_sTreeInfo.m_strBranchTextureFilename = m_sTreeInfo.m_strBranchTextureFilename;
                break;
            case File_Tree_RandomSeed:
                {
                    int nSeed = cFile.ParseInt( );
                    SetSeed(static_cast<unsigned int>(nSeed));
                }
                break;
            case File_Tree_Near:
                m_sTreeInfo.m_fNear = cFile.ParseFloat( );
                break;
            case File_Tree_Far:
                m_sTreeInfo.m_fFar = cFile.ParseFloat( );
                break;
            case File_Tree_RenderStyle:
                (void) cFile.ParseInt( );
                break;
            case File_Tree_LOD:
                (void) cFile.ParseBool( ); // m_bLod is no longer used
                break;
            case File_Tree_Size:
                m_sTreeInfo.m_fSize = cFile.ParseFloat( );
                break;
            case File_Tree_Size_Variance:
                m_sTreeInfo.m_fSizeVariance = cFile.ParseFloat( );
                break;
            case File_BeginBranchInfo:
                ParseBranchInfo(cFile);
                break;
            default:
                throw(IdvFileError("malformed general tree information"));
        }
        nToken = cFile.ParseToken( );
    } while (nToken != File_EndTreeInfo);
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::SetSeed definition
//
//  Seed values:
//
//       0 = pick a random seed
//       1 = use existing internal seed (from m_sTreeInfo.m_nSeed)
//      >1 = use nSeed parameter as seed value

void CTreeEngine::SetSeed(unsigned int nSeed)
{
    if (nSeed == 0)
    {
        // pick a random seed
        const int c_nMaxSeed = 1000000;

        m_cRandom.Reseed( );
        m_sTreeInfo.m_nSeed = static_cast<int>(m_cRandom.GetUniform(2.0f, static_cast<float>(c_nMaxSeed)));
    }
    else if (nSeed == 1)
    {
        // do nothing
    }
    else
    {
        // use parameter as new seed
        m_sTreeInfo.m_nSeed = nSeed;
    }
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::BuildBranchLods definition
//
//  BuildBranchLods() will start to remove branches from the tree in order
//  of their relative volume.  Smaller branches go first because they have
//  less of a visual impact.  Larger branches remain because they define
//  the bulk of the tree.

void CTreeEngine::BuildBranchLods(void)
{
    // build a linear vector of all branches on tree
    st_vector_branches vAllBranches;
    if (m_pBranch &&
        m_pBranchGeometry->GetVertexCount( ) > 0)
        m_pBranch->BuildBranchVector(vAllBranches);

    // compute total volume of all branches
    float fTotalVolume = 0.0f;
    float fLargestVolume = 0.0f;
    unsigned int i = 0; // declared outside of the for-scope for portability
    for (i = 0; i < vAllBranches.size( ); ++i)
    {
        float fVolume = vAllBranches[i]->GetVolume( );
        if (fVolume < 0.0f)
            fVolume = 0.0f;
        fTotalVolume += fVolume;

        if (fVolume > fLargestVolume)
            fLargestVolume = fVolume;
    }

    // implement fuzziness
    stRandom cRandom;
    float fSavePercent = 1.0f - m_fLargeBranchPercent;
    st_vector_branches vSavedBranches;
    for (i = 0; i < vAllBranches.size( ); ++i)
    {
        float fVolume = vAllBranches[i]->GetVolume( );

        if (fVolume > fLargestVolume * fSavePercent)
        {
            vSavedBranches.push_back(vAllBranches[i]);
            (void) vAllBranches.erase(vAllBranches.begin( ) + i--);
        }
        else
        {
            // compute a new, fuzzy volume
            float fFuzziness = cRandom.GetUniform(0.0f, m_fBranchReductionFuzziness), fOneMinusFuzziness = 1.0f - fFuzziness;
            fVolume = (fOneMinusFuzziness * fVolume) + (fFuzziness * fLargestVolume);

            // make sure we don't get a negative volume
            fVolume = st_max(fVolume, 0.0f);
            vAllBranches[i]->SetFuzzyVolume(fVolume);
        }
    }

    // sort branches in descending order of volume and add insert save branches at the beginning
    CBranch::SortBranchVector(vAllBranches);
    CBranch::SortBranchVector(vSavedBranches);
    for (i = 0; i < vSavedBranches.size( ); ++i)
        (void) vAllBranches.insert(vAllBranches.begin( ), vSavedBranches[i]);

    // for each LOD level, compute a target volume and use enough branches to reach that target
    int nNumBranchLodLevels = GetNumBranchLodLevels( );
    for (unsigned int uiLod = 0; uiLod < (unsigned int) nNumBranchLodLevels; ++uiLod)
    {
        if (uiLod == 0)
            m_pBranchGeometry->DeleteLodStrips(0);

        // figure what part of the branch structure stays for lod level i, computed by volume
        float fPercent = (nNumBranchLodLevels < 2) ? 1.0f : VecInterpolate(m_fMaxBranchVolumePercent, m_fMinBranchVolumePercent, uiLod / float(nNumBranchLodLevels - 1));
        float fTargetVolume = fPercent * fTotalVolume;

        // which branches will contribute to lod level uiLod
        int nBranchCount = 0;
        float fLodVolume = 0.0f;
        for (unsigned int j = 0; j < vAllBranches.size( ) && fLodVolume < fTargetVolume; ++j)
        {
            fLodVolume += vAllBranches[j]->GetVolume( );
            nBranchCount++;
        }

        // setup stripping container
        m_pBranchGeometry->ResetStripCounter( );

        // compute the indexes for each branch in this lod
        if (uiLod == 0 && nBranchCount == 0)
        {
            // need to zero out the original LOD strip created in CBranch::Compute()
            m_pBranchGeometry->AddStrip(0, NULL, 0);
        }
        else
            for (int k = 0; k < nBranchCount; ++k)
                vAllBranches[k]->ComputeLod(uiLod, m_pBranchGeometry);
    }
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::ParseBranchInfo definition

void CTreeEngine::ParseBranchInfo(CTreeFileAccess& cFile)
{
    ClearBranchInfo( );

    int nSize = cFile.ParseInt( );
    for (int i = 0; i < nSize; ++i)
    {
        // Parse allocates a new SIdvBranchInfo structure
        SIdvBranchInfo* pInfo = SIdvBranchInfo::Parse(cFile);
        m_vBranchInfo.push_back(pInfo);
    }

    int nToken = cFile.ParseToken( );
    if (nToken != File_EndBranchInfo)
        throw(IdvFileError("malformed branch data"));
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::SaveBranchInfo definition

void CTreeEngine::SaveBranchInfo(CTreeFileAccess& cFile) const
{
    cFile.SaveToken(File_BeginBranchInfo);

    int nSize = int(m_vBranchInfo.size( ));
    cFile.SaveInt(nSize);
    for (int i = 0; i < nSize; ++i)
    {
        cFile.SaveToken(File_BeginBranchLevel);
        m_vBranchInfo[i]->Save(cFile);
        cFile.SaveToken(File_EndBranchLevel);
    }

    cFile.SaveToken(File_EndBranchInfo);
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::BuildLeafLods definition
//
//  Wrapper invocation of CLeafLodEngine - see LeafLod.h & .cpp for details

void CTreeEngine::BuildLeafLods(float fLeafSizeIncreaseFactor)
{
    // find the largest leaf used in the tree
    float fLargestLeaf = -1.0f;
    for (unsigned int i = 0; i < m_sLeafInfo.m_vLeafTextures.size( ); ++i)
    {
        const SIdvLeafTexture& sTexture = m_sLeafInfo.m_vLeafTextures[i];
        float fLargestSide = sTexture.m_cSizeUsed[0] > sTexture.m_cSizeUsed[1] ? sTexture.m_cSizeUsed[0] : sTexture.m_cSizeUsed[1];
        if (fLargestSide * m_sLeafInfo.m_fSpacingTolerance > fLargestLeaf)
            fLargestLeaf = fLargestSide * m_sLeafInfo.m_fSpacingTolerance;
    }
    // if no leaves, use an arbitrary value
    if (fLargestLeaf == -1.0f)
        fLargestLeaf = 10.0f;

    // m_vBillboardLeaves contains all of the generated leaves, it will be the highest LOD
    if (m_pLeafLods)
    {
        m_pLeafLods[0] = m_vBillboardLeaves;
        for (int j = 1; j < m_sLeafInfo.m_nNumLeafLodLevels; ++j)
        {
            float fSizeMultiplier = 1.0f + fLeafSizeIncreaseFactor * float(j);
            CLeafLodEngine cLod(m_sLeafInfo, fLargestLeaf * 10.0f, m_fLeafReductionPercentage, fSizeMultiplier);
            cLod.ComputeNextLevel(m_pLeafLods[j - 1], m_pLeafLods[j]);
        }
    }
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::ParseLeafCluster definition
//
//  SPT files may or may not have all of the leaf LODs stored - this function
//  will retrieve the leaves if they're there.

void CTreeEngine::ParseLeafCluster(CTreeFileAccess& cFile)
{
    int nLeafLod = 0;

    int nNumLeafLodLevels = cFile.ParseInt( );

    m_sLeafInfo.m_nNumLeafLodLevels = nNumLeafLodLevels;
    st_delete_array<st_vector_leaves >(m_pLeafLods, "CTreeEngine::ParseLeafCluster, m_pLeafLods");
    m_pLeafLods = st_new_array<st_vector_leaves>(m_sLeafInfo.m_nNumLeafLodLevels, "CTreeEngine::ParseLeafCluster, m_pLeafLods");

    int nToken = cFile.ParseToken( );
    while (nToken != File_EndLeafCluster)
    {
        if (nLeafLod >= m_sLeafInfo.m_nNumLeafLodLevels)
            throw(IdvFileError("too many leaf lod levels"));

        if (nToken != File_BeginLeafLodLevel)
            throw(IdvFileError("malformed leaf lod data"));

        nToken = cFile.ParseToken( );
        if (CSpeedTreeRT::GetMemoryPreference( ) == CSpeedTreeRT::MP_FAVOR_LESS_FRAGMENTATION)
            m_pLeafLods[nLeafLod].reserve(200);
        while (nToken != File_EndLeafLodLevel)
        {
            if (nToken != File_BeginBillboardLeaf)
                throw(IdvFileError("malformed leaf lod data"));

            CBillboardLeaf cLeaf;
            CBillboardLeaf::Parse(cFile, cLeaf);
            m_pLeafLods[nLeafLod].push_back(cLeaf);

            nToken = cFile.ParseToken( );
        }
        ++nLeafLod;

        nToken = cFile.ParseToken( );
    }
    m_bParsedLeafLods = true;
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::SaveLeafCluster
//
//  SPT files may or may not have all of the leaf LODs stored - this function
//  will save the leaves LODs if invoked.

void CTreeEngine::SaveLeafCluster(CTreeFileAccess& cFile) const
{
    if (m_pLeafLods)
    {
        cFile.SaveToken(File_BeginLeafCluster);
        cFile.SaveInt(m_sLeafInfo.m_nNumLeafLodLevels);
        for (int i = 0; i < m_sLeafInfo.m_nNumLeafLodLevels; ++i)
        {
            cFile.SaveToken(File_BeginLeafLodLevel);
            for (unsigned int j = 0; j < m_pLeafLods[i].size( ); ++j)
                m_pLeafLods[i][j].Save(cFile);
            cFile.SaveToken(File_EndLeafLodLevel);
        }
        cFile.SaveToken(File_EndLeafCluster);
    }
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::SaveLodInfo
//
//  This data must be saved separately at the end of the file to
//  maintain backward compatibility.

void CTreeEngine::SaveLodInfo(CTreeFileAccess& cFile) const
{
    cFile.SaveToken(File_BeginEngineLodInfo);

    cFile.SaveToken(File_BranchNumLods);
    cFile.SaveInt(m_nNumBranchLodLevels);

    cFile.SaveToken(File_BranchMinVolumePercent);
    cFile.SaveFloat(m_fMinBranchVolumePercent);

    cFile.SaveToken(File_LeafReductionFactor);
    cFile.SaveFloat(m_fLeafReductionPercentage);

    cFile.SaveToken(File_LeafNumLods);
    cFile.SaveInt(m_sLeafInfo.m_nNumLeafLodLevels);

    cFile.SaveToken(File_BranchMaxVolumePercent);
    cFile.SaveFloat(m_fMaxBranchVolumePercent);

    cFile.SaveToken(File_BranchReductionFuzziness);
    cFile.SaveFloat(m_fBranchReductionFuzziness);

    cFile.SaveToken(File_LargeBranchPercent);
    cFile.SaveFloat(m_fLargeBranchPercent);

    cFile.SaveToken(File_EndEngineLodInfo);
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::ParseLodInfo
//
//  This data must be saved separately at the end of the file to
//  maintain backward compatibility.

void CTreeEngine::ParseLodInfo(CTreeFileAccess& cFile)
{
    int nToken = cFile.ParseToken( );
    do
    {
        switch (nToken)
        {
        case File_BranchNumLods:
            m_nNumBranchLodLevels = cFile.ParseInt( );
            break;
        case File_BranchMinVolumePercent:
            m_fMinBranchVolumePercent = cFile.ParseFloat( );
            break;
        case File_LeafReductionFactor:
            m_fLeafReductionPercentage = cFile.ParseFloat( );
            break;
        case File_LeafNumLods:
            m_sLeafInfo.m_nNumLeafLodLevels = cFile.ParseInt( );
            break;
        case File_BranchMaxVolumePercent:
            m_fMaxBranchVolumePercent = cFile.ParseFloat( );
            break;
        case File_BranchReductionFuzziness:
            m_fBranchReductionFuzziness = cFile.ParseFloat( );
            break;
        case File_LargeBranchPercent:
            m_fLargeBranchPercent = cFile.ParseFloat( );
            break;
        default:
            throw(IdvFileError("malformed engine lod data"));
        }
        nToken = cFile.ParseToken( );
    } while (nToken != File_EndEngineLodInfo);
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::SetBranchTextureFilename

void CTreeEngine::SetBranchTextureFilename(const char* pFilename)
{
    m_sTreeInfo.m_strBranchTextureFilename = pFilename;
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::SetLeafTextureFilename

void CTreeEngine::SetLeafTextureFilename(unsigned int nLeafMapIndex, const char* pFilename)
{
    st_assert(nLeafMapIndex < m_sLeafInfo.m_vLeafTextures.size( ));
    m_sLeafInfo.m_vLeafTextures[nLeafMapIndex].m_strFilename = pFilename;
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::SaveTextureControls
//
//  This data must be saved separately at the end of the file to
//  maintain backward compatibility.

void CTreeEngine::SaveTextureControls(CTreeFileAccess& cFile) const
{
    cFile.SaveToken(File_BeginTextureControls);

    for (unsigned int i = 0; i < m_vBranchInfo.size( ); ++i)
    {
        cFile.SaveToken(File_TextureOffset);
        cFile.SaveBool(m_vBranchInfo[i]->m_asTexCoordControls[CSpeedTreeRT::TL_DIFFUSE].m_bRandomTCoordOffset);

        cFile.SaveToken(File_TextureTwist);
        cFile.SaveFloat(m_vBranchInfo[i]->m_asTexCoordControls[CSpeedTreeRT::TL_DIFFUSE].m_fTwist);
    }

    cFile.SaveToken(File_EndTextureControls);
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::ParseTextureControls
//
//  This data must be saved separately at the end of the file to
//  maintain backward compatibility.

void CTreeEngine::ParseTextureControls(CTreeFileAccess& cFile)
{
    for (unsigned int i = 0; i < m_vBranchInfo.size( ); ++i)
    {
        if (cFile.ParseToken( ) != File_TextureOffset)
            throw(IdvFileError("malformed texture controls"));
        m_vBranchInfo[i]->m_asTexCoordControls[CSpeedTreeRT::TL_DIFFUSE].m_bRandomTCoordOffset = cFile.ParseBool( );

        if (cFile.ParseToken( ) != File_TextureTwist)
            throw(IdvFileError("malformed texture controls"));
        m_vBranchInfo[i]->m_asTexCoordControls[CSpeedTreeRT::TL_DIFFUSE].m_fTwist = cFile.ParseFloat( );
    }

    if (cFile.ParseToken( ) != File_EndTextureControls)
        throw(IdvFileError("malformed texture controls"));
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::SaveFlareInfo
//
//  This data must be saved separately at the end of the file to
//  maintain backward compatibility.

void CTreeEngine::SaveFlareInfo(CTreeFileAccess& cFile) const
{
    cFile.SaveToken(File_BeginFlareInfo);

    for (unsigned int i = 0; i < m_vBranchInfo.size( ); ++i)
    {
        cFile.SaveToken(File_FlareSegmentPackingExponent);
        cFile.SaveFloat(m_vBranchInfo[i]->m_fSegmentPackingExponent);

        cFile.SaveToken(File_FlareNumFlares);
        cFile.SaveInt(m_vBranchInfo[i]->m_nNumFlares);

        cFile.SaveToken(File_FlareBalance);
        cFile.SaveFloat(m_vBranchInfo[i]->m_fFlareBalance);

        cFile.SaveToken(File_FlareRadialInfluence);
        cFile.SaveFloat(m_vBranchInfo[i]->m_fRadialInfluence);

        cFile.SaveToken(File_FlareRadialInfluenceVariance);
        cFile.SaveFloat(m_vBranchInfo[i]->m_fRadialInfluenceVariance);

        cFile.SaveToken(File_FlareRadialExponent);
        cFile.SaveFloat(m_vBranchInfo[i]->m_fRadialExponent);

        cFile.SaveToken(File_FlareRadialDistance);
        cFile.SaveFloat(m_vBranchInfo[i]->m_fRadialDistance);

        cFile.SaveToken(File_FlareRadialDistanceVariance);
        cFile.SaveFloat(m_vBranchInfo[i]->m_fRadialVariance);

        cFile.SaveToken(File_FlareLengthDistance);
        cFile.SaveFloat(m_vBranchInfo[i]->m_fLengthDistance);

        cFile.SaveToken(File_FlareLengthDistanceVariance);
        cFile.SaveFloat(m_vBranchInfo[i]->m_fLengthVariance);

        cFile.SaveToken(File_LengthExponent);
        cFile.SaveFloat(m_vBranchInfo[i]->m_fLengthExponent);
    }

    cFile.SaveToken(File_EndFlareInfo);
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::ParseFlareInfo
//
//  This data must be saved separately at the end of the file to
//  maintain backward compatibility.

void CTreeEngine::ParseFlareInfo(CTreeFileAccess& cFile)
{
    for (unsigned int i = 0; i < m_vBranchInfo.size( ); ++i)
    {
        if (cFile.ParseToken( ) != File_FlareSegmentPackingExponent)
            throw(IdvFileError("malformed flare info"));
        m_vBranchInfo[i]->m_fSegmentPackingExponent = cFile.ParseFloat( );

        if (cFile.ParseToken( ) != File_FlareNumFlares)
            throw(IdvFileError("malformed flare info"));
        m_vBranchInfo[i]->m_nNumFlares = cFile.ParseInt( );

        if (cFile.ParseToken( ) != File_FlareBalance)
            throw(IdvFileError("malformed flare info"));
        m_vBranchInfo[i]->m_fFlareBalance = cFile.ParseFloat( );

        if (cFile.ParseToken( ) != File_FlareRadialInfluence)
            throw(IdvFileError("malformed flare info"));
        m_vBranchInfo[i]->m_fRadialInfluence = cFile.ParseFloat( );

        if (cFile.ParseToken( ) != File_FlareRadialInfluenceVariance)
            throw(IdvFileError("malformed flare info"));
        m_vBranchInfo[i]->m_fRadialInfluenceVariance = cFile.ParseFloat( );

        if (cFile.ParseToken( ) != File_FlareRadialExponent)
            throw(IdvFileError("malformed flare info"));
        m_vBranchInfo[i]->m_fRadialExponent = cFile.ParseFloat( );

        if (cFile.ParseToken( ) != File_FlareRadialDistance)
            throw(IdvFileError("malformed flare info"));
        m_vBranchInfo[i]->m_fRadialDistance = cFile.ParseFloat( );

        if (cFile.ParseToken( ) != File_FlareRadialDistanceVariance)
            throw(IdvFileError("malformed flare info"));
        m_vBranchInfo[i]->m_fRadialVariance = cFile.ParseFloat( );

        if (cFile.ParseToken( ) != File_FlareLengthDistance)
            throw(IdvFileError("malformed flare info"));
        m_vBranchInfo[i]->m_fLengthDistance = cFile.ParseFloat( );

        if (cFile.ParseToken( ) != File_FlareLengthDistanceVariance)
            throw(IdvFileError("malformed flare info"));
        m_vBranchInfo[i]->m_fLengthVariance = cFile.ParseFloat( );

        if (cFile.ParseToken( ) != File_LengthExponent)
            throw(IdvFileError("malformed flare info"));
        m_vBranchInfo[i]->m_fLengthExponent = cFile.ParseFloat( );
    }

    if (cFile.ParseToken( ) != File_EndFlareInfo)
        throw(IdvFileError("malformed flare info"));
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::SaveFlareSeed
//
//  This data must be saved separately at the end of the file to
//  maintain backward compatibility.

void CTreeEngine::SaveFlareSeed(CTreeFileAccess& cFile) const
{
    cFile.SaveToken(File_FlareSeed);
    cFile.SaveInt(m_sTreeInfo.m_nFlareSeed);
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::ParseFlareSeed
//
//  This data must be saved separately at the end of the file to
//  maintain backward compatibility.

void CTreeEngine::ParseFlareSeed(CTreeFileAccess& cFile)
{
    m_sTreeInfo.m_nFlareSeed = cFile.ParseInt( );
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::SaveLightSeamReductionInfo
//
//  This data must be saved separately at the end of the file to
//  maintain backward compatibility.

void CTreeEngine::SaveLightSeamReductionInfo(CTreeFileAccess& cFile) const
{
    cFile.SaveToken(File_BeginLightSeamReductionInfo);

    for (unsigned int i = 0; i < m_vBranchInfo.size( ); ++i)
    {
        cFile.SaveToken(File_LightSeamStartBias);
        cFile.SaveFloat(m_vBranchInfo[i]->m_fLightSeamStartBias);

        cFile.SaveToken(File_LightSeamEndBias);
        cFile.SaveFloat(m_vBranchInfo[i]->m_fLightSeamEndBias);
    }

    cFile.SaveToken(File_EndLightSeamReductionInfo);
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::ParseLightSeamReductionInfo
//
//  This data must be saved separately at the end of the file to
//  maintain backward compatibility.

void CTreeEngine::ParseLightSeamReductionInfo(CTreeFileAccess& cFile)
{
    for (unsigned int i = 0; i < m_vBranchInfo.size( ); ++i)
    {
        if (cFile.ParseToken( ) != File_LightSeamStartBias)
            throw(IdvFileError("malformed light seam reduction info"));
        m_vBranchInfo[i]->m_fLightSeamStartBias = cFile.ParseFloat( );

        if (cFile.ParseToken( ) != File_LightSeamEndBias)
            throw(IdvFileError("malformed light seam reduction info"));
        m_vBranchInfo[i]->m_fLightSeamEndBias = cFile.ParseFloat( );
    }

    if (cFile.ParseToken( ) != File_EndLightSeamReductionInfo)
        throw(IdvFileError("malformed light seam reduction info"));
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::SaveSupplementalLeafPlacementInfo
//
//  This data must be saved separately at the end of the file to
//  maintain backward compatibility.

void CTreeEngine::SaveSupplementalLeafPlacementInfo(CTreeFileAccess& cFile) const
{
    cFile.SaveToken(File_BeginSupplementalLeafPlacementInfo);

    cFile.SaveToken(File_LeafPlacementDistance);
    cFile.SaveFloat(m_sLeafInfo.m_fPlacementDistance);

    cFile.SaveToken(File_LeafPlacementLevel);
    cFile.SaveInt(m_sLeafInfo.m_nPlacementLevel);

    cFile.SaveToken(File_EndSupplementalLeafPlacementInfo);
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::ParseSupplementalLeafPlacementInfo
//
//  This data must be saved separately at the end of the file to
//  maintain backward compatibility.

void CTreeEngine::ParseSupplementalLeafPlacementInfo(CTreeFileAccess& cFile)
{
    if (cFile.ParseToken( ) != File_LeafPlacementDistance)
        throw(IdvFileError("malformed supplemental leaf placement info"));
    m_sLeafInfo.m_fPlacementDistance = cFile.ParseFloat( );

    if (cFile.ParseToken( ) != File_LeafPlacementLevel)
        throw(IdvFileError("malformed supplemental leaf placement info"));
    m_sLeafInfo.m_nPlacementLevel = cFile.ParseInt( );

    if (cFile.ParseToken( ) != File_EndSupplementalLeafPlacementInfo)
        throw(IdvFileError("malformed supplemental leaf placement info"));
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::SaveSupplementalBranchInfo
//
//  This data must be saved separately at the end of the file to
//  maintain backward compatibility.

void CTreeEngine::SaveSupplementalBranchInfo(CTreeFileAccess& cFile) const
{
    cFile.SaveToken(File_BeginSupplementalBranchInfo);

    for (unsigned int i = 0; i < m_vBranchInfo.size( ); ++i)
    {
        cFile.SaveToken(File_BeginBranch);

        cFile.SaveToken(File_BranchRoughness);
        cFile.SaveFloat(m_vBranchInfo[i]->m_fRoughness);

        cFile.SaveToken(File_BranchRoughnessVerticalFrequency);
        cFile.SaveFloat(m_vBranchInfo[i]->m_fRoughnessVerticalFrequency);

        cFile.SaveToken(File_BranchRoughnessHorizontalFrequency);
        cFile.SaveFloat(m_vBranchInfo[i]->m_fRoughnessHorizontalFrequency);

        cFile.SaveToken(File_BranchRandomRoughness);
        cFile.SaveFloat(m_vBranchInfo[i]->m_fRandomRoughness);

        cFile.SaveToken(File_BranchMinLengthPercent);
        cFile.SaveFloat(m_vBranchInfo[i]->m_fMinLengthPercent);

        cFile.SaveToken(File_BranchMinCrossSectionPercent);
        cFile.SaveFloat(m_vBranchInfo[i]->m_fMinCrossSectionPercent);

        cFile.SaveToken(File_BranchPruningPercent);
        cFile.SaveFloat(m_vBranchInfo[i]->m_fPruningPercent);

        cFile.SaveToken(File_BranchPruningDepth);
        cFile.SaveInt(m_vBranchInfo[i]->m_uiPruningDepth);

        cFile.SaveToken(File_ForkEnabled);
        cFile.SaveBool(m_vBranchInfo[i]->m_bForkingEnabled);

        cFile.SaveToken(File_ForkBias);
        cFile.SaveFloat(m_vBranchInfo[i]->m_fForkBias);

        cFile.SaveToken(File_ForkAngle);
        cFile.SaveFloat(m_vBranchInfo[i]->m_fForkAngle);

        cFile.SaveToken(File_ForkLimit);
        cFile.SaveInt(m_vBranchInfo[i]->m_nForkLimit);

        cFile.SaveToken(File_CrossSectionProfile);
        cFile.SaveBranchParameter(m_vBranchInfo[i]->GetCrossSectionProfile( ));

        cFile.SaveToken(File_LightSeamProfile);
        cFile.SaveBranchParameter(m_vBranchInfo[i]->GetLightSeamProfile( ));

        cFile.SaveToken(File_RoughnessProfile);
        cFile.SaveBranchParameter(m_vBranchInfo[i]->GetRoughnessProfile( ));

        cFile.SaveToken(File_FrequencyProfile);
        cFile.SaveBranchParameter(m_vBranchInfo[i]->GetFrequencyProfile( ));

        cFile.SaveToken(File_LightSeamBias);
        cFile.SaveBranchParameter(m_vBranchInfo[i]->GetLightSeamBias( ));

        cFile.SaveToken(File_GnarlProfile);
        cFile.SaveBranchParameter(m_vBranchInfo[i]->GetGnarlProfile( ));

        cFile.SaveToken(File_Gnarl);
        cFile.SaveFloat(m_vBranchInfo[i]->m_fGnarl);

        cFile.SaveToken(File_GnarlUnison);
        cFile.SaveBool(m_vBranchInfo[i]->m_bGnarlUnison);

        cFile.SaveToken(File_EndBranch);
    }

    cFile.SaveToken(File_EndSupplementalBranchInfo);
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::ParseSupplementalBranchInfo
//
//  This data must be saved separately at the end of the file to
//  maintain backward compatibility.

void CTreeEngine::ParseSupplementalBranchInfo(CTreeFileAccess& cFile)
{
    for (unsigned int i = 0; i < m_vBranchInfo.size( ); ++i)
    {
        if (cFile.ParseToken( ) != File_BeginBranch)
            throw(IdvFileError("malformed supplemental branch info"));

        int nToken;
        do
        {
            nToken = cFile.ParseToken( );
            switch (nToken)
            {
            case File_BranchRoughness:
                m_vBranchInfo[i]->m_fRoughness = cFile.ParseFloat( );
                break;
            case File_BranchRoughnessVerticalFrequency:
                m_vBranchInfo[i]->m_fRoughnessVerticalFrequency = cFile.ParseFloat( );
                break;
            case File_BranchRoughnessHorizontalFrequency:
                m_vBranchInfo[i]->m_fRoughnessHorizontalFrequency = cFile.ParseFloat( );
                break;
            case File_BranchRandomRoughness:
                m_vBranchInfo[i]->m_fRandomRoughness = cFile.ParseFloat( );
                break;
            case File_BranchMinLengthPercent:
                m_vBranchInfo[i]->m_fMinLengthPercent = cFile.ParseFloat( );
                break;
            case File_BranchMinCrossSectionPercent:
                m_vBranchInfo[i]->m_fMinCrossSectionPercent = cFile.ParseFloat( );
                break;
            case File_BranchPruningPercent:
                m_vBranchInfo[i]->m_fPruningPercent = cFile.ParseFloat( );
                break;
            case File_BranchPruningDepth:
                m_vBranchInfo[i]->m_uiPruningDepth = static_cast<unsigned int>(cFile.ParseInt( ));
                break;
            case File_ForkEnabled:
                m_vBranchInfo[i]->m_bForkingEnabled = cFile.ParseBool( );
                break;
            case File_ForkBias:
                m_vBranchInfo[i]->m_fForkBias = cFile.ParseFloat( );
                break;
            case File_ForkAngle:
                m_vBranchInfo[i]->m_fForkAngle = cFile.ParseFloat( );
                break;
            case File_ForkLimit:
                m_vBranchInfo[i]->m_nForkLimit = cFile.ParseInt( );
                break;
            case File_CrossSectionProfile:
                cFile.ParseBranchParameter(m_vBranchInfo[i]->GetCrossSectionProfile( ));
                break;
            case File_LightSeamProfile:
                cFile.ParseBranchParameter(m_vBranchInfo[i]->GetLightSeamProfile( ));
                break;
            case File_RoughnessProfile:
                cFile.ParseBranchParameter(m_vBranchInfo[i]->GetRoughnessProfile( ));
                break;
            case File_FrequencyProfile:
                cFile.ParseBranchParameter(m_vBranchInfo[i]->GetFrequencyProfile( ));
                break;
            case File_LightSeamBias:
                cFile.ParseBranchParameter(m_vBranchInfo[i]->GetLightSeamBias( ));
                break;
            case File_GnarlProfile:
                cFile.ParseBranchParameter(m_vBranchInfo[i]->GetGnarlProfile( ));
                break;
            case File_Gnarl:
                m_vBranchInfo[i]->m_fGnarl = cFile.ParseFloat( );
                break;
            case File_GnarlUnison:
                m_vBranchInfo[i]->m_bGnarlUnison = cFile.ParseBool( );
                break;
            case File_EndBranch:
                break;
            default:
               throw(IdvFileError("malformed supplemental branch info"));
            }
        }
        while (nToken != File_EndBranch);
    }

    if (cFile.ParseToken( ) != File_EndSupplementalBranchInfo)
        throw(IdvFileError("malformed supplemental branch info"));
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::SaveFloorInfo
//
//  This data must be saved separately at the end of the file to
//  maintain backward compatibility.

void CTreeEngine::SaveFloorInfo(CTreeFileAccess& cFile) const
{
    cFile.SaveToken(File_BeginFloorInfo);

    cFile.SaveToken(File_FloorEnabled);
    cFile.SaveBool(m_bFloorEnabled);

    cFile.SaveToken(File_FloorValue);
    cFile.SaveFloat(m_fFloor);

    cFile.SaveToken(File_FloorLevel);
    cFile.SaveInt(m_nFloorLevel);

    cFile.SaveToken(File_FloorExponent);
    cFile.SaveFloat(m_fFloorExponent);

    cFile.SaveToken(File_FloorBias);
    cFile.SaveFloat(m_fFloorBias);

    cFile.SaveToken(File_EndFloorInfo);
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::ParseFloorInfo
//
//  This data must be saved separately at the end of the file to
//  maintain backward compatibility.

void CTreeEngine::ParseFloorInfo(CTreeFileAccess& cFile)
{
    int nToken;
    do
    {
        nToken = cFile.ParseToken( );
        switch (nToken)
        {
        case File_FloorEnabled:
            m_bFloorEnabled = cFile.ParseBool( );
            break;
        case File_FloorValue:
            m_fFloor = cFile.ParseFloat( );
            break;
        case File_FloorLevel:
            m_nFloorLevel = cFile.ParseInt( );
            break;
        case File_FloorExponent:
            m_fFloorExponent = cFile.ParseFloat( );
            break;
        case File_FloorBias:
            m_fFloorBias = cFile.ParseFloat( );
            break;
        case File_EndFloorInfo:
            break;
        default:
            throw(IdvFileError("malformed floor info"));
        }
    }
    while (nToken != File_EndFloorInfo);
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::SaveLeafNormalSmoothing
//
//  This data must be saved separately at the end of the file to
//  maintain backward compatibility.

void CTreeEngine::SaveLeafNormalSmoothing(CTreeFileAccess& cFile) const
{
    cFile.SaveToken(File_BeginLeafNormalSmoothing);

    cFile.SaveToken(File_LeafSmoothingEnabled);
    cFile.SaveBool(m_sLeafInfo.m_bSmoothingEnabled);

    cFile.SaveToken(File_LeafSmoothingAngle);
    cFile.SaveFloat(m_sLeafInfo.m_fSmoothingAngle);

    cFile.SaveToken(File_LeafDimmingDepth);
    cFile.SaveInt(m_sLeafInfo.m_nDimmingDepth);

    cFile.SaveToken(File_EndLeafNormalSmoothing);
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::ParseLeafNormalSmoothing
//
//  This data must be saved separately at the end of the file to
//  maintain backward compatibility.

void CTreeEngine::ParseLeafNormalSmoothing(CTreeFileAccess& cFile)
{
    int nToken;
    do
    {
        nToken = cFile.ParseToken( );
        switch (nToken)
        {
        case File_LeafSmoothingEnabled:
            m_sLeafInfo.m_bSmoothingEnabled = cFile.ParseBool( );
            break;
        case File_LeafSmoothingAngle:
            m_sLeafInfo.m_fSmoothingAngle = cFile.ParseFloat( );
            break;
        case File_LeafDimmingDepth:
            m_sLeafInfo.m_nDimmingDepth = cFile.ParseInt( );
            break;
        case File_EndLeafNormalSmoothing:
            break;
        default:
           throw(IdvFileError("malformed leaf normal smoothing info"));
        }
    }
    while (nToken != File_EndLeafNormalSmoothing);
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::SaveClusterInfo
//
//  This data must be saved separately at the end of the file to
//  maintain backward compatibility.

void CTreeEngine::SaveClusterInfo(CTreeFileAccess& cFile) const
{
    cFile.SaveToken(File_BeginClusterInfo);

    cFile.SaveToken(File_FirstBranchLevel);
    cFile.SaveInt(m_nFirstBranchLevel);

    cFile.SaveToken(File_EndClusterInfo);
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::ParseClusterInfo
//
//  This data must be saved separately at the end of the file to
//  maintain backward compatibility.

void CTreeEngine::ParseClusterInfo(CTreeFileAccess& cFile)
{
    int nToken;
    do
    {
        nToken = cFile.ParseToken( );
        switch (nToken)
        {
        case File_FirstBranchLevel:
            m_nFirstBranchLevel = cFile.ParseInt( );
            break;
        case File_EndClusterInfo:
            break;
        default:
            throw(IdvFileError("malformed cluster info"));
        }
    }
    while (nToken != File_EndClusterInfo);
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::SaveRootSupportInfo
//
//  This data must be saved separately at the end of the file to
//  maintain backward compatibility.

void CTreeEngine::SaveRootSupportInfo(CTreeFileAccess& cFile) const
{
    st_assert(m_pRootSupport);

    m_pRootSupport->Save(cFile);
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::ParseRootSupportInfo
//
//  This data must be saved separately at the end of the file to
//  maintain backward compatibility.

void CTreeEngine::ParseRootSupportInfo(CTreeFileAccess& cFile)
{
    st_assert(m_pRootSupport);

    m_pRootSupport->Parse(cFile);
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::SaveTexCoordControls
//
//  This data must be saved separately at the end of the file to
//  maintain backward compatibility.

void CTreeEngine::SaveTexCoordControls(CTreeFileAccess& cFile) const
{
    cFile.SaveToken(File_BeginTexCoordControls);

    for (unsigned int i = 0; i <= m_vBranchInfo.size( ); ++i)
    {
        const SIdvBranchInfo* pInfo = NULL;
        if (i < m_vBranchInfo.size( ))
            pInfo = m_vBranchInfo[i];
        else
            pInfo = &(m_pRootSupport->m_sRoots);

        for (unsigned int j = 0; j < CSpeedTreeRT::TL_SHADOW; ++j)
        {
            cFile.SaveToken(File_BeginControl);

            cFile.SaveToken(File_Tex_STile);
            cFile.SaveFloat(pInfo->m_asTexCoordControls[j].m_fSTile);

            cFile.SaveToken(File_Tex_TTile);
            cFile.SaveFloat(pInfo->m_asTexCoordControls[j].m_fTTile);

            cFile.SaveToken(File_Tex_STileAbsolute);
            cFile.SaveBool(pInfo->m_asTexCoordControls[j].m_bSTileAbsolute);

            cFile.SaveToken(File_Tex_TTileAbsolute);
            cFile.SaveBool(pInfo->m_asTexCoordControls[j].m_bTTileAbsolute);

            cFile.SaveToken(File_Tex_RandomTOffset);
            cFile.SaveBool(pInfo->m_asTexCoordControls[j].m_bRandomTCoordOffset);

            cFile.SaveToken(File_Tex_Twist);
            cFile.SaveFloat(pInfo->m_asTexCoordControls[j].m_fTwist);

            cFile.SaveToken(File_Tex_TOffset);
            cFile.SaveFloat(pInfo->m_asTexCoordControls[j].m_fTCoordOffset);

            cFile.SaveToken(File_Tex_SOffset);
            cFile.SaveFloat(pInfo->m_asTexCoordControls[j].m_fSCoordOffset);

            cFile.SaveToken(File_Tex_ClampS);
            cFile.SaveBool(pInfo->m_asTexCoordControls[j].m_bClampSCoord);

            cFile.SaveToken(File_Tex_ClampT);
            cFile.SaveBool(pInfo->m_asTexCoordControls[j].m_bClampTCoord);

            cFile.SaveToken(File_Tex_ClampLeft);
            cFile.SaveFloat(pInfo->m_asTexCoordControls[j].m_afClampValues[SIdvBranchInfo::STexCoordControls::LEFT]);

            cFile.SaveToken(File_Tex_ClampRight);
            cFile.SaveFloat(pInfo->m_asTexCoordControls[j].m_afClampValues[SIdvBranchInfo::STexCoordControls::RIGHT]);

            cFile.SaveToken(File_Tex_ClampBottom);
            cFile.SaveFloat(pInfo->m_asTexCoordControls[j].m_afClampValues[SIdvBranchInfo::STexCoordControls::BOTTOM]);

            cFile.SaveToken(File_Tex_ClampTop);
            cFile.SaveFloat(pInfo->m_asTexCoordControls[j].m_afClampValues[SIdvBranchInfo::STexCoordControls::TOP]);

            cFile.SaveToken(File_Tex_Synch);
            cFile.SaveBool(pInfo->m_asTexCoordControls[j].m_bSynch);

            cFile.SaveToken(File_EndControl);
        }
    }

    cFile.SaveToken(File_EndTexCoordControls);
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::ParseTexCoordControls
//
//  This data must be saved separately at the end of the file to
//  maintain backward compatibility.

void CTreeEngine::ParseTexCoordControls(CTreeFileAccess& cFile)
{
    for (unsigned int i = 0; i <= m_vBranchInfo.size( ); ++i)
    {
        SIdvBranchInfo* pInfo = NULL;
        if (i < m_vBranchInfo.size( ))
            pInfo = m_vBranchInfo[i];
        else
            pInfo = &m_pRootSupport->m_sRoots;

        for (unsigned int j = 0; j < CSpeedTreeRT::TL_SHADOW; ++j)
        {
            if (cFile.ParseToken( ) != File_BeginControl)
                throw(IdvFileError("malformed tex coord controls"));

            int nToken = 0;
            do
            {
                nToken = cFile.ParseToken( );

                switch (nToken)
                {
                case File_Tex_STile:
                    pInfo->m_asTexCoordControls[j].m_fSTile = cFile.ParseFloat( );
                    break;
                case File_Tex_TTile:
                    pInfo->m_asTexCoordControls[j].m_fTTile = cFile.ParseFloat( );
                    break;
                case File_Tex_STileAbsolute:
                    pInfo->m_asTexCoordControls[j].m_bSTileAbsolute = cFile.ParseBool( );
                    break;
                case File_Tex_TTileAbsolute:
                    pInfo->m_asTexCoordControls[j].m_bTTileAbsolute = cFile.ParseBool( );
                    break;
                case File_Tex_RandomTOffset:
                    pInfo->m_asTexCoordControls[j].m_bRandomTCoordOffset = cFile.ParseBool( );
                    break;
                case File_Tex_Twist:
                    pInfo->m_asTexCoordControls[j].m_fTwist = cFile.ParseFloat( );
                    break;
                case File_Tex_TOffset:
                    pInfo->m_asTexCoordControls[j].m_fTCoordOffset = cFile.ParseFloat( );
                    break;
                case File_Tex_SOffset:
                    pInfo->m_asTexCoordControls[j].m_fSCoordOffset = cFile.ParseFloat( );
                    break;
                case File_Tex_ClampS:
                    pInfo->m_asTexCoordControls[j].m_bClampSCoord = cFile.ParseBool( );
                    break;
                case File_Tex_ClampT:
                    pInfo->m_asTexCoordControls[j].m_bClampTCoord = cFile.ParseBool( );
                    break;
                case File_Tex_ClampLeft:
                    pInfo->m_asTexCoordControls[j].m_afClampValues[SIdvBranchInfo::STexCoordControls::LEFT] = cFile.ParseFloat( );
                    break;
                case File_Tex_ClampRight:
                    pInfo->m_asTexCoordControls[j].m_afClampValues[SIdvBranchInfo::STexCoordControls::RIGHT] = cFile.ParseFloat( );
                    break;
                case File_Tex_ClampBottom:
                    pInfo->m_asTexCoordControls[j].m_afClampValues[SIdvBranchInfo::STexCoordControls::BOTTOM] = cFile.ParseFloat( );
                    break;
                case File_Tex_ClampTop:
                    pInfo->m_asTexCoordControls[j].m_afClampValues[SIdvBranchInfo::STexCoordControls::TOP] = cFile.ParseFloat( );
                    break;
                case File_EndControl:
                    break;
                case File_Tex_Synch:
                    pInfo->m_asTexCoordControls[j].m_bSynch = cFile.ParseBool( );
                    break;
                default:
                   throw(IdvFileError("malformed tex coord controls"));
                }
            }
            while (nToken != File_EndControl);
        }
    }

    if (cFile.ParseToken( ) != File_EndTexCoordControls)
        throw(IdvFileError("malformed tex coord controls"));
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::SynchTexCoords

void CTreeEngine::SynchTexCoords(void)
{
    for (unsigned int i = 0; i < m_vBranchInfo.size( ); ++i)
    {
        int nSynchLayer = 0;
        for (int j = int(CSpeedTreeRT::TL_DETAIL); j < int(CSpeedTreeRT::TL_SHADOW); ++j)
        {
            if (m_vBranchInfo[i]->m_asTexCoordControls[j].m_bSynch)
                m_vBranchInfo[i]->m_asTexCoordControls[j] = m_vBranchInfo[i]->m_asTexCoordControls[nSynchLayer];
        }
    }
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::AddMeshInfo

void CTreeEngine::AddMeshInfo(SIdvMeshInfo* pMeshInfo)
{
    m_vMeshInfo.push_back(pMeshInfo);
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::ClearMeshInfo

void CTreeEngine::ClearMeshInfo(void)
{
    for (unsigned int i = 0; i < m_vMeshInfo.size( ); ++i)
        st_delete<SIdvMeshInfo>(m_vMeshInfo[i], "CTreeEngine::ClearMeshInfo");
    st_vector_meshinfo( ).swap(m_vMeshInfo);
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::SaveMeshes

void CTreeEngine::SaveMeshes(CTreeFileAccess& cFile) const
{
    cFile.SaveToken(File_BeginMeshes);

    unsigned int uiNumMeshes = (unsigned int)m_vMeshInfo.size( );
    cFile.SaveToken(File_NumMeshes);
    cFile.SaveInt(uiNumMeshes);
    for (unsigned int j = 0; j < uiNumMeshes; ++j)
    {
        SIdvMeshInfo* pMesh = m_vMeshInfo[j];

        cFile.SaveToken(File_BeginMesh);

        cFile.SaveToken(File_Mesh_Name);
        cFile.SaveString(pMesh->m_strName);

        cFile.SaveToken(File_Mesh_VertexCount);
        cFile.SaveInt(int(pMesh->m_vVertices.size( )));

        int i = 0;
        for (i = 0; i < int(pMesh-> m_vVertices.size( )); ++i)
        {
            const SIdvMeshVertex* pVertex = &pMesh->m_vVertices[i];
            cFile.SaveToken(File_Mesh_BeginVertex);

            cFile.SaveToken(File_Mesh_VertexPos);
            cFile.SaveVector3(pVertex->m_cPos);

            cFile.SaveToken(File_Mesh_VertexNormal);
            cFile.SaveVector3(pVertex->m_cNormal);

            cFile.SaveToken(File_Mesh_VertexBinormal);
            cFile.SaveVector3(pVertex->m_cBinormal);

            cFile.SaveToken(File_Mesh_VertexTangent);
            cFile.SaveVector3(pVertex->m_cTangent);

            cFile.SaveToken(File_Mesh_VertexTexCoord);
            cFile.SaveFloat(pVertex->m_afUV[0]);
            cFile.SaveFloat(pVertex->m_afUV[1]);

            cFile.SaveToken(File_Mesh_EndVertex);
        }

        cFile.SaveToken(File_Mesh_IndexCount);
        cFile.SaveInt(int(pMesh->m_vIndices.size( )));

        for (i = 0; i < int(pMesh->m_vIndices.size( )); ++i)
        {
            cFile.SaveToken(File_Mesh_Index);
            cFile.SaveInt(pMesh->m_vIndices[i]);
        }

        cFile.SaveToken(File_EndMesh);
    }

    cFile.SaveToken(File_EndMeshes);
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::ParseMeshes

void CTreeEngine::ParseMeshes(CTreeFileAccess& cFile)
{
    int nToken = cFile.ParseToken( );
    do
    {
        switch (nToken)
        {
        case File_NumMeshes:
            m_vMeshInfo.reserve(cFile.ParseInt( ));
            break;
        case File_BeginMesh:
        {
            SIdvMeshInfo* pNewMesh = new ("CTreeEngine::ParseMeshes, pNewMesh") SIdvMeshInfo;

            nToken = cFile.ParseToken( );

            do
            {
                switch (nToken)
                {
                case File_Mesh_Name:
                    cFile.ParseString(pNewMesh->m_strName);
                    break;
                case File_Mesh_VertexCount:
                    pNewMesh->m_vVertices.reserve(cFile.ParseInt( ));
                    break;
                case File_Mesh_BeginVertex:
                {
                    nToken = cFile.ParseToken( );
                    SIdvMeshVertex sNewVertex;
                    do
                    {
                        switch(nToken)
                        {
                        case File_Mesh_VertexPos:
                            sNewVertex.m_cPos = cFile.ParseVector3( );
                            break;
                        case File_Mesh_VertexNormal:
                            sNewVertex.m_cNormal = cFile.ParseVector3( );
                            break;
                        case File_Mesh_VertexBinormal:
                            sNewVertex.m_cBinormal = cFile.ParseVector3( );
                            break;
                        case File_Mesh_VertexTangent:
                            sNewVertex.m_cTangent = cFile.ParseVector3( );
                            break;
                        case File_Mesh_VertexTexCoord:
                            sNewVertex.m_afUV[0] = cFile.ParseFloat( );
                            sNewVertex.m_afUV[1] = cFile.ParseFloat( );
                            break;
                        default:
                            throw(IdvFileError("malformed mesh info"));
                        }

                        nToken = cFile.ParseToken( );
                    }
                    while (nToken != File_Mesh_EndVertex);

                    pNewMesh->m_vVertices.push_back(sNewVertex);
                }
                    break;
                case File_Mesh_IndexCount:
                    pNewMesh->m_vIndices.reserve(cFile.ParseInt( ));
                    break;
                case File_Mesh_Index:
                    pNewMesh->m_vIndices.push_back(cFile.ParseInt( ));
                    break;
                default:
                    throw(IdvFileError("malformed mesh info"));
                }

                nToken = cFile.ParseToken( );

            } while (nToken != File_EndMesh);

            AddMeshInfo(pNewMesh);
        }
            break;
        default:
            throw(IdvFileError("malformed mesh info"));
        }

        nToken = cFile.ParseToken( );

    }
    while (nToken != File_EndMeshes);
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::SaveLeafMeshInfo

void CTreeEngine::SaveLeafMeshInfo(CTreeFileAccess& cFile) const
{
    cFile.SaveToken(File_BeginLeafMeshInfo);

    for (unsigned int i = 0; i < m_sLeafInfo.m_vLeafTextures.size( ); ++i)
    {
        cFile.SaveToken(File_BeginLeafCluster);
        cFile.SaveToken(File_LeafMesh_UseMeshes);
        cFile.SaveBool(m_sLeafInfo.m_vLeafTextures[i].m_bUseMeshes);

        cFile.SaveToken(File_LeafMesh_Index);
        cFile.SaveInt(m_sLeafInfo.m_vLeafTextures[i].m_nMeshLeafIndex);

        cFile.SaveToken(File_LeafMesh_Hang);
        cFile.SaveFloat(m_sLeafInfo.m_vLeafTextures[i].m_fHang);

        cFile.SaveToken(File_LeafMesh_Rotate);
        cFile.SaveFloat(m_sLeafInfo.m_vLeafTextures[i].m_fRotate);

        cFile.SaveToken(File_EndLeafCluster);
    }

    cFile.SaveToken(File_EndLeafMeshInfo);
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::ParseLeafMeshInfo

void CTreeEngine::ParseLeafMeshInfo(CTreeFileAccess& cFile)
{
    int nLeafCluster = 0;

    int nToken = cFile.ParseToken( );
    do
    {
        if (nToken == File_BeginLeafCluster)
        {
            nToken = cFile.ParseToken( );
            do
            {
                switch (nToken)
                {
                case File_Num_LeafMesh_Indices:
                {
                    // vestigial token
                    (void) cFile.ParseInt( );
                }
                    break;
                case File_LeafMesh_Index:
                {
                    int nIndex = cFile.ParseInt( );
                    if (nLeafCluster < int(m_sLeafInfo.m_vLeafTextures.size( )))
                        m_sLeafInfo.m_vLeafTextures[nLeafCluster].m_nMeshLeafIndex = nIndex;
                }
                    break;
                case File_LeafMesh_UseMeshes:
                {
                    bool bUseMeshes = cFile.ParseBool( );
                    if (nLeafCluster < int(m_sLeafInfo.m_vLeafTextures.size( )))
                        m_sLeafInfo.m_vLeafTextures[nLeafCluster].m_bUseMeshes = bUseMeshes;
                }
                    break;
                case File_LeafMesh_Hang:
                {
                    float fHang = cFile.ParseFloat( );
                    if (nLeafCluster < int(m_sLeafInfo.m_vLeafTextures.size( )))
                        m_sLeafInfo.m_vLeafTextures[nLeafCluster].m_fHang = fHang;
                }
                    break;
                case File_LeafMesh_Rotate:
                {
                    float fRotate = cFile.ParseFloat( );
                    if (nLeafCluster < int(m_sLeafInfo.m_vLeafTextures.size( )))
                        m_sLeafInfo.m_vLeafTextures[nLeafCluster].m_fRotate = fRotate;
                }
                    break;
                default:
                    throw(IdvFileError("malformed leaf mesh info"));
                    break;
                }

                nToken = cFile.ParseToken( );
            }
            while(nToken != File_EndLeafCluster);
        }

        nToken = cFile.ParseToken( );

        ++nLeafCluster;
    }
    while (nToken != File_EndLeafMeshInfo);
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::GetMeshInfo

const SIdvMeshInfo* CTreeEngine::GetMeshInfo(int iIndex) const
{
    SIdvMeshInfo* pReturn = NULL;

    if (iIndex > -1 && iIndex < int(m_vMeshInfo.size( )))
        pReturn = m_vMeshInfo[iIndex];

    return pReturn;
}



///////////////////////////////////////////////////////////////////////
//  CTreeEngine::SaveSupplementalGlobalInfo

void CTreeEngine::SaveSupplementalGlobalInfo(CTreeFileAccess& cFile) const
{
    cFile.SaveToken(File_BeginSupplementalGlobalInfo);

    cFile.SaveToken(File_TreeAngle);
    cFile.SaveFloat(m_fTreeAngle);

    cFile.SaveToken(File_EndSupplementalGlobalInfo);
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::ParseSupplementalGlobalInfo

void CTreeEngine::ParseSupplementalGlobalInfo(CTreeFileAccess& cFile)
{
    int nToken = cFile.ParseToken( );
    do
    {
        switch (nToken)
        {
        case File_TreeAngle:
            m_fTreeAngle = cFile.ParseFloat( );
            break;
        case File_EndSupplementalGlobalInfo:
            break;
        default:
            throw(IdvFileError("malformed supplemental global info"));
            break;
        }

        nToken = cFile.ParseToken( );
    }
    while (nToken != File_EndSupplementalGlobalInfo);
}


///////////////////////////////////////////////////////////////////////
//  function SetMissingProfiles

void SetMissingProfiles(SIdvBranchInfo* pInfo)
{
    if (!pInfo->GetCrossSectionProfile( )->IsInitialized( ))
        pInfo->GetCrossSectionProfile( )->Parse("BezierSpline 0 1 0 { 2 0 1 1 0 0.1 1 1 1 0 0.1 }");

    if (!pInfo->GetLightSeamProfile( )->IsInitialized( ))
        pInfo->GetLightSeamProfile( )->Parse("BezierSpline 0 1 0 { 2 0 1 1 0 0.1 1 1 1 0 0.1 }");

    if (!pInfo->GetRoughnessProfile( )->IsInitialized( ))
        pInfo->GetRoughnessProfile( )->Parse("BezierSpline 0 1 0 { 2 0 1 1 0 0.1 1 1 1 0 0.1 }");

    if (!pInfo->GetFrequencyProfile( )->IsInitialized( ))
        pInfo->GetFrequencyProfile( )->Parse("BezierSpline 0 1 0 { 2 0 1 1 0 0.1 1 1 1 0 0.1 }");

    if (!pInfo->GetLightSeamBias( )->IsInitialized( ))
        pInfo->GetLightSeamBias( )->Parse("BezierSpline 0 1 0 { 2 0 1 1 0 0.1 1 1 1 0 0.1 }");

    if (!pInfo->GetGnarlProfile( )->IsInitialized( ))
        pInfo->GetGnarlProfile( )->Parse("BezierSpline 0 1 0 { 2 0 1 1 0 0.1 1 1 1 0 0.1 }");
}


///////////////////////////////////////////////////////////////////////
//  CTreeEngine::ParseSupplementalGlobalInfo
//
//  Some branch parameters may not have been parsed because they weren't part
//  of the older SPT file formats.  These aren't given default values to avoid
//  redundant computation because the stBezierSpline::Parse function is expensive
//  since it involves a lot of spline computation.

void CTreeEngine::InitMissingBranchProfiles(void)
{
    // roots
    SetMissingProfiles(&m_pRootSupport->m_sRoots);

    // branch levels
    for (int nLevel = 0; nLevel < int(m_vBranchInfo.size( )); ++nLevel)
    {
        SIdvBranchInfo* pBranchInfo = m_vBranchInfo[nLevel];
        SetMissingProfiles(pBranchInfo);
    }
}


