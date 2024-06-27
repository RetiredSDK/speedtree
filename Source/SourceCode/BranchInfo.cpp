///////////////////////////////////////////////////////////////////////  
//  BranchInfo.cpp
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


///////////////////////////////////////////////////////////////////////  
//  SIdvBranchInfo::SIdvBranchInfo definition

SIdvBranchInfo::SIdvBranchInfo( ) :
    m_nCrossSectionSegments(6),
    m_nSegments(3),
    m_fFirstBranch(0.3f),
    m_fLastBranch(1.0f),
    m_fFrequency(0.3f),
    m_fSegmentPackingExponent(1.0f),
    m_nNumFlares(0),
    m_fFlareBalance(1.0f),
    m_fRadialInfluence(30.0f),
    m_fRadialInfluenceVariance(10.0f),
    m_fRadialExponent(1.0f),
    m_fRadialDistance(0.5f),
    m_fRadialVariance(0.25f),
    m_fLengthDistance(0.3f),
    m_fLengthVariance(0.1f),
    m_fLengthExponent(1.0f),
    m_fLightSeamStartBias(1.0f),
    m_fLightSeamEndBias(1.0f),
    m_fRoughness(0.0f),
    m_fRoughnessVerticalFrequency(30.0f),
    m_fRoughnessHorizontalFrequency(5.0f),
    m_fRandomRoughness(0.0f),
    m_fMinLengthPercent(1.0f),
    m_fMinCrossSectionPercent(1.0f),
    m_fPruningPercent(0.0f),
    m_uiPruningDepth(0),
    m_bForkingEnabled(false),
    m_fForkBias(1.0f),
    m_fForkAngle(0.0f),
    m_nForkLimit(1),
    m_fGnarl(0.0f),
    m_bGnarlUnison(false)
{
}


///////////////////////////////////////////////////////////////////////  
//  SIdvBranchInfo::SIdvBranchInfo definition

SIdvBranchInfo::SIdvBranchInfo(const SIdvBranchInfo& sRight)
{
    *this = sRight;
}


///////////////////////////////////////////////////////////////////////  
//  SIdvBranchInfo::Parse definition

SIdvBranchInfo* SIdvBranchInfo::Parse(CTreeFileAccess& cFile)
{
    SIdvBranchInfo* pInfo = new ("SIdvBranchInfo::Parse, pInfo") SIdvBranchInfo;

    int nToken = cFile.ParseToken( );
    if (nToken != File_BeginBranchLevel)
        throw(IdvFileError("malformed branch data"));

    nToken = cFile.ParseToken( );
    do
    {
        switch (nToken)
        {
            case File_Branch_Disturbance:
                cFile.ParseBranchParameter(pInfo->GetDisturbance( ));
                break;
            case File_Branch_Gravity:
                cFile.ParseBranchParameter(pInfo->GetGravity( ));
                break;
            case File_Branch_Flexibility:
                cFile.ParseBranchParameter(pInfo->GetFlexibility( ));
                break;
            case File_Branch_FlexibilityScale:
                cFile.ParseBranchParameter(pInfo->GetFlexibilityScale( ));
                break;
            case File_Branch_AngleProfile:
                cFile.ParseBranchParameter(pInfo->GetAngleProfile( ));
                break;
            case File_Branch_Length:
                cFile.ParseBranchParameter(pInfo->GetLength( ));
                break;
            case File_Branch_Radius:
                cFile.ParseBranchParameter(pInfo->GetRadius( ));
                break;
            case File_Branch_RadiusScale:
                cFile.ParseBranchParameter(pInfo->GetRadiusScale( ));
                break;
            case File_Branch_StartAngle:
                cFile.ParseBranchParameter(pInfo->GetStartAngle( ));
                break;
            case File_Branch_CrossSectionSegments:
                pInfo->m_nCrossSectionSegments = cFile.ParseInt( );
                break;
            case File_Branch_Segments:
                pInfo->m_nSegments = cFile.ParseInt( );
                break;
            case File_Branch_FirstBranch:
                pInfo->m_fFirstBranch = cFile.ParseFloat( );
                break;
            case File_Branch_LastBranch:
                pInfo->m_fLastBranch = cFile.ParseFloat( );
                break;
            case File_Branch_Frequency:
                pInfo->m_fFrequency = cFile.ParseFloat( );
                break;
            case File_Branch_STile:
                pInfo->m_asTexCoordControls[CSpeedTreeRT::TL_DIFFUSE].m_fSTile = cFile.ParseFloat( );
                break;
            case File_Branch_TTile:
                pInfo->m_asTexCoordControls[CSpeedTreeRT::TL_DIFFUSE].m_fTTile = cFile.ParseFloat( );
                break;
            case File_Branch_STileAbsolute:
                pInfo->m_asTexCoordControls[CSpeedTreeRT::TL_DIFFUSE].m_bSTileAbsolute = cFile.ParseBool( );
                break;
            case File_Branch_TTileAbsolute:
                pInfo->m_asTexCoordControls[CSpeedTreeRT::TL_DIFFUSE].m_bTTileAbsolute = cFile.ParseBool( );
                break;
            default:
                throw(IdvFileError("malformed general branch information"));
        }
        nToken = cFile.ParseToken( );
    } while (nToken != File_EndBranchLevel);

    return pInfo;
}


///////////////////////////////////////////////////////////////////////  
//  SIdvBranchInfo::ParseThis definition

void SIdvBranchInfo::ParseThis(CTreeFileAccess& cFile)
{
    SIdvBranchInfo* pInfo = this;

    int nToken = cFile.ParseToken( );
    if (nToken != File_BeginBranchLevel)
        throw(IdvFileError("malformed branch data"));

    nToken = cFile.ParseToken( );
    do
    {
        switch (nToken)
        {
            case File_Branch_Disturbance:
                cFile.ParseBranchParameter(pInfo->GetDisturbance( ));
                break;
            case File_Branch_Gravity:
                cFile.ParseBranchParameter(pInfo->GetGravity( ));
                break;
            case File_Branch_Flexibility:
                cFile.ParseBranchParameter(pInfo->GetFlexibility( ));
                break;
            case File_Branch_FlexibilityScale:
                cFile.ParseBranchParameter(pInfo->GetFlexibilityScale( ));
                break;
            case File_Branch_AngleProfile:
                cFile.ParseBranchParameter(pInfo->GetAngleProfile( ));
                break;
            case File_Branch_Length:
                cFile.ParseBranchParameter(pInfo->GetLength( ));
                break;
            case File_Branch_Radius:
                cFile.ParseBranchParameter(pInfo->GetRadius( ));
                break;
            case File_Branch_RadiusScale:
                cFile.ParseBranchParameter(pInfo->GetRadiusScale( ));
                break;
            case File_Branch_StartAngle:
                cFile.ParseBranchParameter(pInfo->GetStartAngle( ));
                break;
           case File_Branch_CrossSectionSegments:
                pInfo->m_nCrossSectionSegments = cFile.ParseInt( );
                break;
            case File_Branch_Segments:
                pInfo->m_nSegments = cFile.ParseInt( );
                break;
            case File_Branch_FirstBranch:
                pInfo->m_fFirstBranch = cFile.ParseFloat( );
                break;
            case File_Branch_LastBranch:
                pInfo->m_fLastBranch = cFile.ParseFloat( );
                break;
            case File_Branch_Frequency:
                pInfo->m_fFrequency = cFile.ParseFloat( );
                break;
            case File_Branch_STile:
                pInfo->m_asTexCoordControls[CSpeedTreeRT::TL_DIFFUSE].m_fSTile = cFile.ParseFloat( );
                break;
            case File_Branch_TTile:
                pInfo->m_asTexCoordControls[CSpeedTreeRT::TL_DIFFUSE].m_fTTile = cFile.ParseFloat( );
                break;
            case File_Branch_STileAbsolute:
                pInfo->m_asTexCoordControls[CSpeedTreeRT::TL_DIFFUSE].m_bSTileAbsolute = cFile.ParseBool( );
                break;
            case File_Branch_TTileAbsolute:
                pInfo->m_asTexCoordControls[CSpeedTreeRT::TL_DIFFUSE].m_bTTileAbsolute = cFile.ParseBool( );
                break;
            default:
                throw(IdvFileError("malformed general branch information"));
        }
        nToken = cFile.ParseToken( );
    } while (nToken != File_EndBranchLevel);
}


///////////////////////////////////////////////////////////////////////  
//  SIdvBranchInfo::Save definition

void SIdvBranchInfo::Save(CTreeFileAccess& cFile) const
{
    cFile.SaveToken(File_Branch_Disturbance);
    cFile.SaveBranchParameter(&m_cDisturbance);

    cFile.SaveToken(File_Branch_Gravity);
    cFile.SaveBranchParameter(&m_cGravity);

    cFile.SaveToken(File_Branch_Flexibility);
    cFile.SaveBranchParameter(&m_cFlexibility);

    cFile.SaveToken(File_Branch_FlexibilityScale);
    cFile.SaveBranchParameter(&m_cFlexibilityScale);

    cFile.SaveToken(File_Branch_Length);
    cFile.SaveBranchParameter(&m_cLength);

    cFile.SaveToken(File_Branch_Radius);
    cFile.SaveBranchParameter(&m_cRadius);

    cFile.SaveToken(File_Branch_RadiusScale);
    cFile.SaveBranchParameter(&m_cRadiusScale);

    cFile.SaveToken(File_Branch_StartAngle);
    cFile.SaveBranchParameter(&m_cStartAngle);

    cFile.SaveToken(File_Branch_CrossSectionSegments);
    cFile.SaveInt(m_nCrossSectionSegments);

    cFile.SaveToken(File_Branch_Segments);
    cFile.SaveInt(m_nSegments);

    cFile.SaveToken(File_Branch_FirstBranch);
    cFile.SaveFloat(m_fFirstBranch);

    cFile.SaveToken(File_Branch_LastBranch);
    cFile.SaveFloat(m_fLastBranch);

    cFile.SaveToken(File_Branch_Frequency);
    cFile.SaveFloat(m_fFrequency);

    cFile.SaveToken(File_Branch_STile);
    cFile.SaveFloat(m_asTexCoordControls[CSpeedTreeRT::TL_DIFFUSE].m_fSTile);

    cFile.SaveToken(File_Branch_TTile);
    cFile.SaveFloat(m_asTexCoordControls[CSpeedTreeRT::TL_DIFFUSE].m_fTTile);

    cFile.SaveToken(File_Branch_STileAbsolute);
    cFile.SaveBool(m_asTexCoordControls[CSpeedTreeRT::TL_DIFFUSE].m_bSTileAbsolute);

    cFile.SaveToken(File_Branch_TTileAbsolute);
    cFile.SaveBool(m_asTexCoordControls[CSpeedTreeRT::TL_DIFFUSE].m_bTTileAbsolute);

    cFile.SaveToken(File_Branch_AngleProfile);
    cFile.SaveBranchParameter(&m_cAngleProfile);
}


///////////////////////////////////////////////////////////////////////  
//  SIdvBranchInfo::ParseSupplemental

void SIdvBranchInfo::ParseSupplemental(CTreeFileAccess& cFile)
{
    int nToken = cFile.ParseToken( );
    do
    {
        switch (nToken)
        {
            case File_BranchRoughness:
                m_fRoughness = cFile.ParseFloat( );
                break;
            case File_BranchRoughnessVerticalFrequency:
                m_fRoughnessVerticalFrequency = cFile.ParseFloat( );
                break;
            case File_BranchRoughnessHorizontalFrequency:
                m_fRoughnessHorizontalFrequency = cFile.ParseFloat( );
                break;
            case File_BranchRandomRoughness:
                m_fRandomRoughness = cFile.ParseFloat( );
                break;
            case File_BranchMinLengthPercent:
                m_fMinLengthPercent = cFile.ParseFloat( );
                break;
            case File_BranchMinCrossSectionPercent:
                m_fMinCrossSectionPercent = cFile.ParseFloat( );
                break;
            case File_BranchPruningPercent:
                m_fPruningPercent = cFile.ParseFloat( );
                break;
            case File_BranchPruningDepth:
                m_uiPruningDepth = static_cast<unsigned int>(cFile.ParseInt( ));
                break;
            case File_ForkEnabled:
                m_bForkingEnabled = cFile.ParseBool( );
                break;
            case File_ForkBias:
                m_fForkBias = cFile.ParseFloat( );
                break;
            case File_ForkAngle:
                m_fForkAngle = cFile.ParseFloat( );
                break;
            case File_ForkLimit:
                m_nForkLimit = cFile.ParseInt( );
                break;
            case File_FlareSegmentPackingExponent:
                m_fSegmentPackingExponent = cFile.ParseFloat( );
                break;
            case File_FlareNumFlares:
                m_nNumFlares = cFile.ParseInt( );
                break;
            case File_FlareBalance:
                m_fFlareBalance = cFile.ParseFloat( );
                break;
            case File_FlareRadialInfluence:
                m_fRadialInfluence = cFile.ParseFloat( );
                break;
            case File_FlareRadialInfluenceVariance:
                m_fRadialInfluenceVariance = cFile.ParseFloat( );
                break;
            case File_FlareRadialExponent:
                m_fRadialExponent = cFile.ParseFloat( );
                break;
            case File_FlareRadialDistance:
                m_fRadialDistance = cFile.ParseFloat( );
                break;
            case File_FlareRadialDistanceVariance:
                m_fRadialVariance = cFile.ParseFloat( );
                break;
            case File_FlareLengthDistance:
                m_fLengthDistance = cFile.ParseFloat( );
                break;
            case File_FlareLengthDistanceVariance:
                m_fLengthVariance = cFile.ParseFloat( );
                break;
            case File_LengthExponent:
                m_fLengthExponent = cFile.ParseFloat( );
                break;
            case File_TextureTwist:
                m_asTexCoordControls[CSpeedTreeRT::TL_DIFFUSE].m_fTwist = cFile.ParseFloat( );
                break;
            case File_TextureOffset:
                m_asTexCoordControls[CSpeedTreeRT::TL_DIFFUSE].m_bRandomTCoordOffset = cFile.ParseBool( );
                break;
            case File_LightSeamStartBias:
                m_fLightSeamStartBias = cFile.ParseFloat( );
                break;
            case File_LightSeamEndBias:
                m_fLightSeamEndBias = cFile.ParseFloat( );
                break;
            case File_CrossSectionProfile:
                cFile.ParseBranchParameter(GetCrossSectionProfile( ));
                break;
            case File_LightSeamProfile:
                cFile.ParseBranchParameter(GetLightSeamProfile( ));
                break;
            case File_RoughnessProfile:
                cFile.ParseBranchParameter(GetRoughnessProfile( ));
                break;
            case File_FrequencyProfile:
                cFile.ParseBranchParameter(GetFrequencyProfile( ));
                break;
            case File_LightSeamBias:
                cFile.ParseBranchParameter(GetLightSeamBias( ));
                break;
            case File_GnarlProfile:
                cFile.ParseBranchParameter(GetGnarlProfile( ));
                break;
            case File_Gnarl:
                m_fGnarl = cFile.ParseFloat( );
                break;
            case File_GnarlUnison:
                m_bGnarlUnison = cFile.ParseBool( );
                break;
            case File_EndSupplementalBranchData:
                break;
            default:
                throw(IdvFileError("malformed supplemental branch information"));
        }
        nToken = cFile.ParseToken( );
    } while (nToken != File_EndSupplementalBranchData);
}


///////////////////////////////////////////////////////////////////////  
//  SIdvBranchInfo::SaveSupplemental

void SIdvBranchInfo::SaveSupplemental(CTreeFileAccess& cFile) const
{
    cFile.SaveToken(File_BeginSupplementalBranchData);

    cFile.SaveToken(File_TextureTwist);
    cFile.SaveFloat(m_asTexCoordControls[CSpeedTreeRT::TL_DIFFUSE].m_fTwist);

    cFile.SaveToken(File_TextureOffset);
    cFile.SaveBool(m_asTexCoordControls[CSpeedTreeRT::TL_DIFFUSE].m_bRandomTCoordOffset);

    cFile.SaveToken(File_FlareSegmentPackingExponent);
    cFile.SaveFloat(m_fSegmentPackingExponent);

    cFile.SaveToken(File_FlareNumFlares);
    cFile.SaveInt(m_nNumFlares);

    cFile.SaveToken(File_FlareBalance);
    cFile.SaveFloat(m_fFlareBalance);

    cFile.SaveToken(File_FlareRadialInfluence);
    cFile.SaveFloat(m_fRadialInfluence);

    cFile.SaveToken(File_FlareRadialInfluenceVariance);
    cFile.SaveFloat(m_fRadialInfluenceVariance);

    cFile.SaveToken(File_FlareRadialExponent);
    cFile.SaveFloat(m_fRadialExponent);

    cFile.SaveToken(File_FlareRadialDistance);
    cFile.SaveFloat(m_fRadialDistance);

    cFile.SaveToken(File_FlareRadialDistanceVariance);
    cFile.SaveFloat(m_fRadialVariance);

    cFile.SaveToken(File_FlareLengthDistance);
    cFile.SaveFloat(m_fLengthDistance);

    cFile.SaveToken(File_FlareLengthDistanceVariance);
    cFile.SaveFloat(m_fLengthVariance);

    cFile.SaveToken(File_LengthExponent);
    cFile.SaveFloat(m_fLengthExponent);

    cFile.SaveToken(File_BranchRoughness);
    cFile.SaveFloat(m_fRoughness);

    cFile.SaveToken(File_BranchRoughnessVerticalFrequency);
    cFile.SaveFloat(m_fRoughnessVerticalFrequency);

    cFile.SaveToken(File_BranchRoughnessHorizontalFrequency);
    cFile.SaveFloat(m_fRoughnessHorizontalFrequency);

    cFile.SaveToken(File_BranchRandomRoughness);
    cFile.SaveFloat(m_fRandomRoughness);

    cFile.SaveToken(File_BranchMinLengthPercent);
    cFile.SaveFloat(m_fMinLengthPercent);

    cFile.SaveToken(File_BranchMinCrossSectionPercent);
    cFile.SaveFloat(m_fMinCrossSectionPercent);

    cFile.SaveToken(File_BranchPruningPercent);
    cFile.SaveFloat(m_fPruningPercent);

    cFile.SaveToken(File_BranchPruningDepth);
    cFile.SaveInt(m_uiPruningDepth);

    cFile.SaveToken(File_ForkEnabled);
    cFile.SaveBool(m_bForkingEnabled);

    cFile.SaveToken(File_ForkBias);
    cFile.SaveFloat(m_fForkBias);

    cFile.SaveToken(File_ForkAngle);
    cFile.SaveFloat(m_fForkAngle);

    cFile.SaveToken(File_ForkLimit);
    cFile.SaveInt(m_nForkLimit);

    cFile.SaveToken(File_CrossSectionProfile);
    cFile.SaveBranchParameter(&m_cCrossSectionProfile);

    cFile.SaveToken(File_LightSeamProfile);
    cFile.SaveBranchParameter(&m_cLightSeamProfile);

    cFile.SaveToken(File_RoughnessProfile);
    cFile.SaveBranchParameter(&m_cRoughnessProfile);

    cFile.SaveToken(File_FrequencyProfile);
    cFile.SaveBranchParameter(&m_cFrequencyProfile);

    cFile.SaveToken(File_LightSeamBias);
    cFile.SaveBranchParameter(&m_cLightSeamBias);

    cFile.SaveToken(File_GnarlProfile);
    cFile.SaveBranchParameter(&m_cGnarlProfile);

    cFile.SaveToken(File_Gnarl);
    cFile.SaveFloat(m_fGnarl);

    cFile.SaveToken(File_GnarlUnison);
    cFile.SaveBool(m_bGnarlUnison);

    cFile.SaveToken(File_EndSupplementalBranchData);
}
