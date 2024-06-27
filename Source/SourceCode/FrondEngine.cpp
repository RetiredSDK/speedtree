///////////////////////////////////////////////////////////////////////  
//  FrondEngine.cpp
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
#include "FrondEngine.h"
#include "IndexedGeometry.h"
#include "LightingEngine.h"
#include "LibRandom_Source\IdvRandom.h"
#include <algorithm>
#include "SpeedTreeRT.h"


using namespace std;


///////////////////////////////////////////////////////////////////////  
//  Static member variables

int CFrondEngine::m_nRefCount = 0;
st_vector_stVec3 CFrondEngine::m_vRightDirections;
st_vector_stVec3 CFrondEngine::m_vProfile;
st_vector_stVec3 CFrondEngine::m_vNormals;
st_vector_stVec3 CFrondEngine::m_vTangents;
st_vector_float CFrondEngine::m_vSideLengths;


///////////////////////////////////////////////////////////////////////  
//  Constants

const float c_fRepeatPreventionFactor = 0.999f;
const float c_fStabilityThreshold = 0.1f;
 

///////////////////////////////////////////////////////////////////////  
//  CFrondEngine::CFrondEngine definition

CFrondEngine::CFrondEngine( ) :
    m_pGeometry(NULL),
    m_pLightingEngine(NULL),
    m_eFrondType(CFrondEngine::FROND_EXTRUSION),
    m_nNumBlades(2),
    m_nProfileSegments(4),
    m_nLevel(1),
    m_bEnabled(false),
    m_nNumLodLevels(4),
    m_fMaxSurfaceAreaPercent(1.0f),
    m_fMinSurfaceAreaPercent(0.0f),
    m_fReductionFuzziness(0.0f),
    m_fLargeRetentionPercent(0.05f),
    m_nMinLengthSegments(2),
    m_nMinCrossSegments(1),
    m_uiDistanceLevel(0),
    m_fDistancePercent(0.0f),
    m_eAboveCondition(ENABLED),
    m_eBelowCondition(ENABLED),
    m_uiSegmentOverride(1),
    m_bUseSegmentOverride(false),
    m_bProhibitSegmentReduction(false)
{
    m_pProfile = new ("CFrondEngine::CFrondEngine, m_pProfile") stBezierSpline(st_string("BezierSpline 0.0 1.0 0.0 { 3 0 0.00138887 0.337009 0.941501 0.132767 0.493215 0.998903 1 0.00102074 0.23702 1 -6.24607e-008 0.307222 -0.951638 0.126974 }"));
    if (CSpeedTreeRT::GetMemoryPreference( ) == CSpeedTreeRT::MP_FAVOR_LESS_FRAGMENTATION)
        m_vGuides.reserve(300);
    ++m_nRefCount;
}


///////////////////////////////////////////////////////////////////////  
//  CFrondEngine::CFrondEngine

CFrondEngine::CFrondEngine(const CFrondEngine& cRight)
{
    *this = cRight;
    ++m_nRefCount;
}


///////////////////////////////////////////////////////////////////////  
//  CFrondEngine::~CFrondEngine definition

CFrondEngine::~CFrondEngine( )
{
    st_delete<stBezierSpline>(m_pProfile, "~CFrondEngine, m_pProfile");

    if (--m_nRefCount == 0)
    {
        // dealloc vectors
        st_vector_stVec3( ).swap(m_vRightDirections);
        st_vector_stVec3( ).swap(m_vProfile);
        st_vector_stVec3( ).swap(m_vNormals);
        st_vector_stVec3( ).swap(m_vTangents);
        st_vector_float( ).swap(m_vSideLengths);
    }
    st_assert(m_nRefCount >= 0);
}


///////////////////////////////////////////////////////////////////////  
//  CFrondEngine::GetLevel definition

int CFrondEngine::GetLevel(void) const
{
    return m_nLevel;
}


///////////////////////////////////////////////////////////////////////  
//  CFrondEngine::Enabled definition

bool CFrondEngine::Enabled(void) const
{
    return m_bEnabled;
}


///////////////////////////////////////////////////////////////////////  
//  CFrondEngine::Parse definition

void CFrondEngine::Parse(CTreeFileAccess& cFile)
{
    int nToken = cFile.ParseToken( );
    do
    {
        switch (nToken)
        {
        case File_FrondLevel:
            m_nLevel = cFile.ParseInt( );
            break;
        case File_FrondType:
            m_eFrondType = static_cast<EFrondType>(cFile.ParseInt( ));
            break;
        case File_FrondNumBlades:
            m_nNumBlades = cFile.ParseInt( );
            break;
        case File_FrondProfile:
            cFile.ParseBranchParameter(GetProfile( ));
            break;
        case File_FrondProfileSegments:
            m_nProfileSegments = cFile.ParseInt( );
            break;
        case File_FrondEnabled:
            m_bEnabled = cFile.ParseBool( );
            break;
        case File_FrondNumLodLevels:
            m_nNumLodLevels = cFile.ParseInt( );
            break;
        case File_FrondMaxSurfaceAreaPercent:
            m_fMaxSurfaceAreaPercent = cFile.ParseFloat( );
            break;
        case File_FrondMinSurfaceAreaPercent:
            m_fMinSurfaceAreaPercent = cFile.ParseFloat( );
            break;
        case File_FrondReductionFuzziness:
            m_fReductionFuzziness = cFile.ParseFloat( );
            break;
        case File_FrondLargeFrondPercent:
            m_fLargeRetentionPercent = cFile.ParseFloat( );
            break;
        case File_FrondMinLengthSegments:
            m_nMinLengthSegments = cFile.ParseInt( );
            break;
        case File_FrondMinCrossSegments:
            m_nMinCrossSegments = cFile.ParseInt( );
            break;
        case File_FrondNumTextures:
            {
                st_vector_frondtexture( ).swap(m_vTextures); // dealloc vector
                int nNumTextures = cFile.ParseInt( );
                for (int i = 0; i < nNumTextures; ++i)
                {
                    SFrondTexture sFrond;
                    nToken = cFile.ParseToken( ); // File_BeginFrondTexture
                    nToken = cFile.ParseToken( );
                    do
                    {
                        switch (nToken)
                        {
                            case File_FrondTextureFilename:
                                cFile.ParseString(sFrond.m_strFilename);
                                sFrond.m_strFilename = IdvNoPath(sFrond.m_strFilename);
                                break;
                            case File_FrondTextureAspectRatio:
                                sFrond.m_fAspectRatio = cFile.ParseFloat( );
                                break;
                            case File_FrondTextureSizeScale:
                                sFrond.m_fSizeScale = cFile.ParseFloat( );
                                break;
                            case File_FrondTextureMinAngleOffset:
                                sFrond.m_fMinAngleOffset = cFile.ParseFloat( );
                                break;
                            case File_FrondTextureMaxAngleOffset:
                                sFrond.m_fMaxAngleOffset = cFile.ParseFloat( );
                                break;
                            default:
                                throw(IdvFileError(IdvFormatString("malformed frond texture information (token %d)", nToken)));
                                break;
                        }
                        nToken = cFile.ParseToken( );
                    } while (nToken != File_EndFrondTexture);

                    m_vTextures.push_back(sFrond);
                }
            }
            break;
        default:
            throw(IdvFileError(IdvFormatString("malformed frond info (token %d)", nToken)));
        }
        nToken = cFile.ParseToken( );
    } while (nToken != File_EndFrondInfo);
}


///////////////////////////////////////////////////////////////////////  
//  CFrondEngine::Save definition

void CFrondEngine::Save(CTreeFileAccess& cFile) const
{
    cFile.SaveToken(File_BeginFrondInfo);

    cFile.SaveToken(File_FrondLevel);
    cFile.SaveInt(m_nLevel);

    cFile.SaveToken(File_FrondType);
    cFile.SaveInt(static_cast<int>(m_eFrondType));

    cFile.SaveToken(File_FrondNumBlades);
    cFile.SaveInt(m_nNumBlades);

    cFile.SaveToken(File_FrondProfile);
    cFile.SaveBranchParameter(m_pProfile);

    cFile.SaveToken(File_FrondProfileSegments);
    cFile.SaveInt(m_nProfileSegments);

    cFile.SaveToken(File_FrondEnabled);
    cFile.SaveBool(m_bEnabled);

    cFile.SaveToken(File_FrondNumLodLevels);
    cFile.SaveInt(m_nNumLodLevels);

    cFile.SaveToken(File_FrondMaxSurfaceAreaPercent);
    cFile.SaveFloat(m_fMaxSurfaceAreaPercent);

    cFile.SaveToken(File_FrondMinSurfaceAreaPercent);
    cFile.SaveFloat(m_fMinSurfaceAreaPercent);

    cFile.SaveToken(File_FrondReductionFuzziness);
    cFile.SaveFloat(m_fReductionFuzziness);

    cFile.SaveToken(File_FrondLargeFrondPercent);
    cFile.SaveFloat(m_fLargeRetentionPercent);

    cFile.SaveToken(File_FrondMinLengthSegments);
    cFile.SaveInt(m_nMinLengthSegments);

    cFile.SaveToken(File_FrondMinCrossSegments);
    cFile.SaveInt(m_nMinCrossSegments);

    cFile.SaveToken(File_FrondNumTextures);
    cFile.SaveInt(int(m_vTextures.size( )));

    for (unsigned int i = 0; i < m_vTextures.size( ); ++i)
    {
        cFile.SaveToken(File_BeginFrondTexture);

        cFile.SaveToken(File_FrondTextureFilename);
        cFile.SaveString(m_vTextures[i].m_strFilename);

        cFile.SaveToken(File_FrondTextureAspectRatio);
        cFile.SaveFloat(m_vTextures[i].m_fAspectRatio);

        cFile.SaveToken(File_FrondTextureSizeScale);
        cFile.SaveFloat(m_vTextures[i].m_fSizeScale);

        cFile.SaveToken(File_FrondTextureMinAngleOffset);
        cFile.SaveFloat(m_vTextures[i].m_fMinAngleOffset);

        cFile.SaveToken(File_FrondTextureMaxAngleOffset);
        cFile.SaveFloat(m_vTextures[i].m_fMaxAngleOffset);

        cFile.SaveToken(File_EndFrondTexture);
    }

    cFile.SaveToken(File_EndFrondInfo);
}


///////////////////////////////////////////////////////////////////////  
//  CFrondEngine::StartGuide definition

void CFrondEngine::StartGuide(int nNumVertices)
{
    m_vGuides.push_back(SFrondGuide( ));

    m_vGuides.back( ).m_nNumVertices = nNumVertices;
    if (nNumVertices <= SFrondGuide::c_nMaxStackVertices)
    {
        m_vGuides.back( ).m_pVertices = m_vGuides.back( ).m_asStackVertices;
    }
    else
    {
        m_vGuides.back( ).m_vVertices.resize(nNumVertices);
        m_vGuides.back( ).m_pVertices = &(m_vGuides.back( ).m_vVertices)[0];
    }
}


///////////////////////////////////////////////////////////////////////  
//  CFrondEngine::AddGuideVertex definition

void CFrondEngine::AddGuideVertex(stVec3 cPos, stRotTransform cTrans, float fCrossSectionWeight1, float fCrossSectionWeight2, int nWindGroup1, int nWindGroup2)
{
    SFrondVertex sVertex;
    sVertex.m_cPos = cPos;
    sVertex.m_cTrans = cTrans;
    sVertex.m_fCrossSectionWeight1 = fCrossSectionWeight1;
    sVertex.m_fCrossSectionWeight2 = fCrossSectionWeight2;
    sVertex.m_nWindGroup1 = nWindGroup1;
    sVertex.m_nWindGroup2 = nWindGroup2;

    m_vGuides.back( ).m_pVertices[m_vGuides.back( ).m_nCurrentIndex++] = sVertex;
}


///////////////////////////////////////////////////////////////////////  
//  CFrondEngine::EndGuide definition

void CFrondEngine::EndGuide(float fLodSizeScalar)
{
    SFrondGuide& sGuide = m_vGuides[m_vGuides.size( ) - 1];

    int i = 0;
    for (i = 0; i < sGuide.m_nCurrentIndex - 1; ++i)
        sGuide.m_fLength += sGuide.m_pVertices[i].m_cPos.Distance(sGuide.m_pVertices[i + 1].m_cPos);

    // choose a frond map, compute the radius and start angle
    stRandom cRandom;
    if (m_vTextures.size( ) == 0)
    {
        m_vGuides[i].m_chFrondMapIndex = 0;
        sGuide.m_fRadius = sGuide.m_fLength * 0.5f;
        sGuide.m_fOffsetAngle = 0.0f;
    }
    else
    {
        sGuide.m_chFrondMapIndex = static_cast<unsigned char>(static_cast<unsigned int>(cRandom.GetUniform(0.0f, 100000.0f)) % m_vTextures.size( ));
        sGuide.m_fRadius = sGuide.m_fLength * m_vTextures[static_cast<int>(sGuide.m_chFrondMapIndex)].m_fAspectRatio * 0.5f;
        sGuide.m_fRadius *= m_vTextures[sGuide.m_chFrondMapIndex].m_fSizeScale;
        sGuide.m_fOffsetAngle = cRandom.GetUniform(m_vTextures[sGuide.m_chFrondMapIndex].m_fMinAngleOffset, m_vTextures[sGuide.m_chFrondMapIndex].m_fMaxAngleOffset);
        if (m_vGuides.size( ) % 2 == 1)
            sGuide.m_fOffsetAngle *= -1.0f;
    }

    sGuide.m_fSurfaceArea = sGuide.m_fLength * fLodSizeScalar;
}


///////////////////////////////////////////////////////////////////////  
//  CFrondEngine::Compute definition

void CFrondEngine::Compute(CIndexedGeometry* pGeometry, CLightingEngine* pLightingEngine)
{
    m_pGeometry = pGeometry;
    m_pLightingEngine = pLightingEngine;

    st_assert(m_pGeometry);
    st_assert(m_pLightingEngine);

    int nVertices = 0;
    BuildGuideLods( );
    for (unsigned int j = 0; j < m_vGuideLods[0].size( ); ++j)
    {
        switch (m_eFrondType)
        {
        case FROND_BLADES:
            // all blade lods reference the same vertices so only report them once
            nVertices += int(m_vGuideLods[0][j]->m_nCurrentIndex) * 2 * m_nNumBlades;
            break;
        case FROND_EXTRUSION:
            // all extrusion lods reference the same vertices so only report them once
            nVertices += int(m_vGuideLods[0][j]->m_nCurrentIndex) * ((m_nProfileSegments * 2) - 1);
            break;
        default:
            throw(IdvFileError("default reached in CFrondEngine::Compute()"));
        }
    }

    if (nVertices > USHRT_MAX)
        throw(IdvRuntimeError(IdvFormatString("frond vertices exceed %d", USHRT_MAX)));

    m_pGeometry->SetNumLodLevels(m_nNumLodLevels);

    switch (m_eFrondType)
    {
    case FROND_BLADES:
    {
        // build vertices for highest lod (lower lods share them)
        for (unsigned int i = 0; i < m_vGuideLods[0].size( ); ++i)
            BuildBladeVertices(*m_vGuideLods[0][i]);

        // build all strips
        for (int m = 0; m < int(m_vGuideLods.size( )); ++m)
        {
            m_pGeometry->ResetStripCounter( );
            for (int k = 0; k < int(m_vGuideLods[m].size( )); ++k)
                ComputeBlade(m, m_vGuideLods[0][k]->m_nVertexStartIndex, *m_vGuideLods[m][k]);
        }
        break;
    }
    case FROND_EXTRUSION:
    {
        // build vertices for highest lod (lower lods share them)
        for (unsigned int i = 0; i < m_vGuideLods[0].size( ); ++i)
            BuildExtrusionVertices(*m_vGuideLods[0][i]);

        for (int l = 0; l < int(m_vGuideLods.size( )); ++l)
        {
            m_pGeometry->ResetStripCounter( );

            for (size_t k = 0; k < m_vGuideLods[l].size( ); ++k)
            {
                m_vGuideLods[l][k]->m_nVerticesPerGuideVertex = m_vGuideLods[0][k]->m_nVerticesPerGuideVertex;
                ComputeExtrusion(l, m_vGuideLods[0][k]->m_nVertexStartIndex, *m_vGuideLods[l][k]);
            }
        }
        break;
    }
    default:
        throw(IdvFileError("default reached in CFrondEngine::Compute()"));
    }
}


///////////////////////////////////////////////////////////////////////  
//  CFrondEngine::BuildBladeVertices definition

void CFrondEngine::BuildBladeVertices(SFrondGuide& sGuide)
{
    if (m_pGeometry && m_pLightingEngine)
    {
        float fAngleBetween = 180.0f / m_nNumBlades;

        sGuide.m_nVertexStartIndex = m_pGeometry->GetVertexCounter( );
        sGuide.m_nVerticesPerGuideVertex = 2;
        m_vRightDirections.resize(0);
        if (m_vRightDirections.capacity( ) < 10)
            m_vRightDirections.reserve(10);

        // declare these outside to reduce fragmentation
        m_vSideLengths.resize(0);
        if (m_vSideLengths.capacity( ) < 16)
            m_vSideLengths.reserve(16);
        for (unsigned int k = 0; k < m_nNumBlades; ++k)
        {
            int nLocalStart = m_pGeometry->GetVertexCounter( );
            // add vertices
            int nNumVertices = sGuide.m_nCurrentIndex;
            m_vRightDirections.resize(nNumVertices);
            int i = 0;
            for (i = 0; i < nNumVertices; ++i)
            {
                // get the matrix for these vertices
                stRotTransform cTrans;
                if (i < sGuide.m_nCurrentIndex - 1)
                    cTrans = sGuide.m_pVertices[i].m_cTrans;
                else
                    cTrans = sGuide.m_pVertices[i - 1].m_cTrans;
                cTrans.RotateX(fAngleBetween * k + sGuide.m_fOffsetAngle);

                // compute directions
                stVec3 cRightDir = stVec3(0.0f, 1.0f, 0.0f) * cTrans;
                m_vRightDirections[i] = cRightDir;
                
                // compute normal
                stVec3 cNormal;
                if (i == 0)
                    cNormal = sGuide.m_pVertices[i + 1].m_cPos - sGuide.m_pVertices[0].m_cPos;
                else
                    cNormal = sGuide.m_pVertices[i].m_cPos - sGuide.m_pVertices[0].m_cPos;
                cNormal.Normalize( );

                // compute vertices
                stVec3 cRight, cLeft;

                if (i == sGuide.m_nCurrentIndex - 1 && sGuide.m_nCurrentIndex > 0)
                {
                    cRight = sGuide.m_pVertices[i].m_cPos + m_vRightDirections[i - 1] * sGuide.m_fRadius;
                    cLeft = sGuide.m_pVertices[i].m_cPos - m_vRightDirections[i - 1] * sGuide.m_fRadius;
                }
                else if (i > 0 && i < sGuide.m_nCurrentIndex - 1)
                {
                    stVec3 cTemp;

                    // right
                    cTemp = sGuide.m_pVertices[i].m_cPos + m_vRightDirections[i - 1] * sGuide.m_fRadius;
                    cRight = sGuide.m_pVertices[i].m_cPos + cRightDir * sGuide.m_fRadius;
                    cRight = (cRight + cTemp) * 0.5f;

                    // left
                    cTemp = sGuide.m_pVertices[i].m_cPos - m_vRightDirections[i - 1] * sGuide.m_fRadius;
                    cLeft = sGuide.m_pVertices[i].m_cPos - cRightDir * sGuide.m_fRadius;
                    cLeft = (cLeft + cTemp) * 0.5f;
                }
                else
                {
                    cRight = sGuide.m_pVertices[i].m_cPos + cRightDir * sGuide.m_fRadius;
                    cLeft = sGuide.m_pVertices[i].m_cPos - cRightDir * sGuide.m_fRadius;
                }

                float fLengthPercent = static_cast<float>(i) / (sGuide.m_nCurrentIndex - 1.0f);

                // add right vertex
                m_pGeometry->AddVertexCoord(cRight.m_afData);

                float afTexCoords[2];
                afTexCoords[0] = 1.0f;
                afTexCoords[1] = fLengthPercent;
                m_pGeometry->AddVertexTexCoord(CSpeedTreeRT::TL_DIFFUSE, afTexCoords, sGuide.m_chFrondMapIndex);

                if (m_pLightingEngine->GetFrondLightingMethod( ) == CSpeedTreeRT::LIGHT_STATIC)
                {
                    float afColor[4];
                    m_pLightingEngine->ComputeStandardStaticLighting(cNormal.m_afData, cRight.m_afData, afColor, CLightingEngine::MATERIAL_FROND);
                    m_pGeometry->AddVertexColor(afColor);
                }
                else
                {
                    m_pGeometry->AddVertexNormal(cNormal.m_afData);

                    // bump mapping
                    m_pGeometry->AddVertexTangent(cRightDir);
                    stVec3 cBinormal = cNormal * cRightDir;
                    if (cBinormal.Magnitude( ) < c_fStabilityThreshold)
                        cBinormal= cNormal;
                    else
                        cBinormal.Normalize( );
                    m_pGeometry->AddVertexBinormal(-cBinormal);
                }

                if (m_pGeometry->IsVertexWeightingEnabled( ))
                {
                    m_pGeometry->AddVertexWind1(sGuide.m_pVertices[i].m_fCrossSectionWeight1, static_cast<unsigned char>(sGuide.m_pVertices[i].m_nWindGroup1));
                    m_pGeometry->AddVertexWind2(sGuide.m_pVertices[i].m_fCrossSectionWeight2, static_cast<unsigned char>(sGuide.m_pVertices[i].m_nWindGroup2));
                }

                m_pGeometry->AdvanceVertexCounter( );

                // add left vertex
                m_pGeometry->AddVertexCoord(cLeft.m_afData);

                afTexCoords[0] = 0.0f;
                m_pGeometry->AddVertexTexCoord(CSpeedTreeRT::TL_DIFFUSE, afTexCoords, sGuide.m_chFrondMapIndex);

                if (m_pLightingEngine->GetFrondLightingMethod( ) == CSpeedTreeRT::LIGHT_STATIC)
                {
                    float afColor[4];
                    m_pLightingEngine->ComputeStandardStaticLighting(cNormal.m_afData, cLeft.m_afData, afColor, CLightingEngine::MATERIAL_FROND);
                    m_pGeometry->AddVertexColor(afColor);
                }
                else
                {
                    m_pGeometry->AddVertexNormal(cNormal.m_afData);

                    // bump mapping
                    m_pGeometry->AddVertexTangent(cRightDir);
                    stVec3 cBinormal = cNormal * cRightDir;
                    if (cBinormal.Magnitude( ) < c_fStabilityThreshold)
                        cBinormal= cNormal;
                    else
                        cBinormal.Normalize( );
                    m_pGeometry->AddVertexBinormal(-cBinormal);
                }

                if (m_pGeometry->IsVertexWeightingEnabled( ))
                {
                    m_pGeometry->AddVertexWind1(sGuide.m_pVertices[i].m_fCrossSectionWeight1, static_cast<unsigned char>(sGuide.m_pVertices[i].m_nWindGroup1));
                    m_pGeometry->AddVertexWind2(sGuide.m_pVertices[i].m_fCrossSectionWeight2, static_cast<unsigned char>(sGuide.m_pVertices[i].m_nWindGroup2));
                }

                m_pGeometry->AdvanceVertexCounter( );
            }
            
            // compute lengths for t coord adjustment
            float fRightLength = 0.0f, fLeftLength = 0.0f;
            unsigned int nSize = (unsigned int) sGuide.m_nCurrentIndex;
            if (nSize > 1)
            {
                m_vSideLengths.resize(2 * (nSize - 1));
                for (i = 0; i < int(nSize - 1); ++i)
                {
                    const float* pThisCoord = m_pGeometry->GetVertexCoord(i * 2 + 0 + sGuide.m_nVertexStartIndex);
                    const float* pNextCoord = m_pGeometry->GetVertexCoord(i * 2 + 2 + sGuide.m_nVertexStartIndex);
                    stVec3 cThis(pThisCoord[0], pThisCoord[1], pThisCoord[2]);
                    stVec3 cNext(pNextCoord[0], pNextCoord[1], pNextCoord[2]);
                    fRightLength += cThis.Distance(cNext);
                    m_vSideLengths[i * 2] = fRightLength;

                    pThisCoord = m_pGeometry->GetVertexCoord(i * 2 + 1 + sGuide.m_nVertexStartIndex);
                    pNextCoord = m_pGeometry->GetVertexCoord(i * 2 + 3 + sGuide.m_nVertexStartIndex);
                    cThis.Set(pThisCoord[0], pThisCoord[1], pThisCoord[2]);
                    cNext.Set(pNextCoord[0], pNextCoord[1], pNextCoord[2]);
                    fLeftLength += cThis.Distance(cNext);
                    m_vSideLengths[i * 2 + 1] = fLeftLength;
                }

                // set new, "unpinched" t coords (skip the first one since it is always zero)
                m_pGeometry->SetVertexCounter(nLocalStart + 2);
                for (i = 1; i < int(nSize - 1); ++i)
                {
                    const float* pTexCoord = m_pGeometry->GetVertexTexCoord(CSpeedTreeRT::TL_DIFFUSE, i * 2 + 0 + sGuide.m_nVertexStartIndex);
                    float afRightTexCoord[ ] = { pTexCoord[0], c_fRepeatPreventionFactor * (m_vSideLengths[2 * (i - 1)] / fRightLength) };
                    m_pGeometry->AddVertexTexCoord(CSpeedTreeRT::TL_DIFFUSE, afRightTexCoord, sGuide.m_chFrondMapIndex);
                    m_pGeometry->AdvanceVertexCounter( );

                    pTexCoord = m_pGeometry->GetVertexTexCoord(CSpeedTreeRT::TL_DIFFUSE, i * 2 + 1 + sGuide.m_nVertexStartIndex);
                    float afLeftTexCoord[ ] = { pTexCoord[0], c_fRepeatPreventionFactor * (m_vSideLengths[2 * (i - 1) + 1] / fLeftLength) };
                    m_pGeometry->AddVertexTexCoord(CSpeedTreeRT::TL_DIFFUSE, afLeftTexCoord, sGuide.m_chFrondMapIndex);
                    m_pGeometry->AdvanceVertexCounter( );
                }
            }

            // account for unmodified end coords
            m_pGeometry->AdvanceVertexCounter( );
            m_pGeometry->AdvanceVertexCounter( );
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  CFrondEngine::ComputeBlade definition

void CFrondEngine::ComputeBlade(unsigned int nLod, unsigned int nStart, SFrondGuide& sGuide)
{
    if (m_pGeometry)
    {
        for (unsigned int k = 0; k < m_nNumBlades; ++k)
        {
            // build strips
            int nIndexCount = sGuide.m_nCurrentIndex * 2;

            st_vector_int vStrip(sGuide.m_nCurrentIndex * 2);
            int i = 0;
            for (i = 0; i < sGuide.m_nCurrentIndex * 2; ++i)
                vStrip[i] = int(nStart + (k * 2 * sGuide.m_nCurrentIndex) + i);

            m_pGeometry->AddStrip(nLod, &vStrip[0], nIndexCount);
            m_pGeometry->AdvanceStripCounter( );

            // set fade hints
            const float c_fLodFadeHint = (nLod == m_nNumLodLevels - 1) ? -1.0f : 1.0f - float(nLod + 1) / m_nNumLodLevels;
            for (i = 0; i < nIndexCount; ++i)
                m_pGeometry->SetLodFadeHint(vStrip[i], c_fLodFadeHint);
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  CFrondEngine::BuildExtrusionVertices definition

void CFrondEngine::BuildExtrusionVertices(SFrondGuide& sGuide)
{
    if (m_pGeometry && m_pLightingEngine)
    {
        if (m_vProfile.capacity( ) < 3)
        {
            m_vProfile.reserve(3);
            m_vNormals.reserve(3);
            m_vTangents.reserve(3);
        }
        BuildProfileVectors(sGuide, m_vProfile, m_vNormals, m_vTangents);

        // build vertex table
        sGuide.m_nVerticesPerGuideVertex = int(m_vProfile.size( ));
        sGuide.m_nVertexStartIndex = int(m_pGeometry->GetVertexCounter( ));

        // run through points defining the spine of the frond
        int i = 0;
        for (i = 0; i < sGuide.m_nCurrentIndex; ++i)
        {
            // T texcoords
            st_assert(sGuide.m_nCurrentIndex != 0);
            float fT = static_cast<float>(i) / (sGuide.m_nCurrentIndex - 1.0f);

            // run through the points on either side of point in spine (calculate
            // two points, one on each side)
            for (unsigned int j = 0; j < sGuide.m_nVerticesPerGuideVertex; ++j)
            {
                // S texcoords
                st_assert(sGuide.m_nVerticesPerGuideVertex != 1);
                float fS = j / (sGuide.m_nVerticesPerGuideVertex - 1.0f);

                // position
                stRotTransform cTrans = sGuide.m_pVertices[i].m_cTrans;
                stRotTransform cPreviousTrans = i ? sGuide.m_pVertices[i - 1].m_cTrans : cTrans;

                cTrans.RotateX(sGuide.m_fOffsetAngle);
                cPreviousTrans.RotateX(sGuide.m_fOffsetAngle);

                stVec3 cPos1 = m_vProfile[j] * cTrans, cPos2 = m_vProfile[j] * cPreviousTrans;
                stVec3 cPos = sGuide.m_pVertices[i].m_cPos + ((cPos1 + cPos2) * 0.5f);

                m_pGeometry->AddVertexCoord(cPos.m_afData);
                
                float afTexCoords[2];
                afTexCoords[0] = fS;
                afTexCoords[1] = fT;
                m_pGeometry->AddVertexTexCoord(CSpeedTreeRT::TL_DIFFUSE, afTexCoords, sGuide.m_chFrondMapIndex);

                // normal
                stVec3 cNormal1 = m_vNormals[j] * cTrans, cNormal2 = m_vNormals[j] * cPreviousTrans;
                stVec3 cNormal = (cNormal1 + cNormal2) * 0.5f;
                cNormal.Normalize( );

                // tangent
                stVec3 cTangent1 = m_vTangents[j] * cTrans, cTangent2 = m_vTangents[j] * cPreviousTrans;
                stVec3 cTangent = (cTangent1 + cTangent2) * 0.5f;
                cTangent.Normalize( );

                if (m_pLightingEngine->GetFrondLightingMethod( ) == CSpeedTreeRT::LIGHT_STATIC)
                {
                    float afColor[4];
                    m_pLightingEngine->ComputeStandardStaticLighting(cNormal.m_afData, cPos.m_afData, afColor, CLightingEngine::MATERIAL_FROND);
                    m_pGeometry->AddVertexColor(afColor);
                }
                else
                {
                    m_pGeometry->AddVertexNormal(cNormal);

                    // bump mapping
                    m_pGeometry->AddVertexTangent(cTangent);

                    stVec3 cBinormal = cTangent * cNormal;
                    // fp problems?
                    if (cBinormal.Magnitude( ) < c_fStabilityThreshold)
                        cBinormal= cNormal;
                    else
                        cBinormal.Normalize( );
                    m_pGeometry->AddVertexBinormal(cBinormal);
                }

                if (m_pGeometry->IsVertexWeightingEnabled( ))
                {
                    m_pGeometry->AddVertexWind1(sGuide.m_pVertices[i].m_fCrossSectionWeight1, static_cast<unsigned char>(sGuide.m_pVertices[i].m_nWindGroup1));
                    m_pGeometry->AddVertexWind2(sGuide.m_pVertices[i].m_fCrossSectionWeight2, static_cast<unsigned char>(sGuide.m_pVertices[i].m_nWindGroup2));
                }

                m_pGeometry->AdvanceVertexCounter( );
            }
        }

        // compute lengths for t coord adjustment
        if (sGuide.m_nVerticesPerGuideVertex > 0)
        {
            if (m_vLengths.size( ) < sGuide.m_nVerticesPerGuideVertex)
                m_vLengths.resize(sGuide.m_nVerticesPerGuideVertex);
            if (m_vRunningLengths.size( ) < sGuide.m_nVerticesPerGuideVertex)
                m_vRunningLengths.resize(sGuide.m_nVerticesPerGuideVertex);

            for (i = 0; i < (int) sGuide.m_nVerticesPerGuideVertex; ++i)
            {
                float fLength = 0.0f;
                size_t nNumVertices = sGuide.m_nCurrentIndex;

                if (m_vRunningLengths[i].size( ) < nNumVertices)
                    m_vRunningLengths[i].resize(nNumVertices);

                if (nNumVertices > 0)
                    m_vRunningLengths[i][0] = 0.0f;
                for (unsigned int j = 1; j < nNumVertices; ++j)
                {
                    int nThisIndex = sGuide.m_nVertexStartIndex + (j * sGuide.m_nVerticesPerGuideVertex) + i;
                    int nPrevIndex = sGuide.m_nVertexStartIndex + ((j - 1) * sGuide.m_nVerticesPerGuideVertex) + i;

                    const float* pThisCoord = m_pGeometry->GetVertexCoord(nThisIndex);
                    const float* pPrevCoord = m_pGeometry->GetVertexCoord(nPrevIndex);
                    stVec3 cThis(pThisCoord[0], pThisCoord[1], pThisCoord[2]);
                    stVec3 cPrev(pPrevCoord[0], pPrevCoord[1], pPrevCoord[2]);
                    fLength += cThis.Distance(cPrev);
                    m_vRunningLengths[i][j] = fLength;
                }
                m_vLengths[i] = fLength;
            }

            // set new, "unpinched" t coords (skip the first one since it is always zero)
            for (i = 0; i < int(sGuide.m_nVerticesPerGuideVertex); ++i)
            {
                for (int j = 1; j < sGuide.m_nCurrentIndex; ++j)
                {
                    int nThisIndex = sGuide.m_nVertexStartIndex + (j * sGuide.m_nVerticesPerGuideVertex) + i;
                    m_pGeometry->SetVertexCounter(nThisIndex);

                    const float* pTexCoord = m_pGeometry->GetVertexTexCoord(CSpeedTreeRT::TL_DIFFUSE, nThisIndex);
                    float afNewTexCoord[ ] = { pTexCoord[0], c_fRepeatPreventionFactor * (m_vRunningLengths[i][j] / m_vLengths[i]) };
                    m_pGeometry->AddVertexTexCoord(CSpeedTreeRT::TL_DIFFUSE, afNewTexCoord, sGuide.m_chFrondMapIndex);
                    m_pGeometry->AdvanceVertexCounter( );
                }
            }
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  CFrondEngine::ComputeExtrusion definition

void CFrondEngine::ComputeExtrusion(unsigned int nLod, unsigned int nStart, SFrondGuide& sGuide)
{
    if (m_pGeometry)
    {
        // compute lod info
        float fLodPercent = (m_nNumLodLevels == 1) ? 1.0f : 1.0f - (nLod / (m_nNumLodLevels - 1.0f));

        int nNumLengthVertices = static_cast<int>(VecInterpolate(static_cast<float>(m_nMinLengthSegments + 1), static_cast<float>(sGuide.m_nCurrentIndex), fLodPercent)); // plus one for vertex conversion
        int nNumCrossVertices = static_cast<int>(VecInterpolate(static_cast<float>((m_nMinCrossSegments * 2) + 1), static_cast<float>(sGuide.m_nVerticesPerGuideVertex), fLodPercent)); // * 2 for mirroring, plus one for vertex conversion

        if (m_bProhibitSegmentReduction)
        {
            nNumLengthVertices = sGuide.m_nCurrentIndex;
            nNumCrossVertices = sGuide.m_nVerticesPerGuideVertex;
        }

        float fLengthStep = sGuide.m_nCurrentIndex / static_cast<float>(nNumLengthVertices - 1);  // -1 because one of them doesn't "step"
        float fCrossStep = (sGuide.m_nVerticesPerGuideVertex) / static_cast<float>(nNumCrossVertices - 1);  // -1 because one of them doesn't "step"

        // build strips
        int nSegments = nNumLengthVertices - 1;
        int nIndexCount = (nSegments * (nNumCrossVertices * 2 + 1)) - 1; // + 1 for repeated vertex, - 1 because we do not need to repeat the last one
        if (m_vCurrentStrip.capacity( ) < size_t(nIndexCount))
            m_vCurrentStrip.reserve(nIndexCount);
        m_vCurrentStrip.resize(nIndexCount);

        int nVertexIndex = 0;
        int i = 0;
        for (i = 0; i < nNumLengthVertices - 1; ++i)
        {
            int nThisLengthIndex = static_cast<int>(i * fLengthStep);
            int nNextLengthIndex = static_cast<int>((i + 1) * fLengthStep);

            if (i == nNumLengthVertices - 2 || nNextLengthIndex > static_cast<int>(sGuide.m_nCurrentIndex - 1))
                nNextLengthIndex = int(sGuide.m_nCurrentIndex) - 1;

            int nSkipFactor = nNextLengthIndex - nThisLengthIndex;

            if (i % 2 == 0)
            {
                int nBase = nThisLengthIndex * sGuide.m_nVerticesPerGuideVertex;
                for (int j = 0; j < nNumCrossVertices; ++j)
                {
                    unsigned int nThisCrossVertex = static_cast<unsigned int>(j * fCrossStep);

                    if (j == nNumCrossVertices - 1)
                        nThisCrossVertex = sGuide.m_nVerticesPerGuideVertex - 1;

                    if (nThisCrossVertex > sGuide.m_nVerticesPerGuideVertex - 1)
                        nThisCrossVertex = sGuide.m_nVerticesPerGuideVertex - 1;

                    m_vCurrentStrip[nVertexIndex++] = nStart + nBase + nThisCrossVertex;
                    m_vCurrentStrip[nVertexIndex++] = nStart + nBase + nThisCrossVertex + (nSkipFactor * sGuide.m_nVerticesPerGuideVertex);
                }

                // repeat this vertex so we can combine the multiple segments of one frond
                if (i < nNumLengthVertices - 2)
                {
                    m_vCurrentStrip[nVertexIndex] = m_vCurrentStrip[nVertexIndex - 1];
                    ++nVertexIndex;
                }
            }
            else
            {
                int nBase = nThisLengthIndex * sGuide.m_nVerticesPerGuideVertex;
                for (int j = nNumCrossVertices - 1; j >= 0; --j)
                {
                    unsigned int nThisCrossVertex = static_cast<unsigned int>((j * fCrossStep));

                    if (j == nNumCrossVertices - 1)
                        nThisCrossVertex = sGuide.m_nVerticesPerGuideVertex - 1;

                    if (nThisCrossVertex > sGuide.m_nVerticesPerGuideVertex - 1)
                        nThisCrossVertex = sGuide.m_nVerticesPerGuideVertex - 1;

                    m_vCurrentStrip[nVertexIndex++] = nStart + nBase + nThisCrossVertex;
                    m_vCurrentStrip[nVertexIndex++] = nStart + nBase + nThisCrossVertex + (nSkipFactor * sGuide.m_nVerticesPerGuideVertex);
                }

                // repeat this vertex so we can combine the multiple segments of one frond
                if (i < nNumLengthVertices - 2)
                {
                    m_vCurrentStrip[nVertexIndex] = m_vCurrentStrip[nVertexIndex - 1];
                    ++nVertexIndex;
                }
            }
        }

        st_assert(!m_vCurrentStrip.empty( ));
        m_pGeometry->AddStrip(nLod, &(m_vCurrentStrip[0]), nIndexCount);
        m_pGeometry->AdvanceStripCounter( );

        // set fade hints
        const float c_fLodFadeHint = (nLod == m_nNumLodLevels - 1) ? -1.0f : 1.0f - float(nLod + 1) / m_nNumLodLevels;
        for (i = 0; i < nIndexCount; ++i)
            m_pGeometry->SetLodFadeHint(m_vCurrentStrip[i], c_fLodFadeHint);
    }
}


///////////////////////////////////////////////////////////////////////  
//  CFrondEngine::BuildProfileVectors definition

void CFrondEngine::BuildProfileVectors(SFrondGuide& sGuide, st_vector_stVec3& vProfile, st_vector_stVec3& vNormals, st_vector_stVec3& vTangents)
{
    // left side
    float fAdjust = m_pProfile->Evaluate(0.0f) * sGuide.m_fRadius;
    unsigned int i = 0; // declared outside of the for-scope for portability
    vProfile.resize(m_nProfileSegments * 2 - 1);
    for (i = 0; i < m_nProfileSegments; ++i)
    {
        float fPercent = 1.0f - (i / (m_nProfileSegments - 1.0f));
        vProfile[i][0] = 0.0f;
        vProfile[i][1] = -fPercent * sGuide.m_fRadius;
        vProfile[i][2] = (m_pProfile->Evaluate(fPercent) * sGuide.m_fRadius) - fAdjust;
    }

    // reverse for right side w/o duplicating center point
    for (int j = m_nProfileSegments - 2; j >= 0; --j)
    {
        stVec3 cVertex = vProfile[j];
        cVertex[1] *= -1.0f;
        vProfile[i++] = cVertex;
    }

    // build normal profile
    size_t nProfileSize = vProfile.size( );
    vNormals.resize(nProfileSize);
    vTangents.resize(nProfileSize);
    for (i = 0; i < nProfileSize; ++i)
    {
        float fAngle;
        if (i == m_nProfileSegments - 1)
        {
            fAngle = c_fHalfPi;
        }
        else if (i == 0)
        {
            float fNextAngle = atan2f(vProfile[i + 1][2] - vProfile[i][2], vProfile[i + 1][1] - vProfile[i][1]);
            fAngle = fNextAngle + c_fHalfPi;
        }
        else if (i == vProfile.size( ) - 1)
        {
            float fPreviousAngle = atan2f(vProfile[i][2] - vProfile[i - 1][2], vProfile[i][1] - vProfile[i - 1][1]);
            fAngle = fPreviousAngle + c_fHalfPi;
        }
        else
        {
            float fNextAngle = atan2f(vProfile[i + 1][2] - vProfile[i][2], vProfile[i + 1][1] - vProfile[i][1]);
            float fPreviousAngle = atan2f(vProfile[i][2] - vProfile[i - 1][2], vProfile[i][1] - vProfile[i - 1][1]);
            fAngle = (0.5f * (fPreviousAngle + fNextAngle)) + c_fHalfPi;
        }
        stVec3 cNormal(0.0f, cosf(fAngle), sinf(fAngle));
        cNormal.Normalize( );
        vNormals[i] = cNormal;
 
        fAngle += c_fHalfPi;
        stVec3 cTangent(0.0f, -cosf(fAngle), -sinf(fAngle));
        cTangent.Normalize( );
        vTangents[i] = cTangent;
    }
}


///////////////////////////////////////////////////////////////////////  
//  CFrondEngine::SetTextureCoords definition

void CFrondEngine::SetTextureCoords(CIndexedGeometry* pGeometry, unsigned int nFrondMapIndex, const float* pTexCoords, bool bFlip)
{
    // flip the coords if necessary
    float afCoords[8];
    memcpy(afCoords, pTexCoords, 8 * sizeof(float));
    if (bFlip)
    {
        for (int i = 1; i < 8; i +=2)
            afCoords[i] = -afCoords[i];
    }

    // check all of the vertices
    unsigned char chIndex = static_cast<unsigned char>(nFrondMapIndex);
    pGeometry->ResetVertexCounter( );
    for (int i = 0; i < pGeometry->GetVertexCount( ); ++i)
    {
        pGeometry->ChangeTexCoord(chIndex, afCoords);
        pGeometry->AdvanceVertexCounter( );
    }
}


//  class CGuideSorter definition
//
//  Function object used to sort the guides by surface area.

class CGuideSorter
{
public:
    bool operator()(const SFrondGuide* pLeft, const SFrondGuide* pRight)
    {
        return (pLeft->m_fFuzzySurfaceArea > pRight->m_fFuzzySurfaceArea);
    }
};


///////////////////////////////////////////////////////////////////////  
//  CFrondEngine::BuildGuideLods definition

void CFrondEngine::BuildGuideLods( )
{
    // query the size just once
    size_t nNumGuides = m_vGuides.size( );

    // compute total surface area of all fronds
    float fTotalArea = 0.0f;
    float fLargestArea = 0.0f;
    unsigned int i = 0; // declared outside of the for-scope for portability
    for (i = 0; i < nNumGuides; ++i)
    {
        fTotalArea += m_vGuides[i].m_fSurfaceArea;

        if (m_vGuides[i].m_fSurfaceArea > fLargestArea)
            fLargestArea = m_vGuides[i].m_fSurfaceArea;
    }

    // increase area to make fuzziness more effective (fronds do not typically benefit from the huge trunk)
    float fSquaredArea = fLargestArea * fLargestArea;

    // make a copy of m_vGuides, using pointers
    st_vector_frondguide_p vGuidesPtr(nNumGuides);
    for (i = 0; i < nNumGuides; ++i)
        vGuidesPtr[i] = &(m_vGuides[i]);

    // implement fuzziness
    stRandom cRandom;
    float fSavePercent = 1.0f - m_fLargeRetentionPercent;
    st_vector_frondguide_p vSavedFronds;
    for (i = 0; i < vGuidesPtr.size( ); ++i)
    {
        float fArea = vGuidesPtr[i]->m_fSurfaceArea;

        if (fArea > fLargestArea * fSavePercent)
        {
            vSavedFronds.push_back(vGuidesPtr[i]);
            (void) vGuidesPtr.erase(vGuidesPtr.begin( ) + i--);
        }
        else
        {
            // compute a new, fuzzy volume
            float fFuzziness = cRandom.GetUniform(0.0f, m_fReductionFuzziness), fOneMinusFuzziness = 1.0f - fFuzziness;
            fArea = (fOneMinusFuzziness * fArea) + (fFuzziness * fSquaredArea);
            vGuidesPtr[i]->m_fFuzzySurfaceArea = fArea;
        }
    }

    // sort branches in descending order of volume and add insert save branches at the beginning
    sort(vGuidesPtr.begin( ), vGuidesPtr.end( ), CGuideSorter( ));
    sort(vSavedFronds.begin( ), vSavedFronds.end( ), CGuideSorter( ));
    for (i = 0; i < vSavedFronds.size( ); ++i)
        (void) vGuidesPtr.insert(vGuidesPtr.begin( ), vSavedFronds[i]);

    // for each LOD level, compute a target volume and use enough branches to reach that target
    m_vGuideLods.resize(m_nNumLodLevels);
    for (i = 0; i < m_nNumLodLevels; ++i)
    {
        // figure what part of the branch structure stays for lod level i, computed by volume
        float fPercent;
        if (m_nNumLodLevels == 1)
            fPercent = m_fMaxSurfaceAreaPercent;
        else
            fPercent = VecInterpolate(m_fMaxSurfaceAreaPercent, m_fMinSurfaceAreaPercent, i / float(m_nNumLodLevels - 1));

        float fTargetVolume = fPercent * fTotalArea;

        st_vector_frondguide vLodLevel;
        if (fTargetVolume > 0.0f)
        {
            float fLodArea = 0.0f;
            size_t nComputedSize = 0;
            unsigned int j = 0;
            for (j = 0; j < m_vGuides.size( ) && fLodArea <= fTargetVolume; ++j)
            {
                fLodArea += vGuidesPtr[j]->m_fSurfaceArea;
                ++nComputedSize;
            }

            m_vGuideLods[i].resize(nComputedSize);

            for (j = 0; j < nComputedSize; ++j)
                m_vGuideLods[i][j] = vGuidesPtr[j];
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  CFrondEngine::GetRules definition

void CFrondEngine::GetRules(float& fDistance, unsigned int& uiLevel, EConditions& eAbove, EConditions& eBelow) const
{
    fDistance = m_fDistancePercent;
    uiLevel = m_uiDistanceLevel;
    eAbove = m_eAboveCondition;
    eBelow = m_eBelowCondition;
}



///////////////////////////////////////////////////////////////////////  
//  CFrondEngine::SaveSupplementalInfo definition

void CFrondEngine::SaveSupplementalInfo(CTreeFileAccess& cFile) const
{
    cFile.SaveToken(File_BeginSupplementalFrondInfo);

    cFile.SaveToken(File_FrondDistanceLevel);
    cFile.SaveInt(m_uiDistanceLevel);

    cFile.SaveToken(File_FrondDistancePercent);
    cFile.SaveFloat(m_fDistancePercent);

    cFile.SaveToken(File_FrondAboveCondition);
    cFile.SaveInt(static_cast<int>(m_eAboveCondition));

    cFile.SaveToken(File_FrondBelowCondition);
    cFile.SaveInt(static_cast<int>(m_eBelowCondition));

    cFile.SaveToken(File_FrondSegmentOverride);
    cFile.SaveInt(m_uiSegmentOverride);

    cFile.SaveToken(File_FrondUseSegmentOverride);
    cFile.SaveBool(m_bUseSegmentOverride);

    cFile.SaveToken(File_EndSupplementalFrondInfo);
}


///////////////////////////////////////////////////////////////////////  
//  CFrondEngine::ParseSupplementalInfo definition

void CFrondEngine::ParseSupplementalInfo(CTreeFileAccess& cFile)
{
    int nToken = cFile.ParseToken( );
    do
    {
        switch (nToken)
        {
        case File_FrondDistanceLevel:
            m_uiDistanceLevel = cFile.ParseInt( );
            break;
        case File_FrondDistancePercent:
            m_fDistancePercent = cFile.ParseFloat( );
            break;
        case File_FrondAboveCondition:
            m_eAboveCondition = static_cast<EConditions>(cFile.ParseInt( ));
            break;
        case File_FrondBelowCondition:
            m_eBelowCondition = static_cast<EConditions>(cFile.ParseInt( ));
            break;
        case File_FrondSegmentOverride:
            m_uiSegmentOverride = cFile.ParseInt( );
            break;
        case File_FrondUseSegmentOverride:
            m_bUseSegmentOverride = cFile.ParseBool( );
            break;
        default:
            throw(IdvFileError(IdvFormatString("malformed supplemental frond info (token %d)", nToken)));
        }
        nToken = cFile.ParseToken( );
    } while (nToken != File_EndSupplementalFrondInfo);
}


///////////////////////////////////////////////////////////////////////  
//  CFrondEngine::ExtrusionStripLength definition

int CFrondEngine::ExtrusionStripLength(unsigned int uiLod, const SFrondGuide& sGuide)
{
    float fLodPercent = (m_nNumLodLevels == 1) ? 1.0f : 1.0f - (uiLod / (m_nNumLodLevels - 1.0f));
    int nNumLengthVertices = static_cast<int>(VecInterpolate(static_cast<float>(m_nMinLengthSegments + 1), static_cast<float>(sGuide.m_nCurrentIndex), fLodPercent)); // plus one for vertex conversion
    int nNumCrossVertices = static_cast<int>(VecInterpolate(static_cast<float>((m_nMinCrossSegments * 2) + 1), static_cast<float>(sGuide.m_nVerticesPerGuideVertex), fLodPercent)); // * 2 for mirroring, plus one for vertex conversion

    if (m_bProhibitSegmentReduction)
    {
        nNumLengthVertices = sGuide.m_nCurrentIndex;
        nNumCrossVertices = sGuide.m_nVerticesPerGuideVertex;
    }

    // build strips
    int nSegments = nNumLengthVertices - 1;
    int nIndexCount = (nSegments * (nNumCrossVertices * 2 + 1)) - 1; // + 1 for repeated vertex, - 1 because we do not need to repeat the last one

    return nIndexCount;
}


///////////////////////////////////////////////////////////////////////  
//  CFrondEngine::ProhibitSegmentReduction definition

void CFrondEngine::ProhibitSegmentReduction(void)
{
    m_bProhibitSegmentReduction = true;
}
