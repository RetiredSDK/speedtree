///////////////////////////////////////////////////////////////////////  
//  IndexedGeometry.cpp
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

#ifdef WIN32
#pragma warning (disable : 4786)
#endif
#include "WindEngine.h"
#include "IndexedGeometry.h"
#include "UpVector.h"
#include "Endian.h"
#include "SpeedTreeRT.h"
using namespace std;


///////////////////////////////////////////////////////////////////////  
//  CIndexedGeometry::CIndexedGeometry definition

CIndexedGeometry::CIndexedGeometry(CWindEngine* pWindEngine, bool bRetainTexCoords) :
    m_bRetainTexCoords(bRetainTexCoords),
    m_pWindEngine(pWindEngine),
    m_bVertexWeighting(false),
    m_bManualLighting(false),
    m_pVertexWindComputed(NULL),
    m_nVertexSize(0),
    m_bValid(true),
    m_eWindMethod(CSpeedTreeRT::WIND_NONE),
    m_nNumLodLevels(0),
    m_nNumVertices(0),
    m_nNumVerticesLodLevel(0),
    m_nStripCounter(0),
    m_pNumStrips(NULL),
    m_pStripLengths(NULL),
    m_pStrips(NULL)
{
    st_assert(pWindEngine);

    if (CSpeedTreeRT::GetMemoryPreference( ) == CSpeedTreeRT::MP_FAVOR_LESS_FRAGMENTATION)
    {
        const int c_nStartingSize = 5000;
        m_vColors.reserve(c_nStartingSize);
        m_vCoords.reserve(c_nStartingSize * 3);
        m_vOrigCoords.reserve(c_nStartingSize * 3);
        m_vNormals.reserve(c_nStartingSize * 3);
        m_vBinormals.reserve(c_nStartingSize * 3);
        m_vTangents.reserve(c_nStartingSize * 3);
        m_vOrigTexCoords0.reserve(c_nStartingSize * 2);
        m_vTexIndices0.reserve(c_nStartingSize);
        m_vWindWeights1.reserve(c_nStartingSize);
        m_vWindMatrixIndices1.reserve(c_nStartingSize);
        m_vWindWeights2.reserve(c_nStartingSize);
        m_vWindMatrixIndices2.reserve(c_nStartingSize);
        m_vLodFadeHints.reserve(c_nStartingSize);

        for (int i = 0; i < static_cast<int>(CSpeedTreeRT::TL_NUM_TEX_LAYERS); ++i)
            m_avTexCoords[i].reserve(c_nStartingSize * 2);
    }
}


///////////////////////////////////////////////////////////////////////  
//  CIndexedGeometry::~CIndexedGeometry definition

CIndexedGeometry::~CIndexedGeometry( )
{
    st_delete_array<bool>(m_pVertexWindComputed, "~CIndexedGeometry, m_pVertexWindComputed");

    DeleteIndexData( );
}


///////////////////////////////////////////////////////////////////////  
//  CIndexedGeometry::CombineStrips definition
//
//  The geometry can be accessed through either m_pStrips or m_vAllStrips.  We'll prefer m_vAllStrips
//  to reduce fragmentation issues later on

void CIndexedGeometry::CombineStrips(bool bToggleFaceOrdering)
{
    // no need to concat empty strips
    if (GetVertexCount( ) > 0)
    {
        // compute composite length for all LODs together
        int nTotalCompositeLength = 0;
        int nLod = 0;
        for (nLod = 0; nLod < m_nNumLodLevels; ++nLod)
            nTotalCompositeLength += CompositeLength(nLod, bToggleFaceOrdering);
        int* pAllCompositeStrips = st_new_array<int>(nTotalCompositeLength, "CIndexedGeometry::CombineStrips, pAllCompositeStrips");

        // these will replace the class member variables m_pNumStrips, m_pStripLengths, and m_pStrips
        int* vCompositeStrips = pAllCompositeStrips;

        // for each LOD level, concat the strips together as one
        for (nLod = 0; nLod < m_nNumLodLevels; ++nLod)
        {
            // how many total indices are there for this LOD?
            int nCompositeLength = CompositeLength(nLod, bToggleFaceOrdering);

            if (nCompositeLength > 0)
            {
                int nNumStrips = int(m_vStripInfo[nLod].size( ) / 2);

                int* pStripPointer = vCompositeStrips;
                for (int nStrip = 0; nStrip < nNumStrips; ++nStrip)
                {
                    // copy the strip
                    int nStripLength = m_vStripInfo[nLod][nStrip * 2 + 1];
                    if (nStripLength > 0)
                    {
                        int nStripLocation = m_vStripInfo[nLod][nStrip * 2 + 0];
                        st_assert(!m_vAllStrips[nLod].empty( ));
                        st_assert(&(m_vAllStrips[nLod][nStripLocation]));
                        memcpy(pStripPointer, &(m_vAllStrips[nLod][nStripLocation]), nStripLength * sizeof(int));
                        pStripPointer += nStripLength;

                        // don't stitch if this is the last strip
                        if (nStrip < nNumStrips - 1)
                        {
                            if (bToggleFaceOrdering)
                            {
                                // need to stitch with different number of vertices for correct 2-sided lighting
                                if (nStripLength % 2 == 0)
                                {
                                    // add last vertex again
                                    *pStripPointer = *(pStripPointer - 1);
                                    ++pStripPointer;

                                    // add first vertex of next strip
                                    *pStripPointer++ = m_vAllStrips[nLod][nStripLocation + nStripLength];
                                }
                                else
                                {
                                    // add last vertex again, twice
                                    *pStripPointer = *(pStripPointer - 1);
                                    ++pStripPointer;
                                    *pStripPointer = *(pStripPointer - 1);
                                    ++pStripPointer;

                                    // add first vertex of next strip 
                                    *pStripPointer++ = m_vAllStrips[nLod][nStripLocation + nStripLength];
                                }
                            }
                            else
                            {
                                // add last vertex again
                                *pStripPointer = *(pStripPointer - 1);
                                ++pStripPointer;

                                // add first vertex of next strip
                                *pStripPointer++ = m_vAllStrips[nLod][nStripLocation + nStripLength];
                            }
                        }
                    }
                }
                st_assert(pStripPointer - vCompositeStrips == nCompositeLength);

                // force values to reflect single strip data
                m_vStripInfo[nLod].resize(2);
                m_vStripInfo[nLod][0] = 0;
                m_vStripInfo[nLod][1] = nCompositeLength;
                m_vTriangleCounts[nLod] = nCompositeLength > 2 ? nCompositeLength - 2 : 0;

                m_vAllStrips[nLod].resize(nCompositeLength);
                memcpy(&m_vAllStrips[nLod][0], vCompositeStrips, nCompositeLength * sizeof(int));
            }
            else
            {
                st_vector_int( ).swap(m_vStripInfo[nLod]); // dealloc vector 
                m_vTriangleCounts[nLod] = 0;
            }

            vCompositeStrips += nCompositeLength;
        }

        st_delete_array<int>(pAllCompositeStrips, "CIndexedGeometry::CombineStrips, pAllCompositeStrips");
    }
}


///////////////////////////////////////////////////////////////////////  
//  CIndexedGeometry::ComputeExtents definition

void CIndexedGeometry::ComputeExtents(stRegion& cExtents) const
{
    // only use the highest LOD
    int nNumStrips = GetNumStrips(0);
    for (int i = 0; i < nNumStrips; ++i)
    {
        int nStripLength = m_pStripLengths[0][i];
        
        for (int j = 0; j < nStripLength; ++j)
        {
            const float* pCoord = GetVertexCoord(m_pStrips[0][i][j]);
            stVec3 cCoord(pCoord[0], pCoord[1], pCoord[2]);

            // operator^ expands cExtents to include cCoord
            cExtents = cExtents ^ cCoord;
        }
    }


//    int nVertexCount = GetVertexCount( );
//    for (int i = 0; i < nVertexCount; ++i)
//    {
//        const float* pCoord = GetVertexCoord(i);
//        stVec3 cCoord(pCoord[0], pCoord[1], pCoord[2]);
//
//        // operator^ expands cExtents to include cCoord
//        cExtents = cExtents ^ cCoord;
//    }
}


///////////////////////////////////////////////////////////////////////  
//  CIndexedGeometry::SetNumLodLevels definition

void CIndexedGeometry::SetNumLodLevels(int nLevels)
{
    // dealloc vector of vectors
    int i = 0;
    for (i = 0; i < int(m_vStripInfo.size( )); ++i)
        st_vector_int( ).swap(m_vStripInfo[i]);
    st_vector_int_vector( ).swap(m_vStripInfo);

    for (i = 0; i < int(m_vAllStrips.size( )); ++i)
        st_vector_int( ).swap(m_vAllStrips[i]);
    st_vector_int_vector( ).swap(m_vAllStrips);

    st_vector_int( ).swap(m_vTriangleCounts);

    m_nNumLodLevels = nLevels;

    m_vStripInfo.resize(nLevels);
    m_vAllStrips.resize(nLevels);
    m_vTriangleCounts.resize(nLevels);

    for (i = 0; i < nLevels; ++i)
    {
        if (CSpeedTreeRT::GetMemoryPreference( ) == CSpeedTreeRT::MP_FAVOR_LESS_FRAGMENTATION)
        {
            m_vStripInfo[i].reserve(500);
            m_vAllStrips[i].reserve(2000);
        }

        m_vTriangleCounts[i] = 0;
    }
}


///////////////////////////////////////////////////////////////////////  
//  CIndexedGeometry::Transform definition

void CIndexedGeometry::Transform(const stTransform& cTransform)
{
    int nVertexCount = GetVertexCount( );
    for (int i = 0; i < nVertexCount; ++i)
    {
        // transform coordinates
        float* pCoord = const_cast<float*>(GetVertexCoord(i));
        stVec3 cCoord(pCoord[0], pCoord[1], pCoord[2]);
        cCoord = cCoord * cTransform;
        memcpy(pCoord, cCoord.m_afData, 3 * sizeof(float));

        // orig coordinates are used as a basis for wind computations,
        // need to transform those too, if they are being used
        if (m_bVertexWeighting &&
            !m_vOrigCoords.empty( ))
        {
            float* pOrigCoord = const_cast<float*>(GetOrigVertexCoord(i));
            cCoord.Set(pOrigCoord[0], pOrigCoord[1], pOrigCoord[2]);
            cCoord = cCoord * cTransform;
            memcpy(pOrigCoord, cCoord.m_afData, 3 * sizeof(float));
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  CIndexedGeometry::AddStrip definition

void CIndexedGeometry::AddStrip(int nLodLevel, int* pStrip, int nStripLength)
{
    st_assert(nLodLevel < int(m_vStripInfo.size( )));
    st_assert(nLodLevel < int(m_vAllStrips.size( )));

    if (pStrip)
    {
        // the strip that's being added in this call will start at the end of m_vAllStrips,
        // so record that position here
        int nInsertionPoint = int(m_vAllStrips[nLodLevel].size( ));

        m_vStripInfo[nLodLevel].push_back(nInsertionPoint);
        m_vStripInfo[nLodLevel].push_back(nStripLength);

        // expand the strips container to hold all of the new indices
        m_vAllStrips[nLodLevel].resize(m_vAllStrips[nLodLevel].size( ) + nStripLength);
        memcpy(&(m_vAllStrips[nLodLevel][nInsertionPoint]), pStrip, nStripLength * sizeof(int));
    }
    else
    {
        // clear out this lod
        m_vStripInfo[nLodLevel].resize(2);
        m_vStripInfo[nLodLevel][0] = 0;
        m_vStripInfo[nLodLevel][1] = 0;

        m_vTriangleCounts[nLodLevel] = 0;
        st_vector_int( ).swap(m_vAllStrips[nLodLevel]);
    }
}


///////////////////////////////////////////////////////////////////////  
//  CIndexedGeometry::AddVertexColor definition

void CIndexedGeometry::AddVertexColor(const float* pColor)
{
    st_assert(pColor);

    // convert the 4-float value into a single unsigned int
    unsigned int uiColor = ColorFloatsToUInt(pColor);

    // store new color
    m_vColors.push_back(uiColor);
}


///////////////////////////////////////////////////////////////////////  
//  CIndexedGeometry::AddVertexCoord definition

void CIndexedGeometry::AddVertexCoord(const float* pCoord)
{
    st_assert(pCoord);

    // adjust for current up vector
    float afNewCoord[3];
    Assign3d(afNewCoord, pCoord);

    // store new coordinate
    m_vCoords.push_back(afNewCoord[0]);
    m_vCoords.push_back(afNewCoord[1]);
    m_vCoords.push_back(afNewCoord[2]);

    // set aside fade hint placeholder
    m_vLodFadeHints.push_back(1.0f);

    // if using CPU-based wind we need to have a copy of the coordinates that are
    // unaffected by wind
    if (m_bVertexWeighting &&
        m_eWindMethod == CSpeedTreeRT::WIND_CPU)
    {
        m_vOrigCoords.push_back(afNewCoord[0]);
        m_vOrigCoords.push_back(afNewCoord[1]);
        m_vOrigCoords.push_back(afNewCoord[2]);
    }
}


///////////////////////////////////////////////////////////////////////  
//  CIndexedGeometry::AddVertexNormal definition

void CIndexedGeometry::AddVertexNormal(const float* pNormal, bool bUpAxisAdjust)
{
    st_assert(pNormal);

    // adjust for current up vector
    if (bUpAxisAdjust)
    {
        float afNewNormal[3];
        Assign3d(afNewNormal, pNormal);

        // store new normal
        m_vNormals.push_back(afNewNormal[0]);
        m_vNormals.push_back(afNewNormal[1]);
        m_vNormals.push_back(afNewNormal[2]);
    }
    else
    {
        m_vNormals.push_back(pNormal[0]);
        m_vNormals.push_back(pNormal[1]);
        m_vNormals.push_back(pNormal[2]);
    }
}


///////////////////////////////////////////////////////////////////////  
//  CIndexedGeometry::AddVertexWind1 definition

void CIndexedGeometry::AddVertexWind1(float fWindWeight, unsigned char ucWindMatrixIndex)
{
    // computed values are opposite of those needed by the app
    fWindWeight = st_clamp<float>(1.0f - fWindWeight, 0.0f, 1.0f);
    
    // chWindMatrixIndex spans a wide range of matrices, mod it down to size
    unsigned int nStartingMatrix = 0, nMatrixSpan = 1;
    m_pWindEngine->GetLocalMatrices(nStartingMatrix, nMatrixSpan);
    ucWindMatrixIndex = static_cast<unsigned char>(nStartingMatrix + ucWindMatrixIndex % nMatrixSpan);

    // store the attributes
    m_vWindWeights1.push_back(fWindWeight);
    m_vWindMatrixIndices1.push_back(ucWindMatrixIndex);
}


///////////////////////////////////////////////////////////////////////  
//  CIndexedGeometry::AddVertexWind2 definition

void CIndexedGeometry::AddVertexWind2(float fWindWeight, unsigned char ucWindMatrixIndex)
{
    // computed values are opposite of those needed by the app
    fWindWeight = st_clamp<float>(1.0f - fWindWeight, 0.0f, 1.0f);
    
    // chWindMatrixIndex spans a wide range of matrices, mod it down to size
    unsigned int nStartingMatrix = 0, nMatrixSpan = 1;
    m_pWindEngine->GetLocalMatrices(nStartingMatrix, nMatrixSpan);
    ucWindMatrixIndex = static_cast<unsigned char>(nStartingMatrix + ucWindMatrixIndex % nMatrixSpan);

    // store the attributes
    m_vWindWeights2.push_back(fWindWeight);
    m_vWindMatrixIndices2.push_back(ucWindMatrixIndex);
}


///////////////////////////////////////////////////////////////////////  
//  CIndexedGeometry::AddVertexTangent definition

void CIndexedGeometry::AddVertexTangent(const float* pTangent)
{
    st_assert(pTangent);

    float afAdjustedTangent[3];
    Assign3d(afAdjustedTangent, pTangent);

    m_vTangents.push_back(afAdjustedTangent[0]);
    m_vTangents.push_back(afAdjustedTangent[1]);
    m_vTangents.push_back(afAdjustedTangent[2]);
}


///////////////////////////////////////////////////////////////////////  
//  CIndexedGeometry::AddVertexBinormal definition

void CIndexedGeometry::AddVertexBinormal(const float* pBinormal)
{
    st_assert(pBinormal);

    float afAdjustedBinormal[3];
    Assign3d(afAdjustedBinormal, pBinormal);

    m_vBinormals.push_back(afAdjustedBinormal[0]);
    m_vBinormals.push_back(afAdjustedBinormal[1]);
    m_vBinormals.push_back(afAdjustedBinormal[2]);
}


///////////////////////////////////////////////////////////////////////  
//  CIndexedGeometry::SetLodFadeHint definition

void CIndexedGeometry::SetLodFadeHint(int nIndex, float fFadeHint)
{
    st_assert(nIndex > -1 && nIndex < int(m_vLodFadeHints.size( )));

    m_vLodFadeHints[nIndex] = fFadeHint;
}


///////////////////////////////////////////////////////////////////////  
//  CIndexedGeometry::GetStrip definition

const int* CIndexedGeometry::GetStrips(int nLodLevel) const
{
    st_assert(nLodLevel < int(m_vAllStrips.size( )));

    return &(m_vAllStrips[nLodLevel][0]);
}


///////////////////////////////////////////////////////////////////////  
//  CIndexedGeometry::GetStripInfo definition

const int* CIndexedGeometry::GetStripInfo(int nLodLevel, int& nNumStrips) const
{
    st_assert(nLodLevel < int(m_vStripInfo.size( )));

    nNumStrips = int(m_vStripInfo[nLodLevel].size( ) / 2);
    if (nNumStrips > 0)
        return &(m_vStripInfo[nLodLevel][0]);
    else
        return NULL;
}


///////////////////////////////////////////////////////////////////////  
//  CIndexedGeometry::GetTriangleCount definition

int CIndexedGeometry::GetTriangleCount(int nLodLevel) const
{
    return (nLodLevel < m_nNumLodLevels && nLodLevel > -1) ? m_vTriangleCounts[nLodLevel] : 0;
}


///////////////////////////////////////////////////////////////////////  
//  CIndexedGeometry::ComputeWindEffect definition
//
//  This is the code for the CPU branch wind solution.  It takes two 
//  points per vertex: the original vertex, and that vertex multiplied
//  times the appropriate wind matrix, and then interpolates between them
//  based on the wind weight value (0.0 to 1.0).

bool CIndexedGeometry::ComputeWindEffect(int nLodLevel)
{
    st_assert(m_bVertexWeighting);
    st_assert(m_pWindEngine);
    st_assert(m_pStripLengths);
    st_assert(m_pStrips);

    bool bGeometryUpdated = false;
    if (!IsValid( ))
    {
        int nVertexCount = GetVertexCount( );
        if (!m_pVertexWindComputed)
            m_pVertexWindComputed = st_new_array<bool>(nVertexCount, "CIndexedGeometry::ComputeWindEffect, m_pVertexWindComputed");

        for (int i = 0; i < nVertexCount; ++i)
            m_pVertexWindComputed[i] = false; // makes sure we don't recompute any vertex

        stVec3 cFullWindEffect, cInterpWindEffect;
        for (int nStrip = 0; nStrip < GetNumStrips(nLodLevel); ++nStrip)
        {
            for (int nVertex = 0; nVertex < m_pStripLengths[nLodLevel][nStrip]; ++nVertex)
            {
                int nIndex = m_pStrips[nLodLevel][nStrip][nVertex];
                if (!m_pVertexWindComputed[nIndex])
                {
                    float* pOrigCoord = &(m_vOrigCoords[0]) + 3 * nIndex;

                    // original one level wind computation
                    float fWeight = m_vWindWeights1[nIndex];
                    cFullWindEffect.Set(pOrigCoord[0], pOrigCoord[1], pOrigCoord[2]);
                    unsigned short usWindGroup = m_vWindMatrixIndices1[nIndex];
                    cFullWindEffect = cFullWindEffect * m_pWindEngine->GetWindMatrix(usWindGroup);
                    cInterpWindEffect.Set(VecInterpolate(pOrigCoord[0], cFullWindEffect[0], fWeight),
                                          VecInterpolate(pOrigCoord[1], cFullWindEffect[1], fWeight),
                                          VecInterpolate(pOrigCoord[2], cFullWindEffect[2], fWeight));

                    // additional second level wind computation
                    fWeight = m_vWindWeights2[nIndex];
                    cFullWindEffect = cInterpWindEffect;
                    usWindGroup = m_vWindMatrixIndices2[nIndex];
                    cFullWindEffect = cFullWindEffect * m_pWindEngine->GetWindMatrix(usWindGroup);
                    cInterpWindEffect.Set(VecInterpolate(cInterpWindEffect[0], cFullWindEffect[0], fWeight),
                                          VecInterpolate(cInterpWindEffect[1], cFullWindEffect[1], fWeight),
                                          VecInterpolate(cInterpWindEffect[2], cFullWindEffect[2], fWeight));

                    memcpy(&(m_vCoords[0]) + nIndex * 3, cInterpWindEffect.m_afData, 3 * sizeof(float));

                    m_pVertexWindComputed[nIndex] = true;
                }
            }
        }

        Validate( );
        bGeometryUpdated = true;
    }

    return bGeometryUpdated;
}


///////////////////////////////////////////////////////////////////////  
//  CIndexedGeometry::DeleteLodStrip definition

void CIndexedGeometry::DeleteLodStrips(int nLodLevel)
{
    st_assert(nLodLevel < int(m_vAllStrips.size( )));

    st_vector_int( ).swap(m_vStripInfo[nLodLevel]);
    st_vector_int( ).swap(m_vAllStrips[nLodLevel]);
}


///////////////////////////////////////////////////////////////////////  
//  CIndexedGeometry::ColorFloatsToUInt definition

unsigned int CIndexedGeometry::ColorFloatsToUInt(const float* pColor) const
{
#ifdef BIG_ENDIAN
    return (int(pColor[0] * 255.0f) << 24) + (int(pColor[1] * 255.0f) << 16) + (int(pColor[2] * 255.0f) << 8) + 0x000000ff;
#else
    return (int(pColor[0] * 255.0f) << 0) + (int(pColor[1] * 255.0f) << 8) + (int(pColor[2] * 255.0f) << 16) + 0xff000000;
#endif
}


///////////////////////////////////////////////////////////////////////  
//  CIndexedGeometry::DeleteIndexData definition

void CIndexedGeometry::DeleteIndexData(void)
{
    // delete num strips pointers
    if (m_pNumStrips)
        st_delete_array<int>(m_pNumStrips, "CIndexedGeometry::DeleteIndexData, m_pNumStrips");

    // delete strips
    if (m_pStrips)
    {
        for (int nLod = 0; nLod < m_nNumLodLevels; ++nLod)
            st_delete_array<int*>(m_pStrips[nLod], "CIndexedGeometry::DeleteIndexData, m_pStrips[nLod]");
        st_delete_array<int**>(m_pStrips, "CIndexedGeometry::DeleteIndexData, m_pStrips");
    }

    // delete strip lengths
    if (m_pStripLengths)
    {
        for (int nLod = 0; nLod < m_nNumLodLevels; ++nLod)
            st_delete_array<int>(m_pStripLengths[nLod], "CIndexedGeometry::DeleteIndexData, m_pStripLengths[nLod]");
        st_delete_array<int*>(m_pStripLengths, "CIndexedGeometry::DeleteIndexData, m_pStripLengths");
    }

    int i = 0;
    for (i = 0; i < int(m_vStripInfo.size( )); ++i)
        st_vector_int( ).swap(m_vStripInfo[i]);
    st_vector_int_vector( ).swap(m_vStripInfo);

    for (i = 0; i < int(m_vAllStrips.size( )); ++i)
        st_vector_int( ).swap(m_vAllStrips[i]);
    st_vector_int_vector( ).swap(m_vAllStrips);
}


///////////////////////////////////////////////////////////////////////  
//  CIndexedGeometry::GetVertexCoord definition

const float* CIndexedGeometry::GetVertexCoord(unsigned int nVertexIndex) const
{
    st_assert(nVertexIndex < m_vCoords.size( ) / 3);

    return &(m_vCoords[0]) + nVertexIndex * 3;
}


///////////////////////////////////////////////////////////////////////  
//  CIndexedGeometry::GetVertexTexCoord definition

const float* CIndexedGeometry::GetVertexTexCoord(CSpeedTreeRT::ETextureLayers eLayer, unsigned int nVertexIndex) const
{
    st_assert(nVertexIndex < m_avTexCoords[eLayer].size( ) / 2);

    return &(m_avTexCoords[eLayer][nVertexIndex * 2]);
}


///////////////////////////////////////////////////////////////////////  
//  CIndexedGeometry::GetOrigVertexCoord definition

const float* CIndexedGeometry::GetOrigVertexCoord(unsigned int nVertexIndex) const
{
    st_assert(nVertexIndex < m_vOrigCoords.size( ) / 3);

    return &(m_vOrigCoords[0]) + nVertexIndex * 3;
}


///////////////////////////////////////////////////////////////////////  
//  CIndexedGeometry::ChangeTexCoord definition

void CIndexedGeometry::ChangeTexCoord(unsigned char chChangedMapIndex, float* pNewTexCoords)
{
    st_assert(m_bRetainTexCoords);

    if (chChangedMapIndex == m_vTexIndices0[m_nNumVertices])
    {
        float afTexCoords[2] = 
        {
            VecInterpolate(pNewTexCoords[2], pNewTexCoords[0], m_vOrigTexCoords0[m_nNumVertices * 2]),
            VecInterpolate(pNewTexCoords[5], pNewTexCoords[1], m_vOrigTexCoords0[m_nNumVertices * 2 + 1])
        };

        if (CSpeedTreeRT::GetTextureFlip( ))
            afTexCoords[1] = -afTexCoords[1];

        AddVertexTexCoord(CSpeedTreeRT::TL_DIFFUSE, afTexCoords);
    }
}


///////////////////////////////////////////////////////////////////////  
//  CIndexedGeometry::Finish definition

void CIndexedGeometry::Finish(void)
{
    st_assert(int(m_vStripInfo.size( )) == m_nNumLodLevels);

    // setup num strips
    m_pNumStrips = st_new_array<int>(m_nNumLodLevels, "CIndexedGeometry::Finish, m_pNumStrips");
    int nLod = 0;
    for (nLod = 0; nLod < m_nNumLodLevels; ++nLod)
    {
        m_pNumStrips[nLod] = GetNumStrips(nLod);
    }

    // setup strip lengths (and compute triangle counts)
    m_pStripLengths = st_new_array<int*>(m_nNumLodLevels, "CIndexedGeometry::Finish, m_pStripLengths");
    for (nLod = 0; nLod < m_nNumLodLevels; ++nLod)
    {
        int nNumStrips = int(m_vStripInfo[nLod].size( ) / 2);
        m_pStripLengths[nLod] = st_new_array<int>(nNumStrips, "CIndexedGeometry::Finish, m_pStripLengths[nLod]");

        m_vTriangleCounts[nLod] = 0;
        for (int nStrip = 0; nStrip < nNumStrips; ++nStrip)
        {
            m_pStripLengths[nLod][nStrip] = m_vStripInfo[nLod][nStrip * 2 + 1];
            if (m_vStripInfo[nLod][nStrip * 2 + 1] > 2)
                m_vTriangleCounts[nLod] += m_vStripInfo[nLod][nStrip * 2 + 1] - 2;
        }
    }

    // setup strips
    m_pStrips = st_new_array<int**>(m_nNumLodLevels, "CIndexedGeometry::Finish, m_pStrips");
    for (nLod = 0; nLod < m_nNumLodLevels; ++nLod)
    {
        int nNumStrips = int(m_vStripInfo[nLod].size( ) / 2);
        m_pStrips[nLod] = st_new_array<int*>(nNumStrips, "CIndexedGeometry::Finish, m_pStrips[nLod]");

        for (int nStrip = 0; nStrip < nNumStrips; ++nStrip)
        {
            int nStartingPoint = m_vStripInfo[nLod][nStrip * 2];
            if (nStartingPoint < int(m_vAllStrips[nLod].size( )) &&
                nStartingPoint + int(m_vStripInfo[nLod][nStrip * 2 + 1]) - 1 < int(m_vAllStrips[nLod].size( )))
                m_pStrips[nLod][nStrip] = &(m_vAllStrips[nLod][nStartingPoint]);
            else
                m_pStrips[nLod][nStrip] = NULL;
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  CIndexedGeometry::CompositeLength definition

int CIndexedGeometry::CompositeLength(int nLod, bool bToggleFaceOrdering) const
{
    st_assert(nLod < int(m_vAllStrips.size( )));
    st_assert(nLod < int(m_vStripInfo.size( )));

    int nCompositeLength = 0;
    int nNumStrips = int(m_vStripInfo[nLod].size( ) / 2);
    int nStrip = 0;
    for (nStrip = 0; nStrip < nNumStrips; ++nStrip)
    {
        int nIndex = nStrip * 2 + 1;
        st_assert(nIndex < int(m_vStripInfo[nLod].size( )));
        nCompositeLength += m_vStripInfo[nLod][nIndex];
    }

    // need to add more to nCompositeLength to account for the vertices we'll add for stitching
    if (bToggleFaceOrdering)
    {
        // need to stitch with different number of vertices for correct lighting
        for (nStrip = 0; nStrip < nNumStrips - 1; ++nStrip)
            nCompositeLength += (m_vStripInfo[nLod][nStrip * 2 + 1] % 2) ? 3 : 2;
    }
    else
    {
        // add four more indexes between each individual strip
        nCompositeLength += (nNumStrips > 0) ? 2 * (nNumStrips - 1) : 2 * nNumStrips;
    }

    return nCompositeLength;
}


///////////////////////////////////////////////////////////////////////  
//  CIndexedGeometry::AddVertexTexCoord definition

void CIndexedGeometry::AddVertexTexCoord(CSpeedTreeRT::ETextureLayers eLayer, const float* pTexCoord, int nIndex)
{
    st_assert(pTexCoord);
    st_assert(eLayer < CSpeedTreeRT::TL_NUM_TEX_LAYERS);

    if (eLayer != CSpeedTreeRT::TL_DIFFUSE)
    {
        // store the new texture coords
        m_avTexCoords[eLayer].push_back(pTexCoord[0]);
        m_avTexCoords[eLayer].push_back(CSpeedTreeRT::GetTextureFlip( ) ? -pTexCoord[1] : pTexCoord[1]);
    }
    else
    {
        // texture coordinates are randomly accessed by the CFrondEngine class, so
        // we put some code here to compensate
        if (m_nNumVertices == int(m_avTexCoords[eLayer].size( ) / 2))
        {
            m_avTexCoords[eLayer].resize(m_avTexCoords[eLayer].size( ) + 2);
            if (nIndex > -1 &&
                m_bRetainTexCoords)
            {
                m_vTexIndices0.resize(m_vTexIndices0.size( ) + 1);
                m_vOrigTexCoords0.resize(m_vOrigTexCoords0.size( ) + 2);
            }
        }

        // copies of the texture coordinates and texture indices are used to facilitate functions
        // that exist solely for use in SpeedTreeCAD, SpeedTreeMAX, and SpeedTreeMAYA.  There's
        // pretty much no reason the end user will need to access these.
        if (nIndex > -1 &&
            m_bRetainTexCoords)
        {
            m_vTexIndices0[m_nNumVertices] = static_cast<unsigned char>(nIndex);
            m_vOrigTexCoords0[m_nNumVertices * 2] = pTexCoord[0];
            m_vOrigTexCoords0[m_nNumVertices * 2 + 1] = pTexCoord[1];
        }

        // store the new texture coords
        m_avTexCoords[eLayer][m_nNumVertices * 2] = pTexCoord[0];
        m_avTexCoords[eLayer][m_nNumVertices * 2 + 1] = CSpeedTreeRT::GetTextureFlip( ) ? -pTexCoord[1] : pTexCoord[1];
    }
}

