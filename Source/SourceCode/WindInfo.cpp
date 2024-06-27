///////////////////////////////////////////////////////////////////////  
//  WindInfo.cpp
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


///////////////////////////////////////////////////////////////////////  
//  SIdvWindInfo::SIdvWindInfo definition

SIdvWindInfo::SIdvWindInfo( ) : 
    m_cLeafFactors(0.5f, 0.5f),
    m_cLeafOscillation(0.0f, 0.0f, 0.0f),
    m_fStrength(0.0f)
{
}


///////////////////////////////////////////////////////////////////////  
//  SIdvWindInfo::Parse definition

void SIdvWindInfo::Parse(CTreeFileAccess& cFile)
{
    int nToken = cFile.ParseToken( );
    do
    {
        switch (nToken)
        {
            case File_Wind_Direction:
                (void) cFile.ParseVector3( ); // wind direction is no longer used
                break;
            case File_Wind_BranchOscillation:
                (void) cFile.ParseVector3( ); // branch oscillation is no longer used
                break;
            case File_Wind_LeafOscillation:
                m_cLeafOscillation = cFile.ParseVector3( );
                break;
            case File_Wind_BranchFactors:
                (void) cFile.ParseVector3( ); // branch factor is no longer used
                break;
            case File_Wind_LeafFactors:
                m_cLeafFactors = cFile.ParseVector3( );
                break;
            case File_Wind_Strength:
                m_fStrength = cFile.ParseFloat( );
                break;
            case File_Wind_Enabled:
                (void) cFile.ParseBool( ); // wind enabled is no longer used
                break;
            default:
                throw(IdvFileError("malformed general wind information"));
        }
        nToken = cFile.ParseToken( );
    } while (nToken != File_EndWindInfo);
}


///////////////////////////////////////////////////////////////////////  
//  SIdvWindInfo::Save definition

void SIdvWindInfo::Save(CTreeFileAccess& cFile) const
{
    cFile.SaveToken(File_BeginWindInfo);

    cFile.SaveToken(File_Wind_Direction);
    cFile.SaveVector3(stVec3( )); // wind direction is no longer used

    cFile.SaveToken(File_Wind_BranchOscillation);
    cFile.SaveVector3(stVec3( )); // branch oscillation is no longer used

    cFile.SaveToken(File_Wind_LeafOscillation);
    cFile.SaveVector3(m_cLeafOscillation);

    cFile.SaveToken(File_Wind_BranchFactors);
    cFile.SaveVector3(stVec3( )); // branch factors is no longer used

    cFile.SaveToken(File_Wind_LeafFactors);
    cFile.SaveVector3(m_cLeafFactors);

    cFile.SaveToken(File_Wind_Strength);
    cFile.SaveFloat(m_fStrength);

    cFile.SaveToken(File_Wind_Enabled);
    cFile.SaveBool(false); // wind enabled is no longer used

    cFile.SaveToken(File_EndWindInfo);
}
