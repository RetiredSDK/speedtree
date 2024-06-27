///////////////////////////////////////////////////////////////////////  
//  LeafLod.h
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
#include "TreeEngine.h"
#include "StructsInfo.h"
#if defined(WIN32) || defined(_XBOX)
#pragma warning (disable : 4512)
#endif


///////////////////////////////////////////////////////////////////////  
//  class CLeafLodEngine declaration
//
//  Given a set of billboard leaves (CBillboardLeaf), this class will
//  construct a set of fewer, larger leaves for an appropriate LOD
//  step down.

class CLeafLodEngine
{
public:
        CLeafLodEngine(const SIdvLeafInfo& sLeafInfo, float fSpacingTolerance, float fLeafReductionPercentage, float fSizeIncreaseFactor);

        void                    ComputeNextLevel(st_vector_leaves vOrigLeafSet, st_vector_leaves& vNextLevel);

private:

        ///////////////////////////////////////////////////////////////////////  
        //  struct SLodEntry definition

        struct SLodEntry
        {
            SLodEntry(const CBillboardLeaf* pLeaf) :
                m_pLeaf(pLeaf),
                m_pLeafMatch(NULL)
            {
            }
            ~SLodEntry( )
            {
                m_pLeaf = NULL;
                m_pLeafMatch = NULL;
            }

            const CBillboardLeaf*   m_pLeaf;                    // main leaf
            const CBillboardLeaf*   m_pLeafMatch;               // leaf paired with the main leaf
        };

        typedef std::vector<SLodEntry, st_allocator<SLodEntry> > st_vector_lodentry;

        void                    BuildNewLeaves(st_vector_leaves& vLeaves);
        void                    FindPairs(const st_vector_leaves& vOrigLeafSet);

        st_vector_lodentry      m_vPairs;                       // list of leaf pairs that will combined into one larger leaf       
        float                   m_fLeafSpacingTolerance;        // don't pick leaves any further apart than this distance
        float                   m_fLeafReductionPercentage;     // controls what percentage of leaves are removed at each LOD level
        const SIdvLeafInfo&     m_sLeafInfo;                    // contains leaf sizes, origins, etc.
        float                   m_fLeafSizeIncreaseFactor;      // determines how much the size of reduced leaf set is increased.
};
