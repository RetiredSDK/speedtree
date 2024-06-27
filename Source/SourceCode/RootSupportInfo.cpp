///////////////////////////////////////////////////////////////////////  
//  RootSupportInfo.cpp
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


///////////////////////////////////////////////////////////////////////  
//  Include Files

#include "LibGlobals_Source/IdvGlobals.h"
#include "StructsInfo.h"
#include "FileAccess.h"


///////////////////////////////////////////////////////////////////////  
//  SRootSupportInfo::SRootSupportInfo

SRootSupportInfo::SRootSupportInfo( ) :
    m_nLevel(1),
    m_fFirst(0.0f),
    m_fLast(0.15f),
    m_fFrequency(0.2f)
{
}



///////////////////////////////////////////////////////////////////////  
//  SRootSupportInfo::Parse

void SRootSupportInfo::Parse(CTreeFileAccess& cFile)
{
    int nToken;
    do
    {
        nToken = cFile.ParseToken( );
        switch (nToken)
        {
        case File_RootLevel:
            m_nLevel = cFile.ParseInt( );
            break;
        case File_RootFirst:
            m_fFirst = cFile.ParseFloat( );
            break;
        case File_RootLast:
            m_fLast = cFile.ParseFloat( );
            break;
        case File_RootPercentage:
            m_fFrequency = cFile.ParseFloat( );
            break;
        case File_RootData:
            m_sRoots.ParseThis(cFile);
            break;
        case File_BeginSupplementalBranchData:
            m_sRoots.ParseSupplemental(cFile);
            break;
        case File_EndRootSupportInfo:
            break;
        default:
            throw(IdvFileError("malformed root support info"));
        }
    }
    while (nToken != File_EndRootSupportInfo);
}


///////////////////////////////////////////////////////////////////////  
//  SRootSupportInfo::Save

void SRootSupportInfo::Save(CTreeFileAccess& cFile) const
{
    cFile.SaveToken(File_BeginRootSupportInfo);

    cFile.SaveToken(File_RootLevel);
    cFile.SaveInt(m_nLevel);

    cFile.SaveToken(File_RootFirst);
    cFile.SaveFloat(m_fFirst);

    cFile.SaveToken(File_RootLast);
    cFile.SaveFloat(m_fLast);

    cFile.SaveToken(File_RootPercentage);
    cFile.SaveFloat(m_fFrequency);

    cFile.SaveToken(File_RootData);
    cFile.SaveToken(File_BeginBranchLevel);
    m_sRoots.Save(cFile);
    cFile.SaveToken(File_EndBranchLevel);

    m_sRoots.SaveSupplemental(cFile);

    cFile.SaveToken(File_EndRootSupportInfo);
}
