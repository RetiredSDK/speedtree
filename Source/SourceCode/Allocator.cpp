///////////////////////////////////////////////////////////////////////  
//  Allocator.cpp
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

#ifdef TRACK_ALLOC_STATS

#ifdef WIN32
#pragma warning (disable : 4702 4221) // disable warning about unreachable code in xtree
#endif
#define ALLOCATOR_SOURCE_FILE
#include "SpeedTreeAllocator.h"

CSpeedTreeAllocator::stats_map* m_pStatsMap = NULL;
CSpeedTreeAllocator::SAllocStats m_sGlobalStats;
CSpeedTreeAllocator::leak_map m_mLeakMap;


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeAllocator::TrackAlloc

void CSpeedTreeAllocator::TrackAlloc(const char* pDescription, void* pBlock, size_t sAmount)
{
    if (!m_pStatsMap)
        m_pStatsMap = new stats_map;

    // local statistics
    (*m_pStatsMap)[pDescription].m_siNumAllocates++;
    (*m_pStatsMap)[pDescription].m_siAmountAllocated += sAmount;

    // global statistics
    m_sGlobalStats.m_siNumAllocates++;
    m_sGlobalStats.m_siAmountAllocated += sAmount;

    // leak statistics
    leak_map::iterator iFind = m_mLeakMap.find(pBlock);
    m_mLeakMap[pBlock].m_nCount++;
    m_mLeakMap[pBlock].m_nAmount += int(sAmount);
    m_mLeakMap[pBlock].m_strDesc = std::string(pDescription);
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeAllocator::TrackFree

void CSpeedTreeAllocator::TrackFree(const char* pDescription, void* pBlock, size_t sAmount)
{
    if (pBlock)
    {
        if (!m_pStatsMap)
            m_pStatsMap = new stats_map;

        // local statistics
        (*m_pStatsMap)[pDescription].m_siNumFrees++;
        (*m_pStatsMap)[pDescription].m_siAmountFreed += sAmount;

        // global statistics
        m_sGlobalStats.m_siNumFrees++;
        m_sGlobalStats.m_siAmountFreed += sAmount;

        // leak statistics
        leak_map::iterator iFind = m_mLeakMap.find(pBlock);
        if (iFind != m_mLeakMap.end( ))
        {
            iFind->second.m_nCount--;
            iFind->second.m_nAmount -= int(sAmount);
            if (iFind->second.m_nCount == 0)
                (void) m_mLeakMap.erase(iFind);
        }
        else
            fprintf(stderr, "CSpeedTreeAllocator::TrackFree cannot find [%s]\n", pDescription);
    }
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeAllocator::Report

bool CSpeedTreeAllocator::Report(const char* pFilename)
{
    bool bSuccess = false;
    SAllocStats sAllAllocators;
    std::vector<SAllocStats> vOrdered;
    if (m_pStatsMap)
    {
        for (stats_map::const_iterator i = m_pStatsMap->begin( ); i != m_pStatsMap->end( ); ++i)
        {
            vOrdered.push_back(i->second);
            vOrdered.back( ).m_strDesc = i->first;
            sAllAllocators += i->second;
        }
        std::sort(vOrdered.begin( ), vOrdered.end( ));
    }

    FILE* pFile = fopen(pFilename, "w");
    if (pFile)
    {
        fprintf(pFile, "GLOBAL ------------------\n");
        fprintf(pFile, "\t[%d] total allocations, [%1.f KB] allocated\n", m_sGlobalStats.m_siNumAllocates, m_sGlobalStats.m_siAmountAllocated / 1024.0f);
        fprintf(pFile, "ALL ALLOCATORS ----------\n");
        fprintf(pFile, "\t[%d] total allocations, [%1.f KB] allocated\n", sAllAllocators.m_siNumAllocates, sAllAllocators.m_siAmountAllocated / 1024.0f);
        fprintf(pFile, "\t[%d] total free calls,  [%1.f KB] freed\n", sAllAllocators.m_siNumFrees, sAllAllocators.m_siAmountFreed / 1024.0f);
        fprintf(pFile, "\nAllocator\t# Allocs\t%% Allocs\t# Frees\t%% Frees\tAlloced (KB)\tFreed (KB)\n");
        for (int i = 0; i < int(vOrdered.size( )); ++i)
        {
            fprintf(pFile, "%s\t%d\t%g\t%d\t%g\t%g\t%g\n",
                vOrdered[i].m_strDesc.c_str( ),
                vOrdered[i].m_siNumAllocates,
                100.0f * vOrdered[i].m_siNumAllocates / float(sAllAllocators.m_siNumAllocates),
                vOrdered[i].m_siNumFrees,
                100.0f * vOrdered[i].m_siNumFrees / float(sAllAllocators.m_siNumFrees),
                vOrdered[i].m_siAmountAllocated / 1024.0f,
                vOrdered[i].m_siAmountFreed / 1024.0f);
        }

        // leak report
        fprintf(pFile, "LEAK REPORT\n");

        // fill vector with values and sort
        int nIndex = 0;
        int nTotalLeaked = 0;
        std::vector<SLeakStats> vLeaks(m_mLeakMap.size( ));
        for (leak_map::const_iterator j = m_mLeakMap.begin( ); j != m_mLeakMap.end( ); ++j)
        {
            vLeaks[nIndex++] = j->second;
            nTotalLeaked += j->second.m_nAmount;
        }
        sort(vLeaks.begin( ), vLeaks.end( ));

        fprintf(pFile, "Total Memory Leaked: %.1fKB\n", nTotalLeaked / 1024.0f);
        for (nIndex = 0; nIndex < int(vLeaks.size( )); ++nIndex)
            fprintf(pFile, "\t%d. [%s] qty: %d, total amount: %.1fKB\n", 
                nIndex + 1, vLeaks[nIndex].m_strDesc.c_str( ), vLeaks[nIndex].m_nCount, vLeaks[nIndex].m_nAmount / 1024.0f);

        fclose(pFile);

        bSuccess = true;
    }

    return bSuccess;
}

#endif // TRACK_ALLOC_STATS

