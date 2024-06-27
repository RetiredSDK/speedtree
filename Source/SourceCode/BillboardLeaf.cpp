///////////////////////////////////////////////////////////////////////  
//  BillboardLeaf.cpp
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
#include "LibGlobals_Source\IdvGlobals.h"
#include "BillboardLeaf.h"
#include "FileAccess.h"
#include "Endian.h"


///////////////////////////////////////////////////////////////////////  
//  CBillboardLeaf::CBillboardLeaf definition

CBillboardLeaf::CBillboardLeaf(const stVec3& cPos, short nColorScale, int nAngleIndex, float fWindWeight1, float fWindWeight2, int nWindGroup1, int nWindGroup2) :
    m_ucAngleIndex(static_cast<unsigned char>(nAngleIndex)),
    m_ucColorScale(static_cast<unsigned char>(nColorScale)),
    m_ucTextureIndex(0u),
    m_fWindWeight1(st_clamp<float>(fWindWeight1, 0.0f, 1.0f)),
    m_fWindWeight2(st_clamp<float>(fWindWeight2, 0.0f, 1.0f)),
    m_nWindGroup1(nWindGroup1),
    m_nWindGroup2(nWindGroup2)
{
    // default the colors to white & opaque
    m_auiColors[0] = m_auiColors[1] = m_auiColors[2] = m_auiColors[3] = 0xffffffff;

    m_cPosition = cPos;
}


///////////////////////////////////////////////////////////////////////  
//  CBillboardLeaf::CBillboardLeaf definition

CBillboardLeaf::CBillboardLeaf( ) :
    m_ucAngleIndex(0u),
    m_ucColorScale(0u),
    m_ucTextureIndex(0u),
    m_fWindWeight1(0.0f),
    m_fWindWeight2(0.0f),
    m_nWindGroup1(0),
    m_nWindGroup2(0)
{
    // default the colors to white & opaque
    m_auiColors[0] = m_auiColors[1] = m_auiColors[2] = m_auiColors[3] = 0xffffffff;
}


///////////////////////////////////////////////////////////////////////  
//  CBillboardLeaf::AdjustStaticLighting definition

void CBillboardLeaf::AdjustStaticLighting(const stVec3& cTreeCenter, const stVec3& cLightPos, float fLightScalar)
{
    stVec3 cLeafAngle = m_cPosition - cTreeCenter;
    cLeafAngle.Normalize( );

    stVec3 cLightAngle = cLightPos - cTreeCenter;
    cLightAngle.Normalize( );

    float fAngleBetween = cosf(cLeafAngle.AngleBetween(cLightAngle));
    float fLightAdjust = VecInterpolate(fLightScalar, 1.0f, (fAngleBetween + 1.0f) * 0.5f);

    SetColor(GetColor(0) * fLightAdjust, 0);
    SetColor(GetColor(1) * fLightAdjust, 1);
    SetColor(GetColor(2) * fLightAdjust, 2);
    SetColor(GetColor(3) * fLightAdjust, 3);
}



///////////////////////////////////////////////////////////////////////  
//  CBillboardLeaf::Clone definition

CBillboardLeaf* CBillboardLeaf::Clone(void) const
{
    CBillboardLeaf* pClone = new ("CBillboardLeaf::Clone, pClone") CBillboardLeaf;
    *pClone = *this; // a bit-wise copy is safe

    return pClone;
}


///////////////////////////////////////////////////////////////////////  
//  CBillboardLeaf::GetColor definition
//
//  Converts from the internal long representation to a 3-float RGB value.

stVec3 CBillboardLeaf::GetColor(int nIndex) const
{
    return stVec3(((m_auiColors[nIndex] & 0x000000ff) >> 0) / 255.0f,
                 ((m_auiColors[nIndex] & 0x0000ff00) >> 8) / 255.0f,
                 ((m_auiColors[nIndex] & 0x00ff0000) >> 16) / 255.0f);
}


///////////////////////////////////////////////////////////////////////  
//  CBillboardLeaf::SetColor definition
//
//  To simulate shadow effects, leaves that are closer to the center of the
//  tree are darkened.  This darken scalar value is stored in m_chColorScale and
//  ranges from 0 to 255.

void CBillboardLeaf::SetColor(const stVec3& cColor, int nIndex, bool bApplyDimming)
{
    stVec3 cScaledColor = cColor;
    if (bApplyDimming)
        cScaledColor *= m_ucColorScale / 255.0f;

    for (int i = 0; i < 3; ++i)
    {
        if (cScaledColor[i] > 1.0f)
            cScaledColor[i] = 1.0f;
        if (cScaledColor[i] < 0.0f)
            cScaledColor[i] = 0.0f;
    }
    
    m_auiColors[nIndex] = (int(cScaledColor[0] * 255.0f) << 0) +
                          (int(cScaledColor[1] * 255.0f) << 8) +
                          (int(cScaledColor[2] * 255.0f) << 16) +
                          0xff000000;
}


///////////////////////////////////////////////////////////////////////  
//  CBillboardLeaf::Save definition

void CBillboardLeaf::Save(CTreeFileAccess& cFile) const
{
    cFile.SaveToken(File_BeginBillboardLeaf);

    cFile.SaveToken(File_BillboardLeaf_Color);
    cFile.SaveLong(m_auiColors[0]);

    cFile.SaveToken(File_BillboardLeaf_AngleIndex);
    cFile.SaveByte(m_ucAngleIndex);

    cFile.SaveToken(File_BillboardLeaf_ColorScale);
    cFile.SaveByte(m_ucColorScale);

    cFile.SaveToken(File_BillboardLeaf_Normal);
    cFile.SaveVector3(m_acNormals[0]);

    cFile.SaveToken(File_BillboardLeaf_TextureIndex);
    cFile.SaveByte(m_ucTextureIndex);

    cFile.SaveToken(File_BillboardLeaf_WindWeight);
    if (m_fWindWeight1 > 1.0f)
        cFile.SaveByte(255);
    else
        cFile.SaveByte(static_cast<unsigned char>(255.0f * m_fWindWeight1));

    cFile.SaveToken(File_BillboardLeaf_WindGroup);
    cFile.SaveInt(m_nWindGroup1);

    cFile.SaveToken(File_BillboardLeaf_Pos);
    cFile.SaveVector3(GetPosition( ));

    cFile.SaveToken(File_BillboardLeaf_TimeOffset);
    cFile.SaveFloat(0.0f); // this member var was removed (m_fTimeOffset)

    cFile.SaveToken(File_EndBillboardLeaf);
}


///////////////////////////////////////////////////////////////////////  
//  CBillboardLeaf::Parse definition

void CBillboardLeaf::Parse(CTreeFileAccess& cFile, CBillboardLeaf& cLeaf)
{
    int nToken = cFile.ParseToken( );
    do
    {
        switch (nToken)
        {
            case File_BillboardLeaf_Color:
                cLeaf.m_auiColors[0] = cFile.ParseUInt( );
                break;
            case File_BillboardLeaf_AngleIndex:
                cLeaf.m_ucAngleIndex = cFile.ParseByte( );
                break;
            case File_BillboardLeaf_ColorScale:
                cLeaf.m_ucColorScale = cFile.ParseByte( );
                break;
            case File_BillboardLeaf_Normal:
                cLeaf.m_acNormals[0] = cFile.ParseVector3( );
                break;
            case File_BillboardLeaf_TextureIndex:
                cLeaf.m_ucTextureIndex = cFile.ParseByte( );
                break;
            case File_BillboardLeaf_WindWeight:
                cLeaf.m_fWindWeight1 = cFile.ParseByte( ) / 255.0f;
                break;
            case File_BillboardLeaf_WindGroup:
                cLeaf.m_nWindGroup1 = cFile.ParseInt( );
                break;
            case File_BillboardLeaf_Pos:
                cLeaf.SetPosition(cFile.ParseVector3( ));
                break;
            case File_BillboardLeaf_TimeOffset:
                (void) cFile.ParseFloat( ); // m_fTimeOffset was removed
                break;
            default:
                throw(IdvFileError("malformed billboard leaf"));
        }
        nToken = cFile.ParseToken( );
    } while (nToken != File_EndBillboardLeaf);
}


///////////////////////////////////////////////////////////////////////  
//  CBillboardLeaf::GetColorUInt definition

unsigned int CBillboardLeaf::GetColorUInt(int nIndex) const
{
#ifdef BIG_ENDIAN
    union
    {
        unsigned int    m_uiUnsignedInt;
        unsigned char   m_ucBytes[4];
    } uUnsignedInt;

    const unsigned char* pColorAsBytes = reinterpret_cast<const unsigned char*>(m_auiColors + nIndex);
    uUnsignedInt.m_ucBytes[3] = pColorAsBytes[0];
    uUnsignedInt.m_ucBytes[2] = pColorAsBytes[1];
    uUnsignedInt.m_ucBytes[1] = pColorAsBytes[2];
    uUnsignedInt.m_ucBytes[0] = pColorAsBytes[3];

    return uUnsignedInt.m_uiUnsignedInt;
#else
    return m_auiColors[nIndex];
#endif
}

