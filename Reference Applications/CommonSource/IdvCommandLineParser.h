///////////////////////////////////////////////////////////////////////  
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
//      Web: http://www.idvinc.com

#pragma once
#include "SpeedTreeMemory.h"


///////////////////////////////////////////////////////////////////////  
//  class CIdvCommandLineParser declaration

class CIdvCommandLineParser
{
public:
        CIdvCommandLineParser( );
        ~CIdvCommandLineParser( );

        bool                Parse(int argc, char* argv[]);

        // just access the variables directly - no need for inspector functions
        float               m_fAreaWidth;
        float               m_fAreaHeight;
        float               m_fFogStart;
        float               m_fFogEnd;
        float               m_afLightDir[3];
        float               m_fLightScale;
        float               m_fNearLod;
        float               m_fFarLod;
        int                 m_nNumTrees;
        int                 m_nWindowWidth;
        int                 m_nWindowHeight;
        float               m_fVisibility;
        bool                m_bFullscreen;
        bool                m_bForcedResolution;
        bool                m_bOverlappingForests;
        st_vector_string    m_vForestFiles;

private:
        void                PrintKeys(void);
        void                ParseForestDirs(const char* pDirs);
};

