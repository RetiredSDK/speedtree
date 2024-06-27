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

#include "IdvCommandLineParser.h"
#include "IdvFilename.h"
#ifdef PS3
#include <paths.h>
#endif


///////////////////////////////////////////////////////////////////////  
//  CIdvCommandLineParser::CIdvCommandLineParser
//
// Example: 
//    game:\TreeModels -visibility 10000 -numtrees 1000000 -area 60000 60000 -fog 0 9500 -lod 300 1300 lightdir 0.577 0.577 0.577 (480p)

CIdvCommandLineParser::CIdvCommandLineParser( ) :
    m_fAreaWidth(40000.0f),
    m_fAreaHeight(40000.0f),
    m_fFogStart(0.0f),
    m_fFogEnd(5000.0f),
    m_fLightScale(1.0f),
    m_fNearLod(300.0f),
    m_fFarLod(1200.0f),
    m_nNumTrees(250000),
    m_nWindowWidth(1024),
    m_nWindowHeight(768),
    m_fVisibility(5000.0f),
    m_bFullscreen(true),
    m_bForcedResolution(false),
    m_bOverlappingForests(false)
{
    // default to a single forest file
    m_vForestFiles.resize(1);
#ifdef PS3
    m_vForestFiles[0] = st_string(SYS_APP_HOME"/Data/TreeModels.Tropical");
#elif _XBOX
    m_vForestFiles[0] = "game:\\TreeModels.Tropical";
#else
    m_vForestFiles[0] = "../../Data/TreeModels.Tropical";
#endif

    m_afLightDir[0] = 0.577f;
    m_afLightDir[1] = 0.577f;
    m_afLightDir[2] = 0.577f;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvCommandLineParser::~CIdvCommandLineParser

CIdvCommandLineParser::~CIdvCommandLineParser( )
{
}


///////////////////////////////////////////////////////////////////////  
//  PrintUsage

void PrintUsage(const char* pExeName)
{
    printf("\tSpeedTreeRT v4.1 Reference Application (c) 2007\n");
    printf("\nUsage:\n\n%s <SPT dir #1;SPT dir #2;...> {<options>}\n\n", IdvNoPath(pExeName).c_str( ));
    printf("where options can be (all units are same as SPT files):\n\n");
    printf("\t-area <width> <length>   size of randomly populated area\n");
    printf("\t-fog <start> <end>       linear fog start and end values\n");
    printf("\t-lightdir <x> <y> <z>    direction of light (values will be normalized)\n");
    printf("\t-lightscale <scale>      Scale the lighting range to help normalize the scene\n");
    printf("\t-lod <near> <far>        distance values where LOD begins and end\n");
    printf("\t-numtrees <count>        total number of trees to place in area\n");
    printf("\t-res <width> <height>    window resolution\n");
    printf("\t-visibility <distance>   distance from camera to horizon\n");
    printf("\t-windowed                makes the application run in a window\n");
    printf("\t-overlap                 renders all forests simultaneously\n");
    printf("\n");

    // fog will be disabled if not specified
}


///////////////////////////////////////////////////////////////////////  
//  CIdvCommandLineParser::Parse

bool CIdvCommandLineParser::Parse(int argc, char* argv[])
{
    bool bSuccess = false;

#if !defined(_XBOX) && !defined(PS3)
    PrintKeys( );
#endif

    if (argc >= 2)
    {
        // scan argv[1] for semicolons to separate forest names
        ParseForestDirs(argv[1]);

        for (int i = 2; i < argc; ++i)
        {
            st_string strCommand = argv[i];
            if (strCommand == "-area")
            {
                if (i + 2 < argc)
                {
                    m_fAreaWidth = float(atof(argv[++i]));
                    m_fAreaHeight = float(atof(argv[++i]));
                }
                else
                    PrintUsage(argv[0]);
            }
            else if (strCommand == "-fog")
            {
                if (i + 2 < argc)
                {
                    m_fFogStart = float(atof(argv[++i]));
                    m_fFogEnd = float(atof(argv[++i]));
                }
                else
                    PrintUsage(argv[0]);
            }
            else if (strCommand == "-lightdir")
            {
                if (i + 3 < argc)
                {
                    m_afLightDir[0] = float(atof(argv[++i]));
                    m_afLightDir[1] = float(atof(argv[++i]));
                    m_afLightDir[2] = float(atof(argv[++i]));
                }
                else
                    PrintUsage(argv[0]);
            }
            else if (strCommand == "-lightscale")
            {
                if (i + 1 < argc)
                {
                    m_fLightScale = float(atof(argv[++i]));
                }
                else
                    PrintUsage(argv[0]);
            }
            else if (strCommand == "-lod")
            {
                if (i + 2 < argc)
                {
                    m_fNearLod = float(atof(argv[++i]));
                    m_fFarLod = float(atof(argv[++i]));
                }
                else
                    PrintUsage(argv[0]);
            }
            else if (strCommand == "-numtrees")
            {
                if (i + 1 < argc)
                {
                    m_nNumTrees = atoi(argv[++i]);
                }
                else
                    PrintUsage(argv[0]);
            }
            else if (strCommand == "-res")
            {
                if (i + 2 < argc)
                {
                    m_nWindowWidth = atoi(argv[++i]);
                    m_nWindowHeight = atoi(argv[++i]);
                    m_bForcedResolution = true;
                }
                else
                    PrintUsage(argv[0]);
            }
            else if (strCommand == "-visibility")
            {
                if (i + 1 < argc)
                {
                    m_fVisibility = float(atof(argv[++i]));
                }
                else
                    PrintUsage(argv[0]);
            }
            else if (strCommand == "-windowed" ||
                     strCommand == "-window")
            {
                m_bFullscreen = false;
            }
            else if (strCommand == "-overlap" ||
                     strCommand == "-simul")
            {
                m_bOverlappingForests = true;
            }
            else
            {
                fprintf(stderr, "Unknown command [%s]\n", strCommand.c_str( ));
                PrintUsage(argv[0]);
            }
        }
        bSuccess = true;
    }

    printf("Area: %g square feet (%g sq. mi. or %g sq. km)\n",
        m_fAreaWidth * m_fAreaHeight, 
        (m_fAreaWidth * m_fAreaHeight) / (5280.0f * 5280.0f), 
        (1.613f * 1.613f) * (m_fAreaWidth * m_fAreaHeight) / (5280.0f * 5280.0f));
    printf("Visibility: %g ft (%g miles or %g km)\n", 
        m_fVisibility, m_fVisibility / 5280.0f, 1.613f * m_fVisibility / 5280.0f);

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CIdvCommandLineParser::PrintKeys

void CIdvCommandLineParser::PrintKeys(void)
{
    printf("\n");
    printf("SpeedTreeRT v4.1 Reference Application (c) 2007\n");
    printf("\n");
    printf("Key bindings:\n");
    printf("\tB\t\tToggle branch geometry\n");
    printf("\tF\t\tToggle frond geometry\n");
    printf("\tL\t\tToggle leaf geometry\n");
    printf("\tZ\t\tToggle billboard geometry\n");
    printf("\tC\t\tToggle manual/auto camera\n");
    printf("\tX (hold)\tRotate light around up-axis\n");
    printf("\tO\t\tToggle forest overlay\n");
    printf("\t<, > (hold)\tOverlay zoom\n");
    printf("\tS, D (hold)\tRotate trees\n");
    printf("\tA\t\tToggle alpha testing\n");
    printf("\tW\t\tToggle wireframe\n");
    printf("\tN\t\tToggle normal mapping\n");
    printf("\tG\t\tToggle wind strength (moderate to strong)\n");
    printf("\t1...9\t\tSwitch forests (if specified on command-line)\n");
    printf("\tESC\t\tQuit\n");
    printf("\n");
}


///////////////////////////////////////////////////////////////////////  
//  CIdvCommandLineParser::ParseForestDirs
//
//  Input parameter is a set of folders, separated by semicolons

void CIdvCommandLineParser::ParseForestDirs(const char* pForestDirs)
{
    st_string strForestDirs(pForestDirs);

    if (!strForestDirs.empty( ))
    {
        m_vForestFiles.clear( );
        bool bLastOne = false;
        st_string::size_type uiScanStart = 0;
        while (!bLastOne)
        {
            st_string::size_type uiSemiColonPos = strForestDirs.find(";", uiScanStart);

            st_string strOneForest = strForestDirs.substr(uiScanStart, uiSemiColonPos - uiScanStart);
            if (!strOneForest.empty( ))
            {
                m_vForestFiles.push_back(strOneForest);
            }

            if (uiSemiColonPos >= strForestDirs.length( ))
                bLastOne = true;
            
            uiScanStart = uiSemiColonPos + 1;
        }
    }
}


