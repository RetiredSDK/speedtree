///////////////////////////////////////////////////////////////////////  
//  MapBank.cpp
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

#include "MapBank.h"
#include "LibGlobals_Source/IdvGlobals.h"


///////////////////////////////////////////////////////////////////////  
//  CMapBank::CMapBank definition

CMapBank::CMapBank( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CMapBank::Save

void CMapBank::Save(CTreeFileAccess& cFile) const
{
    cFile.SaveToken(File_BeginMapBank);

    // branch maps
    cFile.SaveToken(File_MapBank_Branches);
    SaveMapCollection(m_sBranchMaps, cFile);

    // leaf maps
    cFile.SaveToken(File_MapBank_Leaves);
    cFile.SaveToken(File_MapBank_NumLeafMaps);
    cFile.SaveInt(int(m_vLeafMaps.size( )));
    unsigned int i = 0;
    for (i = 0; i < m_vLeafMaps.size( ); ++i)
        SaveMapCollection(m_vLeafMaps[i], cFile);

    // frond maps
    cFile.SaveToken(File_MapBank_Fronds);
    cFile.SaveToken(File_MapBank_NumFrondMaps);
    cFile.SaveInt(int(m_vFrondMaps.size( )));
    for (i = 0; i < m_vFrondMaps.size( ); ++i)
        SaveMapCollection(m_vFrondMaps[i], cFile);

    // composite maps
    cFile.SaveToken(File_MapBank_Composite);
    SaveMapCollection(m_sCompositeMaps, cFile);

    cFile.SaveToken(File_MapBank_Billboard);
    SaveMapCollection(m_sBillboardMaps, cFile);

    // self shadow
    cFile.SaveToken(File_MapBank_SelfShadow);
    cFile.SaveString(m_strSelfShadowMap);

    cFile.SaveToken(File_EndMapBank);
}


///////////////////////////////////////////////////////////////////////  
//  CMapBank::Parse

void CMapBank::Parse(CTreeFileAccess& cFile)
{
    int nToken;
    do
    {
        nToken = cFile.ParseToken( );
        switch (nToken)
        {
        case File_MapBank_Branches:
            ParseMapCollection(m_sBranchMaps, cFile);
            break;
        case File_MapBank_Leaves:
            {
                if (cFile.ParseToken( ) != File_MapBank_NumLeafMaps)
                    throw(IdvFileError("malformed map bank (expected num leaf maps)"));

                int nNumLeafMaps = cFile.ParseInt( );

                for (int i = 0; i < nNumLeafMaps; ++i)
                {
                    SMapCollection sCollection;
                    ParseMapCollection(sCollection, cFile);
                    m_vLeafMaps.push_back(sCollection);
                }
            }
            break;
        case File_MapBank_Fronds:
            {
                if (cFile.ParseToken( ) != File_MapBank_NumFrondMaps)
                    throw(IdvFileError("malformed map bank (expected num frond maps)"));

                int nNumFrondMaps = cFile.ParseInt( );

                for (int i = 0; i < nNumFrondMaps; ++i)
                {
                    SMapCollection sCollection;
                    ParseMapCollection(sCollection, cFile);
                    m_vFrondMaps.push_back(sCollection);
                }
            }
            break;
        case File_MapBank_Composite:
            ParseMapCollection(m_sCompositeMaps, cFile);
            break;
        case File_MapBank_Billboard:
            ParseMapCollection(m_sBillboardMaps, cFile);
            break;
        case File_MapBank_SelfShadow:
            cFile.ParseString(m_strSelfShadowMap);
            break;
        case File_EndMapBank:
            break;
        default:
            throw(IdvFileError("malformed map bank"));
        }
    }
    while (nToken != File_EndMapBank);
}


///////////////////////////////////////////////////////////////////////  
//  CMapBank::SaveMapCollection

void CMapBank::SaveMapCollection(const SMapCollection& sCollection, CTreeFileAccess& cFile) const
{
    cFile.SaveToken(File_BeginMapCollection);

    cFile.SaveToken(File_MapCollection_Diffuse);
    cFile.SaveString(sCollection.m_astrMaps[CSpeedTreeRT::TL_DIFFUSE]);

    cFile.SaveToken(File_MapCollection_Detail);
    cFile.SaveString(sCollection.m_astrMaps[CSpeedTreeRT::TL_DETAIL]);

    cFile.SaveToken(File_MapCollection_Normal);
    cFile.SaveString(sCollection.m_astrMaps[CSpeedTreeRT::TL_NORMAL]);

    cFile.SaveToken(File_MapCollection_Height);
    cFile.SaveString(sCollection.m_astrMaps[CSpeedTreeRT::TL_HEIGHT]);

    cFile.SaveToken(File_MapCollection_Specular);
    cFile.SaveString(sCollection.m_astrMaps[CSpeedTreeRT::TL_SPECULAR]);

    cFile.SaveToken(File_MapCollection_User1);
    cFile.SaveString(sCollection.m_astrMaps[CSpeedTreeRT::TL_USER1]);

    cFile.SaveToken(File_MapCollection_User2);
    cFile.SaveString(sCollection.m_astrMaps[CSpeedTreeRT::TL_USER2]);

    cFile.SaveToken(File_EndMapCollection);
}


///////////////////////////////////////////////////////////////////////  
//  CMapBank::ParseMapCollection

void CMapBank::ParseMapCollection(SMapCollection& sCollection, CTreeFileAccess& cFile)
{
    if (cFile.ParseToken( ) != File_BeginMapCollection)
        throw(IdvFileError("malformed map collection"));

    int nToken;
    do
    {
        nToken = cFile.ParseToken( );
        switch (nToken)
        {
        case File_MapCollection_Diffuse:
            cFile.ParseString(sCollection.m_astrMaps[CSpeedTreeRT::TL_DIFFUSE]);
            break;
        case File_MapCollection_Detail:
            cFile.ParseString(sCollection.m_astrMaps[CSpeedTreeRT::TL_DETAIL]);
            break;
        case File_MapCollection_Normal:
            cFile.ParseString(sCollection.m_astrMaps[CSpeedTreeRT::TL_NORMAL]);
            break;
        case File_MapCollection_Height:
            cFile.ParseString(sCollection.m_astrMaps[CSpeedTreeRT::TL_HEIGHT]);
            break;
        case File_MapCollection_Specular:
            cFile.ParseString(sCollection.m_astrMaps[CSpeedTreeRT::TL_SPECULAR]);
            break;
        case File_MapCollection_User1:
            cFile.ParseString(sCollection.m_astrMaps[CSpeedTreeRT::TL_USER1]);
            break;
        case File_MapCollection_User2:
            cFile.ParseString(sCollection.m_astrMaps[CSpeedTreeRT::TL_USER2]);
            break;
        case File_EndMapCollection:
            break;
        default:
            throw(IdvFileError("malformed map collection"));
        }
    }
    while (nToken != File_EndMapCollection);
}

