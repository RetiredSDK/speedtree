///////////////////////////////////////////////////////////////////////  
//  LeafLod.cpp
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

#include "LeafLod.h"


///////////////////////////////////////////////////////////////////////  
//  CLeafLodEngine::CLeafLodEngine definition

CLeafLodEngine::CLeafLodEngine(const SIdvLeafInfo& sLeafInfo, float fSpacingTolerance, float fLeafReductionPercentage, float fSizeIncreaseFactor) :
    m_fLeafSpacingTolerance(fSpacingTolerance),
    m_fLeafReductionPercentage(fLeafReductionPercentage),
    m_sLeafInfo(sLeafInfo),
    m_fLeafSizeIncreaseFactor(fSizeIncreaseFactor)
{
}


///////////////////////////////////////////////////////////////////////  
//  CLeafLodEngine::ComputeNextLevel definition

void CLeafLodEngine::ComputeNextLevel(st_vector_leaves vOrigLeafSet, st_vector_leaves& vNextLevel)
{
    // pairs of leaves are combined into one
    FindPairs(vOrigLeafSet);
    BuildNewLeaves(vNextLevel);
}


///////////////////////////////////////////////////////////////////////  
//  CLeafLodEngine::BuildNewLeaves definition

void CLeafLodEngine::BuildNewLeaves(st_vector_leaves& vLeaves)
{
    static stRandom cDice;

    vLeaves.reserve(m_vPairs.size( ));
    for (st_vector_lodentry::iterator i = m_vPairs.begin( ); i != m_vPairs.end( ); ++i)
    {
        // random call is to randomly remove m_fLeafReductionPercentage of the pairings, resulting in an
        // arbitrary reduction of the leaf count of the next LOD level
        if (cDice.GetUniform(0.0, 1.0) > m_fLeafReductionPercentage)
        {
            CBillboardLeaf cNewLeaf = *i->m_pLeaf;

            // adjust position based on new size and origin location
            stVec3 cNewPosition = ((i->m_pLeaf->GetPosition( )) + (i->m_pLeafMatch->GetPosition( ))) * 0.5f;

            const SIdvLeafTexture& sTexture = m_sLeafInfo.m_vLeafTextures[i->m_pLeaf->GetTextureIndex( )];
            float fAdjust = (sTexture.m_cOrigin[1] - 0.5f) * (m_fLeafSizeIncreaseFactor * sTexture.m_cSizeUsed[1] * 0.5f);
            cNewPosition[2] += fAdjust;

            cNewLeaf.SetPosition(cNewPosition);
            cNewLeaf.SetColor((i->m_pLeaf->GetColor(0) + i->m_pLeafMatch->GetColor(0)) * 0.5f, 0);
            cNewLeaf.SetColor((i->m_pLeaf->GetColor(1) + i->m_pLeafMatch->GetColor(1)) * 0.5f, 1);
            cNewLeaf.SetColor((i->m_pLeaf->GetColor(2) + i->m_pLeafMatch->GetColor(2)) * 0.5f, 2);
            cNewLeaf.SetColor((i->m_pLeaf->GetColor(3) + i->m_pLeafMatch->GetColor(3)) * 0.5f, 3);
            vLeaves.push_back(cNewLeaf);

#ifdef V18
            // adjust position based on new size and origin location
            CVec3 cNewPosition = (i->m_pLeaf->GetPosition( ) + i->m_pLeafMatch->GetPosition( )) * 0.5f;
            const SIdvLeafTexture& sTexture = m_sLeafInfo.m_vLeafTextures[i->m_pLeaf->GetTextureIndex( )];
            float fAdjust = (sTexture.m_cOrigin[1] - 0.5f) * (m_fLeafSizeIncreaseFactor * sTexture.m_cSizeUsed[1] * 0.5f);
            cNewPosition[2] += fAdjust;

            pNewLeaf->SetPosition(cNewPosition);
            pNewLeaf->SetColor((i->m_pLeaf->GetColor( ) + i->m_pLeafMatch->GetColor( )) * 0.5f, false);
            vLeaves.push_back(pNewLeaf);
#endif
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  CLeafLodEngine::FindPairs definition

void CLeafLodEngine::FindPairs(const st_vector_leaves& vOrigLeafSet)
{
    // initialize an vector of bools parallel to the leaves array (all entries false).  these
    // are used to flag if a leaf has been paired
#ifdef PS3
    // using a vector of bools yields strange warnings under gcc
    st_vector_int vFound(vOrigLeafSet.size( ));
    fill(vFound.begin( ), vFound.end( ), 0);
#else
    st_vector_bool vFound(vOrigLeafSet.size( ));
    std::fill(vFound.begin( ), vFound.end( ), false);
#endif

    for (st_vector_leaves::const_iterator i = vOrigLeafSet.begin( ); i != vOrigLeafSet.end( ); ++i)
    {
        SLodEntry sLod(&(*i));

        float fShortestDistance = FLT_MAX;
        int nPairedIndex = -1;
        int nSearchIndex = int(i - vOrigLeafSet.begin( ) + 1);

        // find the closest leaf to *i that:
        //      1. hasn't already been paired 
        //      2. is closer than m_fLeafSpacingTolerance
        for (st_vector_leaves::const_iterator j = i + 1; j != vOrigLeafSet.end( ) && !vFound[nSearchIndex]; ++j)
        {
            if (!vFound[nSearchIndex])
            {
                float fDistance = i->GetPosition( ).Distance(j->GetPosition( ));
                if (fDistance < m_fLeafSpacingTolerance &&
                    fDistance < fShortestDistance)
                {
                    fShortestDistance = fDistance;
                    sLod.m_pLeafMatch = &(*j);
                    nPairedIndex = nSearchIndex;
                }
            }
            nSearchIndex++;
        }

        if (sLod.m_pLeafMatch)
        {
            vFound[nPairedIndex] = true;
            m_vPairs.push_back(sLod); // save each pair for use in BuildNewLeaves()
        }
    }
}

