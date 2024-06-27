///////////////////////////////////////////////////////////////////////  
//  TreeInfo.cpp
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
//  SIdvTreeInfo::SIdvTreeInfo definition
//

SIdvTreeInfo::SIdvTreeInfo( ) :
    m_fFar(1100.0f),
    m_fNear(100.0f),
    m_nSeed(1200),
    m_fSize(50.0f),
    m_fSizeVariance(0.0f),
    m_nFlareSeed(444)
{
}
