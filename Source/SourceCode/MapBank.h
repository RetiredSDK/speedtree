///////////////////////////////////////////////////////////////////////  
//  SimpleBillboard.h
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
#include "Debug.h"
#include "SpeedTreeRT.h"
#include "FileAccess.h"


///////////////////////////////////////////////////////////////////////  
//  class SMapCollection declaration

struct SMapCollection
{
    st_string       m_astrMaps[CSpeedTreeRT::TL_SHADOW];
};


// ps3 (can't define allocator inside class decl)
DefineAllocator(st_allocator_map_collection);
typedef std::vector<SMapCollection, st_allocator_map_collection<SMapCollection> > st_vector_map_collection;


///////////////////////////////////////////////////////////////////////  
//  class CMapBank declaration

class CMapBank
{
public:

        CMapBank( );

        void                        Save(CTreeFileAccess& cFile) const;
        void                        Parse(CTreeFileAccess& cFile);

        // branch maps
        SMapCollection              m_sBranchMaps;

        // leaf maps
        st_vector_map_collection    m_vLeafMaps;

        // frond maps
        st_vector_map_collection    m_vFrondMaps;

        // composite maps
        SMapCollection              m_sCompositeMaps;
        SMapCollection              m_sBillboardMaps;

        // self-shadow
        st_string                   m_strSelfShadowMap;

private:
        void                        SaveMapCollection(const SMapCollection& sCollection, CTreeFileAccess& cFile) const;
        void                        ParseMapCollection(SMapCollection& sCollection, CTreeFileAccess& cFile);
};
