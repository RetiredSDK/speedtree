///////////////////////////////////////////////////////////////////////
//  Branch.cpp
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

#include "Debug.h"
#include "TreeEngine.h"
#include "FileAccess.h"
#include "LibRandom_Source/IdvRandom.h"
#include "BillboardLeaf.h"
#include "SpeedTreeRT.h"
#include "IndexedGeometry.h"
#include "LightingEngine.h"
#include "FrondEngine.h"
#include <algorithm>
#include "Random.h"
#include "UpVector.h"

// branch-related constants
const stVec3 g_cOut(1.0f, 0.0f, 0.0f);
const stVec3 g_cDown(0.0f, 0.0f, -1.0f);
const stVec3 g_cUp(0.0f, 0.0f, 1.0f);
const float c_fInverseOf90 = 1.0f / 90.0f;
const float c_fParentRadiusFactor = 0.85f;
const int c_nMinCrossSegments = 3;


// static variables
//
// if any of these variable must be saved, there should be
// a copy in CTreeEngine

int CBranch::m_nRefCount = 0;
st_vector_int CBranch::m_vBlossoms;
st_vector_int CBranch::m_vNonBlossoms;
st_vector_const_branchinfo CBranch::m_vBranchInfo;
bool CBranch::m_bComputeLeaves = true;
SIdvLeafInfo* CBranch::m_pLeafInfo = NULL;
st_vector_leaves CBranch::m_vLocalLeaves;
CLightingEngine* CBranch::m_pLightingEngine = NULL;
stRandom CBranch::m_cRandom;
int CBranch::m_nWeightLevel = 1;
int CBranch::m_nRoundRobinWindLevel = 0;
CFrondEngine* CBranch::m_pFrondEngine = NULL;
bool CBranch::m_bPropagateFlexibility = false;
bool CBranch::m_bFloorEnabled = false;
float CBranch::m_fFloor = 0.0f;
int CBranch::m_nFloorLevel = 1;
float CBranch::m_fFloorExponent = 1.0f;
float CBranch::m_fFloorBias = 1.0f;
int CBranch::m_nFirstBranchLevel = 0;
float CBranch::m_afRandomBank[c_nBankSize];
int CBranch::m_nRandomIndex = 0;
int CBranch::m_nFrequencyIndex = 0;
SRootSupportInfo* CBranch::m_pRootSupport = NULL;
int CBranch::m_nMeshRotateIndex = 0;


///////////////////////////////////////////////////////////////////////
//  CBranch::CBranch definition

CBranch::CBranch(CBranch* pParent, bool bDestroyChildren) :
    m_pParent(pParent),
    m_fPercentOfParent(0.0f),
    m_pVertices(NULL),
    m_nNumVertices(-1),
    m_usCrossSectionSegments(0),
    m_nStartingOffset(-1),
    m_fVolume(0.0f),
    m_fFuzzyVolume(0.0f),
    m_bDestroyChildren(bDestroyChildren),
    m_bUsingVertexStack(false)
{
    ++m_nRefCount;
}


///////////////////////////////////////////////////////////////////////
//  CBranch::~CBranch definition

CBranch::~CBranch( )
{
    if (!m_bUsingVertexStack)
        st_delete_array<SIdvBranchVertex>(m_pVertices, "~CBranch, m_pVertices");

    if (m_bDestroyChildren)
        for (unsigned int i = 0; i < m_vChildren.size( ); ++i)
            st_delete<CBranch>(m_vChildren[i].m_pBranch, "~CBranch, m_vChildren[i].m_pBranch");

    if (--m_nRefCount == 0)
    {
        // dealloc vectors
        st_vector_int( ).swap(m_vBlossoms);
        st_vector_int( ).swap(m_vNonBlossoms);
        st_vector_const_branchinfo( ).swap(m_vBranchInfo);
        st_vector_leaves( ).swap(m_vLocalLeaves);
    }
    st_assert(m_nRefCount >= 0);
}


///////////////////////////////////////////////////////////////////////
//  CBranch::Compute definition

CFrondEngine::EConditions CBranch::Compute(unsigned int nSeed,
                                           float fSize,
                                           int nLevel,
                                           const stVec3& cBasePos,
                                           float fPercentOfParent,
                                           const stRotTransform& cBaseTransform,
                                           const stVec3& cZAxis,
                                           CIndexedGeometry* pBranchGeometry,
                                           st_vector_leaves& vLeaves,
                                           float fWindWeight1,
                                           float fWindWeight2,
                                           int nWindGroup1,
                                           int nWindGroup2,
                                           float fParentRadius,
                                           bool bUseRoots)
{
    // reset mesh leaf rotation index
    m_nMeshRotateIndex = 0;

    // find out if fronds instead of branches will be generated from this level
    CFrondEngine::EConditions eFrondCondition = nLevel >= m_pFrondEngine->GetLevel( ) ? PassesFrondRules(this, fPercentOfParent) : CFrondEngine::DISABLED;
    if (eFrondCondition == CFrondEngine::ENABLED && bUseRoots)
        eFrondCondition = CFrondEngine::DISABLED;

    if (eFrondCondition == CFrondEngine::PRUNED)
        return eFrondCondition;

    // determine which wind group will be assigned to this branch (uses round-robin approach).
    // the wind group will be adjusted to fit into the specified wind matrix start and span
    // values after Compute()
    if (nLevel == m_nWeightLevel)
        nWindGroup1 = m_nRoundRobinWindLevel++;
    if (nLevel == m_nWeightLevel + 1)
        nWindGroup2 = m_nRoundRobinWindLevel++;

    // since level 0 only occurs once per tree, we use this condition to initialize
    // the blossom vectors used in creating the leaves
    if (nLevel == 0)
        BuildBlossomVectors( );

    // setup up the texture tiling parameters
    SIdvBranchInfo& sInfo = bUseRoots ? m_pRootSupport->m_sRoots : const_cast<SIdvBranchInfo&>(*m_vBranchInfo[nLevel]);

    if (eFrondCondition == CFrondEngine::DISABLED && nLevel >= m_nFirstBranchLevel)
        m_nStartingOffset = pBranchGeometry->GetVertexCounter( ); // remember where this branch's geometry is stored in the branch geometry class
    m_fPercentOfParent = fPercentOfParent;

    // compute the flare entries
    if (eFrondCondition == CFrondEngine::DISABLED)
        ComputeFlareEntries(sInfo);

    // this branch was created from a parent - fPercentOfParent is the location
    // (from 0.0 to 1.0) along the length of the parent where this branch was created.
    // the attributes that govern this branch start by looking at the parent's
    // attributes at the point of the child's creation.
    float fLength = sInfo.GetLength( )->Evaluate(fPercentOfParent) * fSize;
    float fSegmentLength = fLength / sInfo.m_nSegments;
    float fAzimuth = m_cRandom.GetUniform(-180.0f, 180.0f);
    float fStartAngle = sInfo.GetStartAngle( )->Evaluate(fPercentOfParent);
    float fGravity = sInfo.GetGravity( )->Evaluate(fPercentOfParent);
    float fRadius = sInfo.GetRadius( )->Evaluate(fPercentOfParent) * fSize;

    if (fRadius < 0.0f)
        fRadius = -fRadius;

    float fFlexibility = sInfo.GetFlexibility( )->Evaluate(fPercentOfParent);
    m_usCrossSectionSegments = static_cast<unsigned short>(sInfo.m_nCrossSectionSegments);

    // set texcoords tiling
    float fTileOffset = ((fAzimuth + 180.0f) / 360.0f) + fPercentOfParent;

    // make sure radius of this child branch does not exceed radius at parent
    float fMaxPossibleRadius = sInfo.GetRadius( )->GetMax( ) * fSize;
    float fPotentialLimit = fParentRadius * c_fParentRadiusFactor;
    if (nLevel > m_nFirstBranchLevel && fParentRadius > 0.0f && fPotentialLimit < fMaxPossibleRadius)
        fMaxPossibleRadius = fPotentialLimit;

    if (fRadius > fMaxPossibleRadius)
        fRadius = fMaxPossibleRadius;

    float fFirstRadius = fRadius * sInfo.GetRadiusScale( )->Evaluate(0.0f);

    // setup geometry information, passing indices into pBranchGeometry
    m_nNumVertices = sInfo.m_nSegments + 1;
    if (eFrondCondition == CFrondEngine::ENABLED && m_pFrondEngine->IsUsingSegmentOverride( ))
    {
        m_nNumVertices = static_cast<int>(m_pFrondEngine->GetSegmentOverride( ));
    }
    else
    {
        if (sInfo.m_fMinLengthPercent != 1.0f)
        {
            float fMinLength = sInfo.GetLength( )->GetMin( ) * fSize;
            float fMaxLength = sInfo.GetLength( )->GetMax( ) * fSize;
            if (fMinLength < fMaxLength)
            {
                float fPercent = (fLength - fMinLength) / (fMaxLength - fMinLength);
                if (fPercent < 0.0f)
                    fPercent = 0.0f;
                else if (fPercent > 1.0f)
                    fPercent = 1.0f;

                m_nNumVertices = static_cast<int>(VecInterpolate(m_nNumVertices * sInfo.m_fMinLengthPercent, float(m_nNumVertices), fPercent));
                if (m_nNumVertices < 2)
                    m_nNumVertices = 2;
            }
        }

        if (sInfo.m_fMinCrossSectionPercent != 1.0f)
        {
            float fMinRadius = sInfo.GetRadius( )->GetMin( ) * fSize;
            float fMaxRadius = sInfo.GetRadius( )->GetMax( ) * fSize;
            if (fMinRadius < fMaxRadius)
            {
                float fPercent = (fRadius - fMinRadius) / (fMaxRadius - fMinRadius);
                if (fPercent < 0.0f)
                    fPercent = 0.0f;
                else if (fPercent > 1.0f)
                    fPercent = 1.0f;

                int nTempSegments = static_cast<int>(VecInterpolate(m_usCrossSectionSegments * sInfo.m_fMinCrossSectionPercent, m_usCrossSectionSegments, fPercent));
                if (nTempSegments < c_nMinCrossSegments)
                    nTempSegments = c_nMinCrossSegments;
                m_usCrossSectionSegments = static_cast<unsigned short>(nTempSegments);
            }
        }
    }

    if (m_nNumVertices >= 2)
    {
        int nStart = pBranchGeometry->GetVertexCounter( );

        if (m_nNumVertices < c_nMaxStackPoints)
        {
            m_pVertices = m_asVerticesStack;
            m_bUsingVertexStack = true;
        }
        else
            m_pVertices = st_new_array<SIdvBranchVertex>(m_nNumVertices, "CBranch::Compute, m_pVertices");

        // compute the first vertex of the branch - it's different than the subsequent vertices
        bool bEverAboveFloor = false;
        SIdvBranchVertex* pFirstVertex = m_pVertices;
        pFirstVertex->m_cPos = cBasePos;
        if (cBasePos[2] >= m_fFloor)
            bEverAboveFloor = true;

        float fAzimuthDisturbance = sInfo.GetDisturbance( )->ScaledVariance(0.0f);
        float fPitchDisturbance = sInfo.GetDisturbance( )->ScaledVariance(0.0f);
        pFirstVertex->m_cTrans = cBaseTransform;
        stVec3 cRotAxis = cBaseTransform * cZAxis;
        
        pFirstVertex->m_cTrans.RotateAxis(fAzimuth, cRotAxis);
        pFirstVertex->m_cTrans.RotateYZ(fStartAngle + fPitchDisturbance, fAzimuthDisturbance);
        pFirstVertex->m_cDirection = g_cOut * pFirstVertex->m_cTrans;
        pFirstVertex->m_fRadius = fRadius * sInfo.GetRadiusScale( )->Evaluate(0.0f, true);
        float fVertexFlexibility = fFlexibility * (sInfo.GetFlexibilityScale( )->Evaluate(0.0f));

        // adjust first vertex for gravity effect
        float fGravityAngle(VecRad2Deg(pFirstVertex->m_cDirection.AngleBetween(g_cDown)));
        float fFacingFactor(1.0f - c_fInverseOf90 * (fabsf(90.0f - fGravityAngle)));
        stVec3 cAdjustmentAxis(pFirstVertex->m_cDirection * g_cDown);
        cAdjustmentAxis.Normalize( );
        float fGravityProfile = -1.0f * ((sInfo.GetAngleProfile( )->Evaluate(0.0f) - 0.5f) * 2.0f);
        stRotTransform cTransformAdjustment;
        cTransformAdjustment.RotateAxisFromIdentity(fGravityProfile * fGravity * fGravityAngle * fFacingFactor, cAdjustmentAxis);
        pFirstVertex->m_cTrans = pFirstVertex->m_cTrans * cTransformAdjustment;

        float fGnarl = sInfo.m_fGnarl * sInfo.GetGnarlProfile( )->EvaluateWithoutVariance(0.0f);
        bool bNegateGnarl = !sInfo.m_bGnarlUnison && (int) fAzimuth % 2 == 0;
        if (bNegateGnarl)
            fGnarl = -fGnarl;
        stRotTransform cGnarlTransform;
        cGnarlTransform.RotateZ(fGnarl);
        pFirstVertex->m_cTrans = pFirstVertex->m_cTrans * cGnarlTransform;
        
        pFirstVertex->m_cDirection = g_cOut * pFirstVertex->m_cTrans;

        // forking
        pFirstVertex->m_nForkCount = 0;

        // build the cross section associated with this first vertex
        float fCrossSectionWeight1 = fWindWeight1;
        float fCrossSectionWeight2 = fWindWeight2;
        ComputeVertexWeights(fCrossSectionWeight1, fCrossSectionWeight2, nLevel, 0.0f, fVertexFlexibility);
        pFirstVertex->m_fWindWeight1 = fCrossSectionWeight1;
        pFirstVertex->m_fWindWeight2 = fCrossSectionWeight2;

        float fAdjustPercentage = sInfo.GetCrossSectionProfile( )->EvaluateWithoutVariance(0.0f);
        fAdjustPercentage = st_min(1.0f, st_max(0.0f, fAdjustPercentage));
        unsigned short usAdjustCrossSegments = static_cast<unsigned short>(VecInterpolate(3, m_usCrossSectionSegments, fAdjustPercentage));

        if (eFrondCondition == CFrondEngine::DISABLED)
        {
            // regular branch geometry
            if (nLevel >= m_nFirstBranchLevel)
                BuildCrossSection(pFirstVertex, 0.0f, usAdjustCrossSegments, pBranchGeometry, sInfo, fCrossSectionWeight1, fCrossSectionWeight2, nWindGroup1, nWindGroup2, fTileOffset, &sInfo, fMaxPossibleRadius, fSize, pFirstVertex->m_fRadius, fLength, bNegateGnarl);
        }
        else
        {
            // start a new frond and add a vertex
            m_pFrondEngine->StartGuide(m_nNumVertices);
            m_pFrondEngine->AddGuideVertex(pFirstVertex->m_cPos, pFirstVertex->m_cTrans, fCrossSectionWeight1, fCrossSectionWeight2, nWindGroup1, nWindGroup2);
        }

        float fRunningLength = 0.0f;
        m_pVertices[0].m_fRunningLength = 0.0f;

        // calculate the rest of the branch's vertices
        SIdvBranchVertex* pLastVertex = pFirstVertex;
        int i = 0;
        for (i = 1; i < m_nNumVertices; ++i)
        {
            SIdvBranchVertex* pVertex = m_pVertices + i;

            // calculate how far along the branch's length we are
            float fLinearProgress = i / static_cast<float>(m_nNumVertices - 1);
            float fProgress = powf(fLinearProgress, sInfo.m_fSegmentPackingExponent);
            fSegmentLength = (fProgress * fLength) - fRunningLength;

            fAdjustPercentage = sInfo.GetCrossSectionProfile( )->EvaluateWithoutVariance(fProgress);
            fAdjustPercentage = st_min(1.0f, st_max(0.0f, fAdjustPercentage));
            usAdjustCrossSegments = static_cast<unsigned short>(VecInterpolate(3, m_usCrossSectionSegments, fAdjustPercentage));

            // evaluate radius and flexibility
            pVertex->m_fRadius = fRadius * sInfo.GetRadiusScale( )->Evaluate(fProgress, true);
            float fLocalVertexFlexibility = fFlexibility * (sInfo.GetFlexibilityScale( )->Evaluate(fProgress));

            // adjust growth direction, taking disturbance into account
            pVertex->m_cTrans = pLastVertex->m_cTrans;
            pVertex->m_cDirection = g_cOut * pVertex->m_cTrans;

            // adjust for gravity effect
            float fLocalGravityAngle = VecRad2Deg(pVertex->m_cDirection.AngleBetween(g_cDown));
            float fLocalFacingFactor = (1.0f - c_fInverseOf90 * (fabsf(90.0f - fLocalGravityAngle)));
            stVec3 cThisAdjustmentAxis(pVertex->m_cDirection * g_cDown);
            cThisAdjustmentAxis.Normalize( );
            float fLocalGravityProfile = -1.0f * ((sInfo.GetAngleProfile( )->Evaluate(fProgress) - 0.5f) * 2.0f);
            cTransformAdjustment.RotateAxisFromIdentity(fLocalGravityProfile * fGravity * fLocalGravityAngle * fLocalFacingFactor, cThisAdjustmentAxis);
            pVertex->m_cTrans = pVertex->m_cTrans * cTransformAdjustment;

            // keep the branch from being perfectly straight
            float fLocalAzimuthDisturbance = sInfo.GetDisturbance( )->ScaledVariance(fProgress);
            float fLocalPitchDisturbance = sInfo.GetDisturbance( )->ScaledVariance(fProgress);
            pVertex->m_cTrans.RotateYZ(fLocalPitchDisturbance, fLocalAzimuthDisturbance);

            fGnarl = sInfo.m_fGnarl * sInfo.GetGnarlProfile( )->EvaluateWithoutVariance(fProgress);
            if (bNegateGnarl)
                fGnarl = -fGnarl;
            //stRotTransform cGnarlTransform;
            cGnarlTransform.LoadIdentity( );
            cGnarlTransform.RotateZ(fGnarl);
            pVertex->m_cTrans = pVertex->m_cTrans * cGnarlTransform;

            pVertex->m_cDirection = g_cOut * pVertex->m_cTrans;

            // apply final vertex position
            pVertex->m_cPos = pLastVertex ? pLastVertex->m_cPos + (pLastVertex->m_cDirection * fSegmentLength) : cBasePos;
            if (m_bFloorEnabled && bEverAboveFloor && nLevel >= m_nFloorLevel && (!pLastVertex || pVertex->m_cPos[2] <= pLastVertex->m_cPos[2]))
            {
                float fNewProgress = powf(fLinearProgress, m_fFloorExponent);
                if (pVertex->m_cPos[2] < m_fFloor)
                    pVertex->m_cPos[2] = VecInterpolate(m_fFloor, pVertex->m_cPos[2], fNewProgress * m_fFloorBias);
            }
            if (pVertex->m_cPos[2] >= m_fFloor)
                bEverAboveFloor = true;

            // forking
            pVertex->m_nForkCount = 0;

            // pass geometry data into the pBranchGeometry class via BuildCrossSection()
            float fLocalCrossSectionWeight1 = fWindWeight1;
            float fLocalCrossSectionWeight2 = fWindWeight2;
            ComputeVertexWeights(fLocalCrossSectionWeight1, fLocalCrossSectionWeight2, nLevel, fProgress, fLocalVertexFlexibility);
            pVertex->m_fWindWeight1 = fLocalCrossSectionWeight1;
            pVertex->m_fWindWeight2 = fLocalCrossSectionWeight2;

            if (eFrondCondition == CFrondEngine::DISABLED)
            {
                // regular branch geometry
                if (nLevel >= m_nFirstBranchLevel)
                    BuildCrossSection(pVertex, fProgress, usAdjustCrossSegments, pBranchGeometry, sInfo, fLocalCrossSectionWeight1, fLocalCrossSectionWeight2, nWindGroup1, nWindGroup2, fTileOffset, &sInfo, fMaxPossibleRadius, fSize, pFirstVertex->m_fRadius, fLength, bNegateGnarl);
            }
            else
            {
                // frond geometry
                m_pFrondEngine->AddGuideVertex(pVertex->m_cPos, pVertex->m_cTrans, fLocalCrossSectionWeight1, fLocalCrossSectionWeight2, nWindGroup1, nWindGroup2);
            }

            pLastVertex = pVertex;
            fRunningLength += fSegmentLength;
            m_pVertices[i].m_fRunningLength = fRunningLength;
        }

        if (eFrondCondition == CFrondEngine::DISABLED && nLevel >= m_nFirstBranchLevel)
        {
            st_vector_int vIndices;
            vIndices.reserve(200);

            int nRunningCount = 0;
            for (i = 0; i < m_nNumVertices - 1; ++i)
            {
                int nThisCrossCount = m_pVertices[i].m_nCrossSegments;
                int nNextCrossCount = m_pVertices[i + 1].m_nCrossSegments;

                int nMaxCrossCount = st_max(nThisCrossCount, nNextCrossCount);
                for (int j = 0; j < nMaxCrossCount; ++j)
                {
                    float fThisProgress = j / static_cast<float>(nMaxCrossCount - 1.0f);
                    int nThisIndex = static_cast<int>((nThisCrossCount - 1) * fThisProgress);
                    int nNextIndex = static_cast<int>((nNextCrossCount - 1) * fThisProgress);

                    vIndices.push_back(static_cast<unsigned short>(nStart + nRunningCount + nThisCrossCount + nNextIndex));
                    vIndices.push_back(static_cast<unsigned short>(nStart + nRunningCount + nThisIndex));
                }

                int nRepeat = vIndices[vIndices.size( ) - 2];
                // repeat this vertex so we can combine the multiple segments of one branch
                vIndices.push_back(nRepeat);
                // repeat it again to counteract the clockwise vertex ordering that would result otherwise
                vIndices.push_back(nRepeat);

                nRunningCount += nThisCrossCount;
            }

            pBranchGeometry->AddStrip(0, &vIndices[0], int(vIndices.size( )));
            //vIndices.clear( );
            pBranchGeometry->AdvanceStripCounter( );
        }

        if (eFrondCondition == CFrondEngine::ENABLED)
        {
            m_pFrondEngine->EndGuide(fFirstRadius);
        }
        else if (nLevel >= m_nFirstBranchLevel)
        {
            pBranchGeometry->SetVertexCounter(static_cast<unsigned short>(m_nStartingOffset));
            ComputeBranchNormals(pBranchGeometry, sInfo.GetLightSeamProfile( ), sInfo.GetLightSeamBias( )->EvaluateWithoutVariance(fPercentOfParent));
        }

        // this branch's geometry is complete, now determine the number of children -
        // children can be additional branches or "buds" that contain no actual geometry
        // but are used as placeholders for leaf clusters
        int nNumChildBranches = static_cast<int>((sInfo.m_fFrequency / fSize) * fLength);

        ++nLevel; // level 0 = trunk, level 1 = trunk's immediate children, etc.

        int nTotalChildBranches = nNumChildBranches;
        if (nLevel == m_pRootSupport->m_nLevel)
            nTotalChildBranches += static_cast<int>((m_pRootSupport->m_fFrequency / fSize) * fLength);

        unsigned int uiRootSeed = nSeed;

        if (bUseRoots)
            nTotalChildBranches = 0;

        // bChildrenAreBuds tells us if buds will be created from this branch
        bool bChildrenAreBuds = nLevel >= static_cast<int>(m_vBranchInfo.size( ) - 1);
        if (!bChildrenAreBuds || m_bComputeLeaves)
        {
            for (i = 0; i < nTotalChildBranches; ++i)
            {
                bool bRootCandidate = i >= nNumChildBranches;

                SIdvBranch sBranch;

                // the random number generator is reseeded here in order to make EstimateCompute give
                // exact predictions of the size of the tree.  it is only reseeded for normal branches
                // and not buds because it is a relatively expensive operation
                if (i == nNumChildBranches)
                {
                    m_cRandom.Reseed(uiRootSeed);
                }
                else if (!bChildrenAreBuds && i < nNumChildBranches)
                {
                    nSeed += 3;
                    m_cRandom.Reseed(nSeed);
                }

                // make sure that at least one branch (the first one) grows near the end of the parent.
                // trust us, this makes for better looking trees.
                float fPercent = 0.0f;
                if (!bChildrenAreBuds && i == 0 && (nLevel > m_nFirstBranchLevel) && !bRootCandidate)
                {
                    fPercent = m_cRandom.GetUniform(VecInterpolate(sInfo.m_fFirstBranch, sInfo.m_fLastBranch, 0.85f), VecInterpolate(sInfo.m_fFirstBranch, sInfo.m_fLastBranch, 0.95f));
                }
                else
                {
                    if (bRootCandidate)
                        fPercent = m_cRandom.GetUniform(m_pRootSupport->m_fFirst, m_pRootSupport->m_fLast);
                    else
                        fPercent = m_cRandom.GetUniform(sInfo.m_fFirstBranch, sInfo.m_fLastBranch);
                }

                if (!bRootCandidate)
                {
                    bool bCulled = false;
                    float fTest = (fPercent - sInfo.m_fFirstBranch) / (sInfo.m_fLastBranch - sInfo.m_fFirstBranch);
                    if (fTest < 0.0f)
                        fTest = 0.0f;
                    else if (fTest > 1.0f)
                        fTest = 1.0f;

                    float fFrequencyTest = sInfo.GetFrequencyProfile( )->EvaluateWithoutVariance(fTest);
                    if (fFrequencyTest != 1.0f)
                        bCulled = m_afRandomBank[m_nFrequencyIndex++ % c_nBankSize] > fFrequencyTest;

                    if (bCulled)
                        continue;
                }

                if (!bRootCandidate && !bChildrenAreBuds && sInfo.m_bForkingEnabled && m_cRandom.GetUniform(0.0f, 1.0f) <= sInfo.m_fForkBias)
                {
                    fPercent = ComputeForkPercent(fPercent, fLength, sInfo.m_fForkAngle, sInfo.m_nForkLimit);

                    if (fPercent < sInfo.m_fFirstBranch || fPercent > sInfo.m_fLastBranch)
                        continue;
                }

                // prune the branch if necessary
                if (Prune(this, fPercent, sInfo.m_uiPruningDepth, sInfo.m_fPruningPercent))
                    continue;

                // find out which of the parent's vertexes the child is created between
                FillBranch(sBranch, fPercent * fLength);

                // what's the vertex weight where the child will be spawned?
                float fWeightAtChild1 = fWindWeight1;
                float fWeightAtChild2 = fWindWeight1;
                //if (nLevel - 1 == m_nWeightLevel || m_bPropagateFlexibility)
                //{
                    fWeightAtChild1 = VecInterpolate(m_pVertices[sBranch.m_nPreVertexIndex].m_fWindWeight1, m_pVertices[sBranch.m_nPreVertexIndex + 1].m_fWindWeight1, sBranch.m_fPercent);
                    fWeightAtChild2 = VecInterpolate(m_pVertices[sBranch.m_nPreVertexIndex].m_fWindWeight2, m_pVertices[sBranch.m_nPreVertexIndex + 1].m_fWindWeight2, sBranch.m_fPercent);
                //}

                // another reseeding operation (for predictability in EstimateCompute)
                if (!bChildrenAreBuds && !bRootCandidate)
                {
                    m_cRandom.Reseed(nSeed);
                    (void) m_cRandom.GetUniform(0.0f, 100.0f);
                }

                // allocate child CBranch
                stVec3 cPos = VecInterpolate(m_pVertices[sBranch.m_nPreVertexIndex].m_cPos, m_pVertices[sBranch.m_nPreVertexIndex + 1].m_cPos, sBranch.m_fPercent);

                float fProfilePercent = 1.0f;
                if (!bRootCandidate)
                {
                    if (sInfo.m_fLastBranch != sInfo.m_fFirstBranch)
                        fProfilePercent = (fPercent - sInfo.m_fFirstBranch) / (sInfo.m_fLastBranch - sInfo.m_fFirstBranch);
                }
                else
                {
                    if (m_pRootSupport->m_fLast != m_pRootSupport->m_fFirst)
                        fProfilePercent = (fPercent - m_pRootSupport->m_fFirst) / (m_pRootSupport->m_fLast - m_pRootSupport->m_fFirst);
                }

                if (bChildrenAreBuds)
                {
                    // create the bud and possibly the leaf
                    ComputeBud(this, fSize, nLevel, cPos, fProfilePercent, m_pVertices[sBranch.m_nPreVertexIndex].m_cTrans, m_pVertices[sBranch.m_nPreVertexIndex].m_cDirection, vLeaves, fWeightAtChild1, fWeightAtChild2, nWindGroup1, nWindGroup2);
                }
                else
                {
                    // make a new child branch and recursively call CBranch::Compute()
                    float fRadiusOfParentAtBranch = VecInterpolate(m_pVertices[sBranch.m_nPreVertexIndex].m_fRadius, m_pVertices[sBranch.m_nPreVertexIndex + 1].m_fRadius, sBranch.m_fPercent);

                    // pass in a stack copy to help with fragmentation
                    CBranch s_cNewBranch(this, false);
                    sBranch.m_pBranch = &s_cNewBranch;

                    CFrondEngine::EConditions eChildCondition = CFrondEngine::PRUNED;
                    if (nLevel == m_nFirstBranchLevel)
                        eChildCondition = sBranch.m_pBranch->Compute(nSeed, fSize, nLevel, cPos, fProfilePercent, stRotTransform( ), stVec3(0.0f, 0.0f, 1.0f), pBranchGeometry, vLeaves, fWeightAtChild1, fWeightAtChild2, nWindGroup1, nWindGroup2, fRadiusOfParentAtBranch);
                    else
                        eChildCondition = sBranch.m_pBranch->Compute(nSeed, fSize, nLevel, cPos, fProfilePercent, m_pVertices[sBranch.m_nPreVertexIndex].m_cTrans, m_pVertices[sBranch.m_nPreVertexIndex].m_cDirection, pBranchGeometry, vLeaves, fWeightAtChild1, fWeightAtChild2, nWindGroup1, nWindGroup2, fRadiusOfParentAtBranch, bRootCandidate);

                    if (!m_pFrondEngine->Enabled( ) || (eChildCondition == CFrondEngine::DISABLED))
                    {
                        // make a heap copy
                        CBranch* pCopy = new ("CBranch copy") CBranch(sBranch.m_pBranch->m_pParent);
                        *pCopy = *sBranch.m_pBranch;
                        sBranch.m_pBranch = pCopy;
                        pCopy->m_bDestroyChildren = true;

                        if (!s_cNewBranch.m_bUsingVertexStack)
                            st_delete_array<SIdvBranchVertex>(s_cNewBranch.m_pVertices, "sBranch.m_pBranch->m_pVertices temporary");

                        m_vChildren.push_back(sBranch);
                    }
                    else if (!sBranch.m_pBranch->m_vChildren.empty( ))
                    {
                        for (int j = 0; j < int(sBranch.m_pBranch->m_vChildren.size( )); ++j)
                            st_delete<CBranch>(sBranch.m_pBranch->m_vChildren[j].m_pBranch, "CBranch::Compute, sBranch.m_pBranch->m_vChildren[h].m_pBranch");
                        st_vector_sbranches( ).swap(sBranch.m_pBranch->m_vChildren); // dealloc vector
                    }
                }
            }
        }

        if (bChildrenAreBuds)
            st_vector_leaves( ).swap(m_vLocalLeaves); // dealloc vector
        ComputeVolume( );
    }

    return eFrondCondition;
}


///////////////////////////////////////////////////////////////////////
//  CBranch::ComputeBud definition

void CBranch::ComputeBud(const CBranch* pParent,
                            float fSize,
                            int nLevel,
                            const stVec3& cBasePos,
                            float fPercentOfParent,
                            const stRotTransform& cBaseTransform,
                            const stVec3& cZAxis,
                            st_vector_leaves& vLeaves,
                            float fWindWeight1,
                            float fWindWeight2,
                            int nWindGroup1,
                            int nWindGroup2) const
{
    SIdvBranchInfo& sInfo = const_cast<SIdvBranchInfo&>(*m_vBranchInfo[nLevel]);
    const float c_fStartAngle = 60.0f;

    // always need 2 vertices for a bud - one on the parent branch and another for the leaf location
    const int c_nBudVertexCount = 2;
    SIdvBranchVertex acVertices[c_nBudVertexCount];

    float fLength = sInfo.GetLength( )->Evaluate(fPercentOfParent) * fSize;
    float fSegmentLength = fLength / sInfo.m_nSegments;
    float fAzimuth = m_cRandom.GetUniform(-180.0f, 180.0f);
    stVec3 cRotAxis = cBaseTransform * cZAxis;

    // compute position and orientation of first vertex
    SIdvBranchVertex *pFirstVertex = acVertices + 0;
    pFirstVertex->m_cPos = cBasePos;
    pFirstVertex->m_cTrans = cBaseTransform;
    pFirstVertex->m_cTrans.RotateAxis(fAzimuth, cRotAxis);
    pFirstVertex->m_cTrans.RotateY(c_fStartAngle);
    pFirstVertex->m_cDirection = g_cOut * pFirstVertex->m_cTrans;

    // compute leaf vertex
    SIdvBranchVertex *pVertex = acVertices + 1;
    pVertex->m_cPos = pFirstVertex->m_cPos + (pFirstVertex->m_cDirection * fSegmentLength);

    // compute geometric normal of leaf
    stVec3 cNormal = acVertices[1].m_cPos - acVertices[0].m_cPos;
    cNormal.Normalize( );

    // compute the general direction the parent branch was growing
    stVec3 cParentDir;
    if (pParent == NULL || pParent->m_nNumVertices == 0)
        cParentDir = cNormal;
    else
    {
        // back up to the dimming level if necessary
        if (m_pLeafInfo->m_nDimmingDepth == 0)
        {
            // track the shape of the branch
            pVertex = &pParent->m_pVertices[static_cast<int>(fPercentOfParent * (pParent->m_nNumVertices - 1.0))];
            cParentDir = pVertex->m_cDirection;
        }
        else
        {
            // find the connection point
            stVec3 cBase = pParent->m_pVertices[0].m_cPos;
            int nLevels = 0;
            const CBranch* pBranch = pParent;

            while (pBranch && nLevels < m_pLeafInfo->m_nDimmingDepth - 1)
            {
                pBranch = pBranch->GetParent( );
                nLevels++;
            }
            if (pBranch)
            {
               cBase = pBranch->m_pVertices[0].m_cPos;
            }
            cParentDir = acVertices[1].m_cPos - cBase;
        }

    }
    cParentDir.Normalize( );

    // attempt to grow leaf based on this bud's parameters
    MakeLeaf(acVertices[c_nBudVertexCount - 1].m_cPos,
             fPercentOfParent,
             this,
             cNormal,
             cParentDir,
             fWindWeight1,
             fWindWeight2,
             nWindGroup1,
             nWindGroup2,
             vLeaves);
}


///////////////////////////////////////////////////////////////////////
//  CBranch::ComputeLod definition
//
//  Each of the discrete branch LOD's share the same vertex table.  A lower
//  LOD of the branch structure simply drops some of the lower-volume branches.
//  ComputeLod() assigns the new indexes for the requested LOD.

void CBranch::ComputeLod(int nDiscreteLodLevel, CIndexedGeometry* pGeometry) const
{
    // new cross section count
    if (m_usCrossSectionSegments >= 2 && m_nStartingOffset != -1)
    {
        st_vector_int vIndices;
        vIndices.reserve(200);

        int nRunningCount = 0;
        int i = 0;
        for (i = 0; i < m_nNumVertices - 1; ++i)
        {
            st_assert(m_pVertices);

            int nThisCrossCount = m_pVertices[i].m_nCrossSegments + 1;
            int nNextCrossCount = m_pVertices[i + 1].m_nCrossSegments + 1;
            int nMaxCrossCount = st_max(nThisCrossCount, nNextCrossCount);
            for (int j = 0; j < nMaxCrossCount; ++j)
            {
                float fThisProgress = (j / static_cast<float>(nMaxCrossCount - 1.0f));
                int nThisIndex = static_cast<int>((static_cast<float>(nThisCrossCount - 1) * fThisProgress) + 0.5f);
                int nNextIndex = static_cast<int>((static_cast<float>(nNextCrossCount - 1) * fThisProgress) + 0.5f);

                vIndices.push_back(static_cast<unsigned short>(m_nStartingOffset + nRunningCount + nThisCrossCount + nNextIndex));
                vIndices.push_back(static_cast<unsigned short>(m_nStartingOffset + nRunningCount + nThisIndex));
            }

            int nRepeat = vIndices[vIndices.size( ) - 2];
            // repeat this vertex so we can combine the multiple segments of one branch
            vIndices.push_back(nRepeat);
            // repeat it again to counteract the clockwise vertex ordering that would result otherwise
            vIndices.push_back(nRepeat);

            nRunningCount += nThisCrossCount;
        }
        int nNumIndices = int(vIndices.size( ));
        pGeometry->AddStrip(nDiscreteLodLevel, &vIndices[0], nNumIndices);
        pGeometry->AdvanceStripCounter( );

        // set LOD fade hints
        const float c_fLodFadeHint = (nDiscreteLodLevel == pGeometry->GetNumLodLevels( ) - 1) ? -1.0f : 1.0f - float(nDiscreteLodLevel + 1) / pGeometry->GetNumLodLevels( );

        for (i = 0; i < nNumIndices; ++i)
            pGeometry->SetLodFadeHint(vIndices[i], c_fLodFadeHint);
    }
}


///////////////////////////////////////////////////////////////////////
//  CBranch::BuildBranchVector definition
//
//  This is a convenience function that builds a linear vector from the
//  branch tree structure.

void CBranch::BuildBranchVector(st_vector_branches& vAllBranches)
{
    vAllBranches.push_back(this);

    for (unsigned int i = 0; i < m_vChildren.size( ); ++i)
        m_vChildren[i].m_pBranch->BuildBranchVector(vAllBranches);
}


///////////////////////////////////////////////////////////////////////
//  class CBranchVolumeSorter definition
//
//  Function object used to sort the branches by volume.

class CBranchVolumeSorter
{
public:
    bool operator()(const CBranch* pLeft, const CBranch* pRight)
    {
        return (pLeft->GetFuzzyVolume( ) > pRight->GetFuzzyVolume( ));
    }
};


///////////////////////////////////////////////////////////////////////
//  CBranch::SortBranchVector definition

void CBranch::SortBranchVector(st_vector_branches& vAllBranches)
{
    std::sort(vAllBranches.begin( ), vAllBranches.end( ), CBranchVolumeSorter( ));
}


///////////////////////////////////////////////////////////////////////
//  CBranch::Parse definition

void CBranch::Parse(CTreeFileAccess& cFile)
{
    ClearBranchInfo( );

    int nSize = cFile.ParseToken( );
    for (int i = 0; i < nSize; ++i)
    {
        SIdvBranchInfo sInfo;
        int nToken = cFile.ParseToken( );
        if (nToken != File_BeginBranchLevel)
            throw(IdvFileError("malformed branch data"));

        nToken = cFile.ParseToken( );
        do
        {
            switch (nToken)
            {
                case File_Branch_Disturbance:
                    cFile.ParseBranchParameter(sInfo.GetDisturbance( ));
                    break;
                case File_Branch_Gravity:
                    cFile.ParseBranchParameter(sInfo.GetGravity( ));
                    break;
                case File_Branch_Flexibility:
                    cFile.ParseBranchParameter(sInfo.GetFlexibility( ));
                    break;
                case File_Branch_FlexibilityScale:
                    cFile.ParseBranchParameter(sInfo.GetFlexibilityScale( ));
                    break;
                case File_Branch_Length:
                    cFile.ParseBranchParameter(sInfo.GetLength( ));
                    break;
                case File_Branch_Radius:
                    cFile.ParseBranchParameter(sInfo.GetRadius( ));
                    break;
                case File_Branch_RadiusScale:
                    cFile.ParseBranchParameter(sInfo.GetRadiusScale( ));
                    break;
                case File_Branch_StartAngle:
                    cFile.ParseBranchParameter(sInfo.GetStartAngle( ));
                    break;
                case File_Branch_CrossSectionSegments:
                    sInfo.m_nCrossSectionSegments = cFile.ParseInt( );
                    break;
                case File_Branch_Segments:
                    sInfo.m_nSegments = cFile.ParseInt( );
                    break;
                case File_Branch_FirstBranch:
                    sInfo.m_fFirstBranch = cFile.ParseFloat( );
                    break;
                case File_Branch_LastBranch:
                    sInfo.m_fLastBranch = cFile.ParseFloat( );
                    break;
                case File_Branch_Frequency:
                    sInfo.m_fFrequency = cFile.ParseFloat( );
                    break;
                case File_Branch_STile:
                    sInfo.m_asTexCoordControls[CSpeedTreeRT::TL_DIFFUSE].m_fSTile = cFile.ParseFloat( );
                    break;
                case File_Branch_TTile:
                    sInfo.m_asTexCoordControls[CSpeedTreeRT::TL_DIFFUSE].m_fTTile = cFile.ParseFloat( );
                    break;
                case File_Branch_STileAbsolute:
                    sInfo.m_asTexCoordControls[CSpeedTreeRT::TL_DIFFUSE].m_bSTileAbsolute = cFile.ParseBool( );
                    break;
                case File_Branch_TTileAbsolute:
                    sInfo.m_asTexCoordControls[CSpeedTreeRT::TL_DIFFUSE].m_bTTileAbsolute = cFile.ParseBool( );
                    break;
                default:
                    throw(IdvFileError("malformed general branch information"));
            }
            nToken = cFile.ParseToken( );
        } while (nToken != File_EndBranchLevel);
    }

    int nToken = cFile.ParseToken( );
    if (nToken != File_EndBranchInfo)
        throw(IdvFileError("malformed branch data"));
}


///////////////////////////////////////////////////////////////////////
//  CBranch::BuildBlossomVectors definition
//
//  Each leaf texture's index is added either to m_vBlossoms or
//  m_vNonBlossoms.  Each texture index also has a mirror that is the
//  same texture with the s coordinates mirrored.

void CBranch::BuildBlossomVectors(void)
{
    // dealloc vectors
    st_vector_int( ).swap(m_vBlossoms);
    st_vector_int( ).swap(m_vNonBlossoms);

    for (unsigned int i = 0; i < m_pLeafInfo->m_vLeafTextures.size( ); ++i)
        if (m_pLeafInfo->m_vLeafTextures[i].m_bBlossom)
        {
            m_vBlossoms.push_back(i * 2);
            m_vBlossoms.push_back(i * 2 + 1);
        }
        else
        {
            m_vNonBlossoms.push_back(i * 2);
            m_vNonBlossoms.push_back(i * 2 + 1);
        }
}


///////////////////////////////////////////////////////////////////////
//  CBranch::IsBlossom definition
//
//  Uses the parameters specified in SpeedTreeCAD to determine if position
//  fPercentOfParent on branch pParent will result in a blossom.

bool CBranch::IsBlossom(const CBranch* pParent, float fPercentOfParent) const
{
    bool bIsBlossom = false;

    float fTestPercent = fPercentOfParent;
    if (m_pLeafInfo->m_nBlossomLevel != 0)
    {
        // blossoms are created at the branch level m_pLeafInfo->m_nBlossomLevel - pBranch
        // is advanced to the correct level
        int nLevels = 1; // "this" is already pointing to the parent of the bud so start one level up

        const CBranch* pBranch = pParent;
        while (pBranch &&
               pBranch->GetParent( ) &&
               nLevels < m_pLeafInfo->m_nBlossomLevel)
        {
            pBranch = pBranch->GetParent( );
            nLevels++;
        }

        if (pBranch)
            fTestPercent = pBranch->GetPercentOfParent( );
    }

    // random numbers are used because not every leaf should be a blossom - the weighting
    // of which is specified by the user in SpeedTreeCAD
    if (m_pLeafInfo->m_fBlossomDistance < 0.0f)
    {
        if (fTestPercent < -m_pLeafInfo->m_fBlossomDistance &&
            m_cRandom.GetUniform(0.0f, 1.0f) <= m_pLeafInfo->m_fBlossomWeighting)
            bIsBlossom = true;
    }
    else
    {
        if (fTestPercent > m_pLeafInfo->m_fBlossomDistance &&
            m_cRandom.GetUniform(0.0f, 1.0f) <= m_pLeafInfo->m_fBlossomWeighting)
            bIsBlossom = true;
    }


    return bIsBlossom;
}


///////////////////////////////////////////////////////////////////////
//  CBranch::MakeLeaf definition

void CBranch::MakeLeaf(const stVec3& cPos,
                       float fPercent,
                       const CBranch* pParent,
                       const stVec3& cNormal,
                       const stVec3& cParentDir,
                       float fWindWeight1,
                       float fWindWeight2,
                       int nWindGroup1,
                       int nWindGroup2,
                       st_vector_leaves& vLeaves) const
{
    // are there any leaf textures to use?
    if (!m_pLeafInfo->m_vLeafTextures.empty( ))
    {
        int nTextureIndex = 0;

        // determine if this leaf will be a blossom
        bool bIsBlossom = m_vBlossoms.empty( ) ? false : IsBlossom(pParent, fPercent);
        if (bIsBlossom)
        {
            // randomly pick which blossom it will be based on the m_vBlossoms vector
            unsigned int nIndex = 0;
            if (m_vBlossoms.size( ) > 1)
                nIndex = static_cast<unsigned int>(m_cRandom.GetUniform(0.0, 100000.0)) % m_vBlossoms.size( );
            nTextureIndex = m_vBlossoms[nIndex];
        }
        else
        {
            // randomly pick which non-blossom texture will be used
            unsigned int nIndex = 0;
            if (m_vNonBlossoms.size( ) > 1)
            {
                float fRandom = m_cRandom.GetUniform(0.0, 1000000.0);
                nIndex = static_cast<unsigned int>(fRandom) % m_vNonBlossoms.size( );
                nTextureIndex = m_vNonBlossoms[nIndex];
            }
            else if (m_vNonBlossoms.empty( ))
                return;
            else
                nTextureIndex = m_vNonBlossoms[0];
        }

        // test to see if the leaf will be too crowded
        bool bValid = true;
        switch (m_pLeafInfo->m_eCollisionType)
        {
        case SIdvLeafInfo::NONE:    // no leaf will ever be crowded
            break;
        case SIdvLeafInfo::BRANCH:  // test only against leaves that are spawned from a common branch
            bValid = RoomForLeaf(cPos, nTextureIndex / 2, m_vLocalLeaves);
            break;
        case SIdvLeafInfo::TREE:    // test against all leaves in the tree
            bValid = RoomForLeaf(cPos, nTextureIndex / 2, vLeaves);
            break;
        default:
            // should never arrive here
            break;
        }

        if (bValid)
        {
            const SIdvLeafTexture& sTexture = m_pLeafInfo->m_vLeafTextures[nTextureIndex / 2];

            float fColorScale = 1.0f;

            // if dimming is enabled, modify the color scalar to darken it an appropriate amount.
            float fDimPercent = fPercent;
            if (m_pLeafInfo->m_bDimming)
            {
                if (m_pLeafInfo->m_nDimmingDepth != 0)
                {
                    int nLevels = 0;
                    const CBranch* pBranch = pParent;

                    while (pBranch && nLevels < m_pLeafInfo->m_nDimmingDepth)
                    {
                        fDimPercent *= pBranch->GetPercentOfParent( );
                        pBranch = pBranch->GetParent( );
                        nLevels++;
                    }
                }

                fColorScale = VecInterpolate(fDimPercent, 1.0f, 1.0f - m_pLeafInfo->m_fDimmingScalar);
            }

            // actually make the leaf now
            //lint -esym(429,pLeaf)  { pLeaf is not freed or returned from MakeLeaf(), but it is stored in vLeaves for later deletion }
            CBillboardLeaf cLeaf(cPos, short(255.0f * fColorScale), static_cast<int>(m_cRandom.GetUniform(0.0, 10000.0)) % m_pLeafInfo->m_nNumRockingGroups, fWindWeight1, fWindWeight2, nWindGroup1, nWindGroup2);

            cLeaf.SetTextureIndex(nTextureIndex);

            // let the color variance perturb the leaf normal (for dynamic lighting)
            stVec3 cAdjustedNormal;
            if (cNormal.Magnitude( ) < 0.0001)
                cAdjustedNormal = cParentDir;
            else
                cAdjustedNormal = VecInterpolate(cParentDir, cNormal, sTexture.m_fColorVariance);
            cAdjustedNormal.Normalize( );

            stVec3 cDown(0.0f, 0.0f, 1.0f);
            stVec3 cNormalBeforeHang = cAdjustedNormal;
            if (m_pLightingEngine->GetLeafLightingMethod( ) == CSpeedTreeRT::LIGHT_DYNAMIC)
            {
                if (sTexture.m_bUseMeshes)
                {
                    cAdjustedNormal = VecInterpolate(cAdjustedNormal, cDown, sTexture.m_fHang);
                    cAdjustedNormal.Normalize( );
                }
            }

            // tangent
            stVec3 cTangent = cAdjustedNormal * cDown;
            cTangent.Normalize( );

            // binormal
            stVec3 cBinormal = cTangent * cAdjustedNormal;
            cBinormal.Normalize( );

            if (cTangent.Magnitude( ) < 0.5f)
            {
                // magnitude should be 1, so cAdjustedNormal must be very close to the up vector
                cBinormal = -cNormalBeforeHang;
                cBinormal[2] = 0.0f;
                cBinormal.Normalize( );
                cTangent = cAdjustedNormal * cBinormal;
                cTangent.Normalize( );
            }

            // rotate the meshes if needed
            if (sTexture.m_bUseMeshes && sTexture.m_fRotate != 0.0f)
            {
                stRotTransform matTransform;
                matTransform.m_afData[0][0] = cTangent[0];
                matTransform.m_afData[0][1] = cTangent[1];
                matTransform.m_afData[0][2] = cTangent[2];
                matTransform.m_afData[1][0] = cBinormal[0];
                matTransform.m_afData[1][1] = cBinormal[1];
                matTransform.m_afData[1][2] = cBinormal[2];
                matTransform.m_afData[2][0] = cAdjustedNormal[0];
                matTransform.m_afData[2][1] = cAdjustedNormal[1];
                matTransform.m_afData[2][2] = cAdjustedNormal[2];

                matTransform.RotateZ(m_afRandomBank[m_nMeshRotateIndex++ % c_nBankSize] * 2.0f * sTexture.m_fRotate - sTexture.m_fRotate);

                cTangent[0] = matTransform.m_afData[0][0];
                cTangent[1] = matTransform.m_afData[0][1];              
                cTangent[2] = matTransform.m_afData[0][2];
                cBinormal[0] = matTransform.m_afData[1][0];
                cBinormal[1] = matTransform.m_afData[1][1];
                cBinormal[2] = matTransform.m_afData[1][2];
                cAdjustedNormal[0] = matTransform.m_afData[2][0];
                cAdjustedNormal[1] = matTransform.m_afData[2][1];
                cAdjustedNormal[2] = matTransform.m_afData[2][2];
            }


            // check for smooth normals
            if (m_pLeafInfo->m_bSmoothingEnabled && !sTexture.m_bUseMeshes)
            {
                stVec3 acSmoothedNormals[4], acSmoothedTangents[4], acSmoothedBinormals[4];
                float fAdjustmentAngle = m_pLeafInfo->m_fSmoothingAngle;

                stVec3 vecNormalAxis(0.0f, 0.0f, 1.0f);
                stRotTransform matTransform;
                matTransform.m_afData[0][0] = cTangent[0];
                matTransform.m_afData[0][1] = cTangent[1];
                matTransform.m_afData[0][2] = cTangent[2];
                matTransform.m_afData[1][0] = cBinormal[0];
                matTransform.m_afData[1][1] = cBinormal[1];
                matTransform.m_afData[1][2] = cBinormal[2];
                matTransform.m_afData[2][0] = cAdjustedNormal[0];
                matTransform.m_afData[2][1] = cAdjustedNormal[1];
                matTransform.m_afData[2][2] = cAdjustedNormal[2];

                stRotTransform matTemp = matTransform;

                float fVerticalPercent = fAdjustmentAngle / 100.0f;
                // rotate each normal
                matTemp.RotateY(-fAdjustmentAngle);
                acSmoothedNormals[0] = vecNormalAxis * matTemp;
                acSmoothedNormals[0] = VecInterpolate(acSmoothedNormals[0], g_cUp, fVerticalPercent);
                acSmoothedNormals[0].Normalize( );
                acSmoothedTangents[0] = acSmoothedNormals[0] * cDown;
                acSmoothedTangents[0].Normalize( );
                acSmoothedBinormals[0] = acSmoothedTangents[0] * acSmoothedNormals[0];
                acSmoothedBinormals[0].Normalize( );

                matTemp = matTransform;
                matTemp.RotateY(fAdjustmentAngle);
                acSmoothedNormals[1] = vecNormalAxis * matTemp;
                acSmoothedNormals[1] = VecInterpolate(acSmoothedNormals[1], g_cUp, fVerticalPercent);
                acSmoothedNormals[1].Normalize( );
                acSmoothedTangents[1] = acSmoothedNormals[1] * cDown;
                acSmoothedTangents[1].Normalize( );
                acSmoothedBinormals[1] = acSmoothedTangents[1] * acSmoothedNormals[1];
                acSmoothedBinormals[1].Normalize( );

                matTemp = matTransform;
                matTemp.RotateY(fAdjustmentAngle);
                acSmoothedNormals[2] = vecNormalAxis * matTemp;
                acSmoothedNormals[2] = VecInterpolate(acSmoothedNormals[2], g_cDown, fVerticalPercent);
                acSmoothedNormals[2].Normalize( );
                acSmoothedTangents[2] = acSmoothedNormals[2] * cDown;
                acSmoothedTangents[2].Normalize( );
                acSmoothedBinormals[2] = acSmoothedTangents[2] * acSmoothedNormals[2];
                acSmoothedBinormals[2].Normalize( );

                matTemp = matTransform;
                matTemp.RotateY(-fAdjustmentAngle);
                acSmoothedNormals[3] = vecNormalAxis * matTemp;
                acSmoothedNormals[3] = VecInterpolate(acSmoothedNormals[3], g_cDown, fVerticalPercent);
                acSmoothedNormals[3].Normalize( );
                acSmoothedTangents[3] = acSmoothedNormals[3] * cDown;
                acSmoothedTangents[3].Normalize( );
                acSmoothedBinormals[3] = acSmoothedTangents[3] * acSmoothedNormals[3];
                acSmoothedBinormals[3].Normalize( );

                for (int i = 0; i < 4; ++i)
                {
                    cLeaf.SetNormal(acSmoothedNormals[i], i);
                    cLeaf.SetTangent(acSmoothedTangents[i], i);
                    cLeaf.SetBinormal(acSmoothedBinormals[i], i);
                }
            }
            else
            {
                for (int i = 0; i < 4; ++i)
                {
                    cLeaf.SetNormal(cAdjustedNormal, i);
                    cLeaf.SetTangent(cTangent, i);
                    cLeaf.SetBinormal(cBinormal, i);
                }
            }

            // directly adjust the color (for static lighting)
            float fOffset = m_cRandom.GetUniform(-sTexture.m_fColorVariance, sTexture.m_fColorVariance);
            stVec3 cColor(sTexture.m_cColor[0] + fOffset, sTexture.m_cColor[1] + fOffset, sTexture.m_cColor[2] + fOffset);
            cLeaf.SetColor(cColor, 0);
            cLeaf.SetColor(cColor, 1);
            cLeaf.SetColor(cColor, 2);
            cLeaf.SetColor(cColor, 3);

            // save leaf
            vLeaves.push_back(cLeaf);
            if (m_pLeafInfo->m_eCollisionType == SIdvLeafInfo::BRANCH)
                m_vLocalLeaves.push_back(cLeaf);
        }
    }
}


///////////////////////////////////////////////////////////////////////
//  CBranch::RoomForLeaf definition
//
//  This function checks against the leaves in the vLeaves vector to see if
//  the leaf at position cPos will be too crowded.

bool CBranch::RoomForLeaf(const stVec3& cPos, int nTextureIndex, const st_vector_leaves& vLeaves) const
{
    bool bIsRoom = true;

    if (!m_pLeafInfo->m_vLeafTextures.empty( ))
    {
        // get the larger of the width and height of the leaf
        const SIdvLeafTexture& sTexture = m_pLeafInfo->m_vLeafTextures[nTextureIndex];
        float fLargerSide = sTexture.m_cSizeUsed[0] > sTexture.m_cSizeUsed[1] ? sTexture.m_cSizeUsed[0] : sTexture.m_cSizeUsed[1];

        // adjust the size by the spacing tolerance (allows adjustment of leaf density)
        fLargerSide *= m_pLeafInfo->m_fSpacingTolerance;
        if (!vLeaves.empty( ))
        {
            int nNumLeaves = int(vLeaves.size( ));
            const CBillboardLeaf* pLeaf = &vLeaves[nNumLeaves - 1];
            while (--nNumLeaves > -1)
            {
                const stVec3& cOther = pLeaf->GetPosition( );
                if ( ((cPos[0] < cOther[0] + fLargerSide) && (cPos[0] > cOther[0] - fLargerSide)) &&
                     ((cPos[1] < cOther[1] + fLargerSide) && (cPos[1] > cOther[1] - fLargerSide)) &&
                     ((cPos[2] < cOther[2] + fLargerSide) && (cPos[2] > cOther[2] - fLargerSide)) )
                {
                    bIsRoom = false;
                    break;
                }
                --pLeaf;
            }
        }
    }
    else
        bIsRoom = false;

    return bIsRoom;
}


///////////////////////////////////////////////////////////////////////
//  CBranch::BuildCrossSection definition
//
//  This function "skins" the branch at pVertex.

void CBranch::BuildCrossSection(const SIdvBranchVertex* pVertex,
                                float fBranchProgress,
                                int nSegments,
                                CIndexedGeometry* pBranchGeometry,
                                const SIdvBranchInfo& sInfo,
                                float fWindWeight1,
                                float fWindWeight2,
                                int nWindGroup1,
                                int nWindGroup2,
                                float fTileOffset,
                                const SIdvBranchInfo* pInfo,
                                float fMaxPossibleRadius,
                                float fSize,
                                float fBaseRadius,
                                float fLength,
                                bool bNegateTwist) const
{
    float fProgress = 0.0f;
    const float fProgressInc = 1.0f / nSegments;
    float afFirstPos[3];
    memcpy(afFirstPos, pVertex->m_cPos, 3 * sizeof(float));

    // storage for first vertex (so it can be repeated for the last)
    const_cast<SIdvBranchVertex*>(pVertex)->m_nCrossSegments = nSegments;
    float fRoughnessFactor = const_cast<SIdvBranchInfo*>(pInfo)->GetRoughnessProfile( )->EvaluateWithoutVariance(fBranchProgress);

    for (int i = 0; i <= nSegments; ++i)
    {
        float fAngle = c_fTwoPi * fProgress;

        // compute and add texcoords
        int j = 0;
        for (j = 0; j < CSpeedTreeRT::TL_SHADOW; ++j)
        {
            float afTexCoords[2];
            ComputeTexCoords(afTexCoords[0], afTexCoords[1], sInfo.m_asTexCoordControls[j], fProgress, fBranchProgress, fTileOffset, fBaseRadius, fLength / fSize, bNegateTwist);
            pBranchGeometry->AddVertexTexCoord(static_cast<CSpeedTreeRT::ETextureLayers>(j), afTexCoords);
        }

        // compute and add normal
        float fCosine = cosf(fAngle);
        float fSine = sinf(fAngle);
        stVec3 cNormal;
        cNormal[0] = fCosine * pVertex->m_cTrans.m_afData[1][0] + fSine * pVertex->m_cTrans.m_afData[2][0];
        cNormal[1] = fCosine * pVertex->m_cTrans.m_afData[1][1] + fSine * pVertex->m_cTrans.m_afData[2][1];
        cNormal[2] = fCosine * pVertex->m_cTrans.m_afData[1][2] + fSine * pVertex->m_cTrans.m_afData[2][2];

        // bump mapping (compute tangent and binormal)
        fCosine = cosf(fAngle + c_fHalfPi);
        fSine = sinf(fAngle + c_fHalfPi);
        stVec3 cTangent;
        cTangent[0] = fCosine * pVertex->m_cTrans.m_afData[1][0] + fSine * pVertex->m_cTrans.m_afData[2][0];
        cTangent[1] = fCosine * pVertex->m_cTrans.m_afData[1][1] + fSine * pVertex->m_cTrans.m_afData[2][1];
        cTangent[2] = fCosine * pVertex->m_cTrans.m_afData[1][2] + fSine * pVertex->m_cTrans.m_afData[2][2];
        pBranchGeometry->AddVertexTangent(cTangent);
        pBranchGeometry->AddVertexBinormal(cNormal * cTangent);

        if (i != nSegments)
        {
            // adjust for flares
            float fFlareAdjust = 1.0f, fFlareAdditions = 0.0f;

            for (j = 0; j < int(m_vFlares.size( )); ++j)
                fFlareAdditions += m_vFlares[j].Distance(fAngle, fBranchProgress);

            fFlareAdjust += fFlareAdditions;

            // position (uses previous normal calculation)
            float afOriginalPos[3];
            float fRadius = pVertex->m_fRadius;

            float fRadiusFactor = fRadius / fMaxPossibleRadius;

            fRadius += fRadiusFactor * pInfo->m_fRoughness * fSize * fRoughnessFactor *
                            sinf(pInfo->m_fRoughnessVerticalFrequency * fBranchProgress) * sinf(pInfo->m_fRoughnessHorizontalFrequency * fBranchProgress) *
                            cosf(pInfo->m_fRoughnessHorizontalFrequency * fAngle);
            if (fRadius > fMaxPossibleRadius)
                fRadius = fMaxPossibleRadius;

            float fTemp = ((m_afRandomBank[m_nRandomIndex % c_nBankSize] - 0.5f) * 2.0f) * pInfo->m_fRandomRoughness * fSize * fRadiusFactor * fRoughnessFactor;
            fRadius += fTemp;
            ++m_nRandomIndex;

            //  const_cast<SIdvBranchVertex*>(pVertex)->m_fRadius = fRadius;

            afOriginalPos[0] = pVertex->m_cPos.m_afData[0] + (cNormal[0] * fRadius);
            afOriginalPos[1] = pVertex->m_cPos.m_afData[1] + (cNormal[1] * fRadius);
            afOriginalPos[2] = pVertex->m_cPos.m_afData[2] + (cNormal[2] * fRadius);

            if (fFlareAdjust != 1.0f)
            {
                float afPos[3];
                afPos[0] = pVertex->m_cPos.m_afData[0] + (cNormal[0] * fRadius * fFlareAdjust);
                afPos[1] = pVertex->m_cPos.m_afData[1] + (cNormal[1] * fRadius * fFlareAdjust);
                afPos[2] = pVertex->m_cPos.m_afData[2] + (cNormal[2] * fRadius * fFlareAdjust);

                stVec3 cNewNormal(afPos[0] - afOriginalPos[0], afPos[1] - afOriginalPos[1], afPos[2] - afOriginalPos[2]);
                cNewNormal.Normalize( );
                cNormal = cNewNormal;

                memcpy(afOriginalPos, afPos, 3 * sizeof(float));
            }

            pBranchGeometry->AddVertexCoord(afOriginalPos);
            if (i == 0)
            {
                afFirstPos[0] = afOriginalPos[0];
                afFirstPos[1] = afOriginalPos[1];
                afFirstPos[2] = afOriginalPos[2];
            }
        }
        else
        {
            pBranchGeometry->AddVertexCoord(afFirstPos);
        }

        // set wind information
        if (pBranchGeometry->IsVertexWeightingEnabled( ))
        {
            pBranchGeometry->AddVertexWind1(fWindWeight1, static_cast<unsigned char>(nWindGroup1));
            pBranchGeometry->AddVertexWind2(fWindWeight2, static_cast<unsigned char>(nWindGroup2));
        }

        pBranchGeometry->AdvanceVertexCounter( );
        fProgress += fProgressInc;
    }
}

///////////////////////////////////////////////////////////////////////
//  CBranch::ComputeTexCoords definition

void CBranch::ComputeTexCoords(float& fS,
                               float& fT,
                               const SIdvBranchInfo::STexCoordControls& sControls,
                               float fRadialProgress,
                               float fBranchProgress,
                               float fTileOffset,
                               float fBaseRadius,
                               float fLength,
                               bool bNegateTwist) const
{
    const stVec3 cTile(sControls.m_bSTileAbsolute ? sControls.m_fSTile : sControls.m_fSTile * fBaseRadius * c_fTwoPi,
                       sControls.m_bTTileAbsolute ? sControls.m_fTTile : sControls.m_fTTile * fLength,
                       bNegateTwist ? -sControls.m_fTwist : sControls.m_fTwist);

    if (sControls.m_bClampSCoord)
    {
        float fLeft = sControls.m_afClampValues[SIdvBranchInfo::STexCoordControls::LEFT];
        float fRight = sControls.m_afClampValues[SIdvBranchInfo::STexCoordControls::RIGHT];

        float fStartAngle = sControls.m_fSCoordOffset * 90.0f;
        if (fStartAngle < 0.0f)
            fStartAngle = 0.0f;

        while (fStartAngle > 360.0f)
            fStartAngle -= 360.0f;

        while (fStartAngle < 0.0f)
            fStartAngle += 360.0f;

        float fSweep;
        if (cTile[0] == 0.0f)
            fSweep = 0.0f;
        else
            fSweep = 360.0f / (cTile[0] != 0.0f ? cTile[0] : 1.0f);

        float fEndAngle = fStartAngle + fSweep;
        float fThisAngle = fRadialProgress * 360.0f;

        if (fEndAngle > 360.0f)
        {
            fEndAngle = 360.0f;
            fStartAngle = fEndAngle - fSweep;
        }

        if (fThisAngle < fStartAngle)
            fThisAngle = fStartAngle;
        else if (fThisAngle > fEndAngle)
            fThisAngle = fEndAngle;

        fS = VecInterpolate(fLeft, fRight, (fThisAngle - fStartAngle) / fSweep);
    }
    else
    {
        fS = fRadialProgress * cTile[0] + sControls.m_fSCoordOffset;
        fS += cTile[2] * fBranchProgress;
    }

    fT = (fBranchProgress + (sControls.m_bRandomTCoordOffset ? fTileOffset : 0.0f)) * cTile[1] - sControls.m_fTCoordOffset;

    if (sControls.m_bClampTCoord)
    {
        if (fT < 0.0f)
            fT = 0.0f;

        if (fT > 1.0f)
            fT = 1.0f;

        fT = VecInterpolate(sControls.m_afClampValues[SIdvBranchInfo::STexCoordControls::BOTTOM],
                            sControls.m_afClampValues[SIdvBranchInfo::STexCoordControls::TOP],
                            fT);
    }
}


///////////////////////////////////////////////////////////////////////
//  CBranch::ComputeVertexWeights definition

void CBranch::ComputeVertexWeights(float& fWeight1, float& fWeight2, int nLevel, float fProgress, float fFlexibility)
{
    if (nLevel < m_nWeightLevel)
    {
        fWeight1 = 1.0f;
        fWeight2 = 1.0f;
    }
    else if (nLevel == m_nWeightLevel)
    {
        fWeight1 = 1.0f - (fProgress * fFlexibility);
        fWeight2 = 1.0f;
    }
    else if (nLevel == m_nWeightLevel + 1)
    {
        // not propagating level 1
        fWeight2 = 1.0f - (fProgress * fFlexibility);
    }
    else if (m_bPropagateFlexibility)
    {
        // not propagating level 1
        fWeight2 = VecInterpolate(fWeight2, 0.0f, fFlexibility);
    }
}


///////////////////////////////////////////////////////////////////////
//  CBranch::ComputeVolume definition
//
//  To compute the volume of the branch, we will add the volumes
//  of each segment.  Each segment is rougly a cone with a non-zero
//  radius at the top.  The volume formula for this type of cone is:
//
//      Volume = PI * (R2^2 + R1 * R2 + R1^2) * Height / 3
//      Cross Sectional Area = Height * (R1 + R2)

void CBranch::ComputeVolume(void)
{
    m_fVolume = 0.0f;
    if (m_pVertices && m_nNumVertices > 1)
    {
        for (int i = 0; i < m_nNumVertices - 1; ++i)
        {
            SIdvBranchVertex* pVertex1 = m_pVertices + i;
            SIdvBranchVertex* pVertex2 = m_pVertices + i + 1;

            float fDistance = pVertex1->m_cPos.Distance(pVertex2->m_cPos);
            m_fVolume += fDistance * (pVertex1->m_fRadius + pVertex2->m_fRadius);
        }
    }
}


///////////////////////////////////////////////////////////////////////
//  CBranch::FillBranch definition
//
//  FillBranch() determines the vertex of the parent just before the child
//  is grown - stored as sBranch.m_nPreVertexIndex.  sBranch.m_fPercent
//  is the percentage distance from this vertex to the next where the child
//  is created.

void CBranch::FillBranch(SIdvBranch& sBranch, float fChildGrowthPos) const
{
    if (m_pVertices && m_nNumVertices >= 2)
    {
        sBranch.m_nPreVertexIndex = 0;
        for (int i = 1; i < m_nNumVertices; ++i)
        {
            if (fChildGrowthPos < m_pVertices[i].m_fRunningLength)
            {
                sBranch.m_nPreVertexIndex = i - 1;
                break;
            }
        }

        sBranch.m_fPercent = (fChildGrowthPos - m_pVertices[sBranch.m_nPreVertexIndex].m_fRunningLength) /
                            (m_pVertices[sBranch.m_nPreVertexIndex + 1].m_fRunningLength - m_pVertices[sBranch.m_nPreVertexIndex].m_fRunningLength);
    }
}


///////////////////////////////////////////////////////////////////////
//  CBranch::ComputeForkPercent definition

float CBranch::ComputeForkPercent(float fPercent, float fLength, float fForkAngle, int nForkLimit) const
{
    float fChildGrowthPos = fPercent * fLength;

    float fNewPercent = -1.0f;  // if we can't find a fork spot, discard this branch

    if (m_pVertices && m_nNumVertices >= 2)
    {
        int nPreVertexIndex = 0;
        for (int i = 1; i < m_nNumVertices; ++i)
        {
            if (fChildGrowthPos < m_pVertices[i].m_fRunningLength)
            {
                nPreVertexIndex = i - 1;
                break;
            }
        }

        if (fChildGrowthPos - m_pVertices[nPreVertexIndex].m_fRunningLength > m_pVertices[nPreVertexIndex + 1].m_fRunningLength - fChildGrowthPos)
            ++nPreVertexIndex;

        if (nPreVertexIndex < m_nNumVertices - 1)
        {
            if (m_pVertices[nPreVertexIndex].m_nForkCount < nForkLimit)
            {
                ++m_pVertices[nPreVertexIndex].m_nForkCount;
                float fAngle = 180.0f;
                if (nPreVertexIndex > 0)
                    fAngle = VecRad2Deg(m_pVertices[nPreVertexIndex].m_cDirection.AngleBetween(m_pVertices[nPreVertexIndex - 1].m_cDirection));

                if (fAngle >= fForkAngle)
                    fNewPercent = m_pVertices[nPreVertexIndex].m_fRunningLength / fLength;
            }
        }
    }

    return fNewPercent;
}


///////////////////////////////////////////////////////////////////////
//  CBranch::ComputeFlareEntries definition
//
//  This function uses the system random numbers in an attempt to preserve
//  seed compatibility and not disturb EstimateCompute( ).

void CBranch::ComputeFlareEntries(const SIdvBranchInfo& sInfo)
{
    if (sInfo.m_nNumFlares != 0)
    {
        float fInitialAngle = GetRandom(0.0f, c_fTwoPi);
        float fBalanceAngle = c_fTwoPi / (sInfo.m_nNumFlares);

        m_vFlares.resize(sInfo.m_nNumFlares);
        for (int i = 0; i < sInfo.m_nNumFlares; ++i)
        {
            SIdvBranchFlare& sFlare = m_vFlares[i];

            // angle
            sFlare.m_fAngle = fInitialAngle + (i * GetRandom(sInfo.m_fFlareBalance * fBalanceAngle, fBalanceAngle));
            if (sFlare.m_fAngle > c_fTwoPi)
                sFlare.m_fAngle -= c_fTwoPi;

            // focus
            sFlare.m_fRadialExponent = sInfo.m_fRadialExponent;
            sFlare.m_fLengthExponent = sInfo.m_fLengthExponent;

            // angle influence
            sFlare.m_fRadialInfluence = VecDeg2Rad(GetRandom(sInfo.m_fRadialInfluence - sInfo.m_fRadialInfluenceVariance, sInfo.m_fRadialInfluence + sInfo.m_fRadialInfluenceVariance));

            // distance influence
            sFlare.m_fLengthInfluence = GetRandom(sInfo.m_fLengthDistance - sInfo.m_fLengthVariance, sInfo.m_fLengthDistance + sInfo.m_fLengthVariance);

            // distance
            sFlare.m_fDistance = GetRandom(sInfo.m_fRadialDistance - sInfo.m_fRadialVariance, sInfo.m_fRadialDistance + sInfo.m_fRadialVariance);
        }
    }
}


///////////////////////////////////////////////////////////////////////
//  CBranch::ComputeBranchNormals definition

void CBranch::ComputeBranchNormals(CIndexedGeometry* pBranchGeometry, const stBezierSpline* pLightSeamProfile, float fLightSeamBias)
{
    st_assert(m_pVertices);

    int nRunningCount = 0;
    for (int i = 0; i < m_nNumVertices; ++i)
    {
        stVec3 cDirection;
        Assign3d(cDirection, m_pVertices[i].m_cDirection);

        int nThisCrossSegments = m_pVertices[i].m_nCrossSegments;

        for (int j = 0; j <= nThisCrossSegments; ++j)
        {
            // get horizontal vector
            int nHorizPrev;
            if (j == 0)
                nHorizPrev = nRunningCount + nThisCrossSegments - 1;
            else
                nHorizPrev = nRunningCount + j - 1;

            int nHorizNext;
            if (j == nThisCrossSegments)
                nHorizNext = nRunningCount + 1;
            else
                nHorizNext = nRunningCount + j + 1;

            nHorizPrev += m_nStartingOffset;
            nHorizNext += m_nStartingOffset;

            const float* pHorizPrev = pBranchGeometry->GetVertexCoord(nHorizPrev);
            const float* pHorizNext = pBranchGeometry->GetVertexCoord(nHorizNext);
            stVec3 cHoriz = stVec3(pHorizNext[0] - pHorizPrev[0], pHorizNext[1] - pHorizPrev[1], pHorizNext[2] - pHorizPrev[2]);
            cHoriz.Normalize( );

            // get vertical vector
            float fThisProgress = j / static_cast<float>(nThisCrossSegments);

            int nVertPrev;
            if (i == 0)
            {
                nVertPrev = nRunningCount + j;
            }
            else
            {
                nVertPrev = nRunningCount - ((m_pVertices[i - 1].m_nCrossSegments + 1) - static_cast<int>((m_pVertices[i - 1].m_nCrossSegments) * fThisProgress));
            }

            int nVertNext;
            if (i == m_nNumVertices - 1)
            {
                nVertNext = nRunningCount + j;
            }
            else
            {
                nVertNext = nRunningCount + (nThisCrossSegments + 1) + static_cast<int>((m_pVertices[i + 1].m_nCrossSegments) * fThisProgress);
            }

            nVertPrev += m_nStartingOffset;
            nVertNext += m_nStartingOffset;

            const float* pVertPrev = pBranchGeometry->GetVertexCoord(nVertPrev);
            const float* pVertNext = pBranchGeometry->GetVertexCoord(nVertNext);
            stVec3 cVert = stVec3(pVertNext[0] - pVertPrev[0], pVertNext[1] - pVertPrev[1], pVertNext[2] - pVertPrev[2]);
            cVert.Normalize( );

            // compute new normal
            stVec3 cNormal = cHoriz * cVert;

            if (cNormal[0] == 0.0f && cNormal[1] == 0.0f && cNormal[2] == 0.0f)
                cNormal = cDirection;

            // light seam reduction
            float fProgress = pLightSeamProfile->EvaluateWithoutVariance(i / float(m_nNumVertices - 1.0f));
            cNormal = VecInterpolate(cDirection, cNormal, fProgress * VecInterpolate(fLightSeamBias, 1.0f, powf(i / float(m_nNumVertices - 1.0f), 0.5f)));
            cNormal.Normalize( );

            if (m_pLightingEngine->GetBranchLightingMethod( ) == CSpeedTreeRT::LIGHT_STATIC)
            {
                float afColor[4];
                m_pLightingEngine->ComputeStandardStaticLighting(cNormal, pBranchGeometry->GetVertexCoord(nRunningCount), afColor);
                pBranchGeometry->AddVertexColor(afColor);
            }
            else
                pBranchGeometry->AddVertexNormal(cNormal, false);

            pBranchGeometry->AdvanceVertexCounter( );
        }

        nRunningCount += nThisCrossSegments + 1;
    }
}


///////////////////////////////////////////////////////////////////////
//  CBranch::PassesFrondRules definition
//
//  Uses the parameters specified in SpeedTreeCAD to determine if
//  this branch should be a frond

CFrondEngine::EConditions CBranch::PassesFrondRules(const CBranch* pParent, float fPercentOfParent) const
{
    CFrondEngine::EConditions eCondition = CFrondEngine::DISABLED;

    if (m_pFrondEngine->Enabled( ))
    {
        float fDistance = 0.0f;
        unsigned int uiLevel(0);
        CFrondEngine::EConditions eAbove(CFrondEngine::ENABLED);
        CFrondEngine::EConditions eBelow(CFrondEngine::ENABLED);
        m_pFrondEngine->GetRules(fDistance, uiLevel, eAbove, eBelow);

        float fTestPercent = fPercentOfParent;
        if (uiLevel > 0)
        {
            int nLevels = 0; // "this" is already pointing to the parent so start one level up

            const CBranch* pBranch = pParent;
            while (pBranch &&
                   pBranch->GetParent( ) &&
                   nLevels < int(uiLevel))
            {
                pBranch = pBranch->GetParent( );
                nLevels++;
            }

            if (pBranch)
                fTestPercent = pBranch->GetPercentOfParent( );
        }

        // we're now at the right level, so test cutoff
        if (fDistance < 0.0f)
        {
            if (fTestPercent >= -fDistance)
                eCondition = eBelow;
            else
                eCondition = eAbove;
        }
        else
        {
            if (fTestPercent >= fDistance)
                eCondition = eAbove;
            else
                eCondition = eBelow;
        }
    }

    return eCondition;
}

///////////////////////////////////////////////////////////////////////
//  CBranch::Prune definition
//
//  Uses the parameters specified in SpeedTreeCAD to determine if
//  this branch should be a frond

bool CBranch::Prune(const CBranch* pParent, float fPercentOfParent, unsigned int uiLevel, float fDistancePercent) const
{
    bool bPrune = true;

    float fTestPercent = fPercentOfParent;
    if (uiLevel != 0)
    {
        unsigned int nLevels = 1; // "this" is already pointing to the parent of the bud so start one level up

        const CBranch* pBranch = pParent;
        while (pBranch &&
               pBranch->GetParent( ) &&
               nLevels < uiLevel)
        {
            pBranch = pBranch->GetParent( );
            nLevels++;
        }

        if (pBranch)
            fTestPercent = pBranch->GetPercentOfParent( );
    }

    if (fDistancePercent < 0.0f)
    {
        if (fTestPercent <= -fDistancePercent)
            bPrune = false;
    }
    else
    {
        if (fTestPercent >= fDistancePercent)
            bPrune = false;
    }

    return bPrune;
}


///////////////////////////////////////////////////////////////////////
//  CBranch::operator=

CBranch& CBranch::operator=(const CBranch& cRight)
{
    m_pParent = cRight.m_pParent;
    m_fPercentOfParent = cRight.m_fPercentOfParent;
    m_vChildren = cRight.m_vChildren;
    m_bUsingVertexStack = cRight.m_bUsingVertexStack;

    if (cRight.m_nNumVertices > 0)
    {
        m_nNumVertices = cRight.m_nNumVertices;
        if (m_bUsingVertexStack)
        {
            memcpy(m_asVerticesStack, cRight.m_asVerticesStack, c_nMaxStackPoints * sizeof(SIdvBranchVertex));
            m_pVertices = m_asVerticesStack;
        }
        else
        {
            m_pVertices = st_new_array<SIdvBranchVertex>(m_nNumVertices, "CBranch::operator=. m_pVertices");
            memcpy(m_pVertices, cRight.m_pVertices, m_nNumVertices * sizeof(SIdvBranchVertex));
        }
    }
    else
    {
        m_pVertices = NULL;
        m_nNumVertices = 0;
    }

    m_usCrossSectionSegments = cRight.m_usCrossSectionSegments;
    m_nStartingOffset = cRight.m_nStartingOffset;
    m_fVolume = cRight.m_fVolume;
    m_fFuzzyVolume = cRight.m_fFuzzyVolume;
    m_vFlares = cRight.m_vFlares;
    m_bDestroyChildren = cRight.m_bDestroyChildren;

    return *this;
}


