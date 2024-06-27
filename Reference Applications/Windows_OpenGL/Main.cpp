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


///////////////////////////////////////////////////////////////////////  
//  Include Files

#include "IdvGlobal.h"
#include "IdvCommandLineParser.h"
#include "IdvFilename.h"
#include "stl_vector.h"
#include "glew.h"
#include "glut.h"
#include "SpeedTreeForest.h"
#include "IdvMouseNavigation.h"
#include "IdvAutoCamera.h"
#include "IdvVectorMath.h"
#include "UpVector.h"
#include "ReferenceAllocator.h"
#include "IdvTimer.h"
#include "IdvRandom.h"
using namespace std;


///////////////////////////////////////////////////////////////////////  
//  Prototypes

        int                     main(int argc, char** argv);
static  bool                    InitWindow(void);
static  void                    Display(void);
static  void                    Idle(void);
static  void                    Visible(int nVisible);
static  void                    Key(unsigned char chKey, int nX, int nY);
static  void                    MouseClick(int nButton, int nState, int nX, int nY);
static  void                    MouseMotion(int nX, int nY);
static  void                    Reshape(int nWidth, int nHeight);
static  void                    SetupWindMatrices(void);
static  void                    CheckTime(void);
static  void                    ExitApp(void);


///////////////////////////////////////////////////////////////////////  
//  Variables

// window
static  int                     g_nWindow = 0;
static  float                   g_fAspectRatio = 4.0f / 3.0f;
static  float                   g_fFieldOfView = 40.0f;
static  float                   g_fNearClipPlane = 1.0f;
static  float                   g_fFarClipPlane = 1000.0f;
                        
// wind         
static  float                   g_fCurrentTime = 0.0f;
static  float                   g_fElapsedTime = 0.0f; // time between frame swaps in seconds
static  float                   g_fLastFrameTime = 0.0f;
static  CIdvTimer               g_cTimer;
static  float                   g_fWindStrength = 0.3f;

// new from version 1.5.1
static  CSpeedTreeForest*       g_pForest = NULL;
static  st_vector_forestp*      g_pForestBank = NULL;
static  idv::Region             g_cWorldExtents; // extents of all forests together
static  int                     g_nNumForests = 0;
static  unsigned int            g_uiRenderFlags = CSpeedTreeForest::ALL;
static  bool                    g_bDrawOverlay = false;
static  CMouseNavigation        g_cMouseNavigation;
static  CAutoCamera             g_cAutoCamera;
static  CNavigationBase*        g_pCurrentNavigation = &g_cMouseNavigation;

// misc
        const float             c_afBgColor[4] = { 209.0f / 255.0f, 235.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f };
        CReferenceAllocator     g_cAllocator;
static  CIdvCommandLineParser*  g_pCmdLine;


///////////////////////////////////////////////////////////////////////  
//  main

int main(int argc, char* argv[])
{
#ifdef USE_CUSTOM_ALLOCATOR
    CSpeedTreeRT::SetAllocator(&g_cAllocator);
#endif

    g_pCmdLine = new ("main, g_pCmdLine") CIdvCommandLineParser;
    if (!g_pCmdLine->Parse(argc, argv))
        printf("Command-line option malformed or not set - using default values to fill in the gaps\n");
    g_fFarClipPlane = g_pCmdLine->m_fVisibility;

    glutInit(&argc, argv);
    glutInitDisplayString("rgb double depth>=24");

    if (g_pCmdLine->m_bFullscreen && g_pCmdLine->m_bForcedResolution)
    {
        // full screen with specific resolution
        glutGameModeString(IdvFormatString("%dx%d:32@60", g_pCmdLine->m_nWindowWidth, g_pCmdLine->m_nWindowHeight).c_str( ));
        (void) glutEnterGameMode( );
    }
    else
    {
        // windowed
        glutInitWindowSize(g_pCmdLine->m_nWindowWidth, g_pCmdLine->m_nWindowHeight);
        (void) glutCreateWindow("SpeedTreeRT OpenGL Example v4.1 (c) 2007 IDV, Inc.");

        // fullscreen with no resolution
        if (g_pCmdLine->m_bFullscreen)
            glutFullScreen( );
    }   

    if (InitWindow( ))
        glutMainLoop( );

    return 0;
}


///////////////////////////////////////////////////////////////////////  
//  InitWindow

bool InitWindow(void)
{
    bool bSuccess = false;

    // initialize OpenGL extensions (for DDS files, compression, etc)
    static bool bGlewInitialized = false;
    if (!bGlewInitialized)
    {
        GLenum err = glewInit( );
        if (err != GLEW_OK)
        {
            printf("GLEW initialization failed: %s\n", glewGetErrorString(err));
            exit(-1);
        }
    }

    // glut setup
    glutDisplayFunc(Display);
    glutVisibilityFunc(Visible);
    glutKeyboardFunc(Key);
    glutMouseFunc(MouseClick);
    glutMotionFunc(MouseMotion);
    glutReshapeFunc(Reshape);
    g_nWindow = glutGetWindow();

    // OpenGL setup
    glClearColor(c_afBgColor[0], c_afBgColor[1], c_afBgColor[2], 1.0f);
    glEnable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glAlphaFunc(GL_GREATER, 0.33f);
    glEnable(GL_ALPHA_TEST);

    // set textures
    CSpeedTreeRT::SetTextureFlip(false);

    // init forest(s)
    g_nNumForests = int(g_pCmdLine->m_vForestFiles.size( ));
    if (g_nNumForests < 1)
    {
        printf("FATAL - No forest directories specified\n");
        exit(-1);
    }

    g_pForestBank = new st_vector_forestp;
    g_pForestBank->resize(g_nNumForests);
    for (int nForest = 0; nForest < g_nNumForests; ++nForest)
    {
        const st_string& strForestDir = g_pCmdLine->m_vForestFiles[nForest];
        printf("\n** Initializing forest in folder [%s] (%d of %d) \n", strForestDir.c_str( ), nForest + 1, g_nNumForests);

        g_pForest = g_pForestBank->at(nForest) = new ("InitWindow, g_vForestBank[nForest]") CSpeedTreeForest;

        // fog
        g_pForest->SetFogParams(g_pCmdLine->m_fFogStart, g_pCmdLine->m_fFogEnd, c_afBgColor);

        // LOD
        g_pForest->SetLodLimits(g_pCmdLine->m_fNearLod, g_pCmdLine->m_fFarLod);

        // lighting
        idv::Vec3 cLightDir(g_pCmdLine->m_afLightDir);
        cLightDir.Normalize( );
        float afLightColor[3] = { 1.0f, 1.0f, 1.0f };
        g_pForest->SetLighting(cLightDir, afLightColor, g_pCmdLine->m_fLightScale);

#ifdef REPORT_TIMES
        CIdvTimer cTimer;
        cTimer.Start( );
#endif

        // load an STF file
        if (IdvStrcmpi(IdvExtension(strForestDir), "stf"))
        {
            bSuccess = g_pForest->PopulateFromStfFile(strForestDir.c_str( ));
        }
        // populate forest randomly with SPT files found in a folder
        else
        {
            st_vector_string vSptFiles;
            IdvScanFolder(strForestDir, "spt", false, vSptFiles);
            if (!vSptFiles.empty( ))
            {
                float afExtents[6];
                Assign3d(afExtents + 0, -0.5f * g_pCmdLine->m_fAreaWidth, -0.5f * g_pCmdLine->m_fAreaHeight, 0.0f);
                Assign3d(afExtents + 3, 0.5f * g_pCmdLine->m_fAreaWidth, 0.5f * g_pCmdLine->m_fAreaHeight, 0.0f);
                bSuccess = g_pForest->PopulateRandomly(afExtents, vSptFiles, g_pCmdLine->m_nNumTrees);
            }
            else
                IdvError("No SPT files were found in folder [%s]", strForestDir.c_str( ));
        }

        if (bSuccess)
        {
#ifdef REPORT_TIMES
            cTimer.Stop( );
            IdvReport("[%d] tree forest populated in %g ms", g_pForest->NumTrees( ), cTimer.GetMilliSec( ));
#endif
            // the camera starts in the center of the scene
            const float* pExtents = g_pForest->Extents( );

            float afCenter[3] =
            {
                0.5f * (pExtents[0] + pExtents[3]),
#ifdef UPVECTOR_POS_Y
                0.25f * (pExtents[1] + pExtents[4]),
                0.5f * (pExtents[2] + pExtents[5])
#else
                0.5f * (pExtents[1] + pExtents[4]),
                0.25f * (pExtents[2] + pExtents[5])
#endif
            };
            g_pCurrentNavigation->SetCameraPos(afCenter);

            // adjust world extents to encompass this forest's extents
            idv::Region cLocalForest;
            cLocalForest.m_cMin.Set(pExtents[0], pExtents[1], pExtents[2]);
            cLocalForest.m_cMax.Set(pExtents[3], pExtents[4], pExtents[5]);
            g_cWorldExtents = g_cWorldExtents + cLocalForest;

            // reasonable navigation speed based on the tallest tree
#ifdef UPVECTOR_POS_Y
            float fLongestExtent = pExtents[4] - pExtents[1]; 
#else
            float fLongestExtent = pExtents[5] - pExtents[2]; 
#endif
            g_pCurrentNavigation->SetSpeedScales(fLongestExtent / 500.0f);

            // setup near/far clip values
            g_pForest->SetProjection(g_fAspectRatio, g_fFieldOfView, g_fNearClipPlane, g_fFarClipPlane);
            g_pForest->SetLodDistances(g_pCmdLine->m_fNearLod, g_pCmdLine->m_fFarLod);
        }
        else
        {
            (void) MessageBoxA(NULL, "FATAL ERROR - Unable to continue", "SpeedTree", MB_ICONERROR | MB_OK);
            exit(-1);
        }
    }

    // update auto-camera extents
    g_cAutoCamera.SetExtents(g_cWorldExtents);
    
    g_cTimer.Start( );

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  Display

void Display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity( );
    gluPerspective(g_fFieldOfView, g_fAspectRatio, g_fNearClipPlane, g_fFarClipPlane);

    glMatrixMode(GL_MODELVIEW);
    const float* pCameraMatrix = g_pCurrentNavigation->GetCameraTransform( );
    glLoadMatrixf(pCameraMatrix);

    // draw forest(s)
    if (g_pCmdLine->m_bOverlappingForests)
    {
        for (int nForest = 0; nForest < g_nNumForests; ++nForest)
            if (g_pForestBank->at(nForest)->IsVisible( ))
                g_pForestBank->at(nForest)->Draw(g_uiRenderFlags);
    }
    else
        g_pForest->Draw(g_uiRenderFlags);

    // draw ground/environment
    g_pForest->DrawGround( );

    // draw overlay
    if (g_bDrawOverlay)
        g_pForest->DrawOverlay( );

    glutSwapBuffers( );
}


///////////////////////////////////////////////////////////////////////  
//  Idle

void Idle(void)
{
    static int nFrameCount = -2;

    CheckTime( );

    // advance navigation
    bool bSwitchCandidate = g_pCurrentNavigation->Advance(g_fElapsedTime, g_pForest);

    // culling & LOD computations
    float afProjection[16];
    glGetFloatv(GL_PROJECTION_MATRIX, afProjection);
    if (g_pCmdLine->m_bOverlappingForests)
    {
        for (int nForest = 0; nForest < g_nNumForests; ++nForest)
        {
            g_pForestBank->at(nForest)->UpdateCamera(g_pCurrentNavigation->GetCameraPos( ), afProjection, g_pCurrentNavigation->GetCameraTransform( ), nFrameCount < 0);

            // advance wind
            g_pForestBank->at(nForest)->SetTime(g_fCurrentTime);
        }
    }
    else
    {
        g_pForest->UpdateCamera(g_pCurrentNavigation->GetCameraPos( ), afProjection, g_pCurrentNavigation->GetCameraTransform( ), nFrameCount < 0);

        // advance wind
        g_pForest->SetTime(g_fCurrentTime);
    }

    // report on even time intervals
    const float c_fReportInterval = 0.75f;
    static float fLastReportTime = g_fCurrentTime;

    ++nFrameCount;
    float fTimeSinceLastReport = g_fCurrentTime - fLastReportTime;

    if (fTimeSinceLastReport >= c_fReportInterval)
    {
        const CSpeedTreeForest::SDrawStats& sStats = g_pForest->GetDrawStats( );
        float fFrameRate = nFrameCount / fTimeSinceLastReport;
        printf("\t%.1f fps, %.1f ms, %.1f Ktris, %.2f Mtris/sec, %d draw calls, %d 3D trees, %d BBs, binds: %d\n", 
            fFrameRate, 
            1000.0f / fFrameRate, 
            sStats.m_nNumTrianglesDrawn / 1000.0f,
            sStats.m_nNumTrianglesDrawn * fFrameRate / 1000000.0f,
            sStats.m_nNumDrawCalls,
            sStats.m_nNum3DTrees,
            sStats.m_nNumBBs,
            sStats.m_nBufferBinds);

        fLastReportTime = g_fCurrentTime;
        nFrameCount = 0;
    }

    // switch forest if necessary and applicable
    if (bSwitchCandidate && g_pForestBank && g_pForestBank->size( ) > 0)
    {
        int nIndex = IdvRandom(0, 10000) % (int) (g_pForestBank->size( ));
        g_pForest = g_pForestBank->at(nIndex);
    }

    // check and report OpenGL errors
    GLenum eError = glGetError( );
    if (eError != GL_NO_ERROR)
        fprintf(stderr, "<error> OpenGL error [%s]\n", gluErrorString(eError));
    while (eError != GL_NO_ERROR)
        eError = glGetError( );

    glutPostWindowRedisplay(g_nWindow);
}


///////////////////////////////////////////////////////////////////////  
//  Visible

void Visible(int nVisible)
{
    if (nVisible == GLUT_VISIBLE)
        glutIdleFunc(Idle);
    else
        glutIdleFunc(NULL);
}


///////////////////////////////////////////////////////////////////////  
//  Key

void Key(unsigned char chKey, int , int )
{
    if (chKey >= '1' && chKey <= '9')
    {
        int nIndex = int(chKey - '1');
        if (nIndex < g_nNumForests)
        {
            if (g_pCmdLine->m_bOverlappingForests)
                g_pForestBank->at(nIndex)->ToggleVisibility( );
            else 
                g_pForest = g_pForestBank->at(nIndex);
        }
    }
    else
    {
        switch (chKey)
        {
        case 27: // quit
            ExitApp( );
            break;
        case 's': case 'S':
            {
                for (int nForest = 0; nForest < g_nNumForests; ++nForest)
                    g_pForestBank->at(nForest)->DebugSpin(-0.025f);
            }
            break;
        case 'd': case 'D':
            {
                for (int nForest = 0; nForest < g_nNumForests; ++nForest)
                    g_pForestBank->at(nForest)->DebugSpin(0.025f);
            }
            break;
        case 'b': case 'B': // toggle branches
            g_uiRenderFlags ^= CSpeedTreeForest::BRANCHES;
            break;
        case 'l': case 'L': // toggle leaves
            g_uiRenderFlags ^= CSpeedTreeForest::LEAVES;
            break;
        case 'f': case 'F': // toggle fronds
            g_uiRenderFlags ^= CSpeedTreeForest::FRONDS;
            break;
        case 'z': case 'Z': // toggle billboards
            g_uiRenderFlags ^= CSpeedTreeForest::BILLBOARDS;
            break;
        case 'g': case 'G': // toggle wind strength
            {
                for (int nForest = 0; nForest < g_nNumForests; ++nForest)
                    g_pForestBank->at(nForest)->ToggleWindStrength( );
            }
            break;
        case 'a': case 'A': // toggle alpha testing
            {
                for (int nForest = 0; nForest < g_nNumForests; ++nForest)
                    g_pForestBank->at(nForest)->ToggleAlphaTesting( );
            }
            break;
        case 'w': case 'W': // toggle wireframe
            {
                for (int nForest = 0; nForest < g_nNumForests; ++nForest)
                    g_pForestBank->at(nForest)->ToggleWireframe( );
            }
            break;
        case 'n': case 'N': // toggle normal mapping
            {
                for (int nForest = 0; nForest < g_nNumForests; ++nForest)
                    g_pForestBank->at(nForest)->ToggleNormalMapping( );
            }
            break;
        case 'o': case 'O': // toggle overlay
            g_bDrawOverlay = !g_bDrawOverlay;
            break;
        case ',': case '<': // zoom into overlay
            {
                for (int nForest = 0; nForest < g_nNumForests; ++nForest)
                    g_pForestBank->at(nForest)->OverlayZoom(-1.05f);
            }
            break;
        case '.': case '>': // zoom into overlay
            {
                for (int nForest = 0; nForest < g_nNumForests; ++nForest)
                    g_pForestBank->at(nForest)->OverlayZoom(1.05f);
            }
            break;
        case 'x': case 'X': // rotate light
            {
                static float fLightTime = 0.0f;
                fLightTime += 0.013f;
                const float c_fSpeed = 7.0f;
                float afLightDir[3];
                Assign3d(afLightDir, cosf(c_fSpeed * fLightTime), sinf(c_fSpeed * fLightTime), 1.0f);
                idv::Vec3 cLightDir(afLightDir);
                cLightDir.Normalize( );
                float afLightColor[3] = { 1.0f, 1.0f, 1.0f };
                for (int nForest = 0; nForest < g_nNumForests; ++nForest)
                    g_pForestBank->at(nForest)->SetLighting(cLightDir, afLightColor, g_pCmdLine->m_fLightScale);
            }
            break;
        case 'c': case 'C': // toggle demo camera
            if (g_pCurrentNavigation == &g_cMouseNavigation)
                g_pCurrentNavigation = &g_cAutoCamera;
            else
                g_pCurrentNavigation = &g_cMouseNavigation;
            break;
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  MouseClick

void MouseClick(int nButton, int nState, int nX, int nY)
{
    CMouseNavigation::EMouseButton eNavButton = CMouseNavigation::BUTTON_NONE;
    if (nButton == 0)
        eNavButton = CMouseNavigation::BUTTON_LEFT;
    else if (nButton == 1)
        eNavButton = CMouseNavigation::BUTTON_MIDDLE;
    else if (nButton == 2)
        eNavButton = CMouseNavigation::BUTTON_RIGHT;

    // shift/ctrl to speed or slow
    int nModifiers = glutGetModifiers( );
    g_pCurrentNavigation->SetShiftKeyState((nModifiers & GLUT_ACTIVE_SHIFT) != 0);
    g_pCurrentNavigation->SetControlKeyState((nModifiers & GLUT_ACTIVE_CTRL) != 0);

    g_pCurrentNavigation->MouseButton(eNavButton, !nState, nX, nY);
}


///////////////////////////////////////////////////////////////////////  
//  MouseMotion

void MouseMotion(int nX, int nY)
{
    g_pCurrentNavigation->MouseMotion(nX, nY);
}


///////////////////////////////////////////////////////////////////////  
//  Reshape

void Reshape(int nWidth, int nHeight)
{
    glViewport(0, 0, nWidth, nHeight);
    g_fAspectRatio = nWidth / static_cast<float>(nHeight);

    for (int nForest = 0; nForest < g_nNumForests; ++nForest)
    {
        g_pForestBank->at(nForest)->SetWindowBounds(nWidth, nHeight);
        g_pForestBank->at(nForest)->SetProjection(g_fAspectRatio, g_fFieldOfView, g_fNearClipPlane, g_fFarClipPlane);
    }
}


///////////////////////////////////////////////////////////////////////  
//  CheckTime

void CheckTime(void)
{
    g_cTimer.Stop( );
    g_fCurrentTime = g_cTimer.GetSec( );
    g_fElapsedTime = g_fCurrentTime - g_fLastFrameTime;

    g_fLastFrameTime = g_fCurrentTime;
}


///////////////////////////////////////////////////////////////////////  
//  ExitApp

void ExitApp(void)
{
    try
    {
        for (int nForest = 0; nForest < g_nNumForests; ++nForest)
        {
            st_delete<CSpeedTreeForest>(g_pForestBank->at(nForest), "ExitApp, g_pForest->at(nForest)");
        }
        st_delete<st_vector_forestp>(g_pForestBank, "ExitApp, g_pForestBank");
        st_delete<CIdvCommandLineParser>(g_pCmdLine, "ExitApp, g_pCmdLine");
    }

    catch (...)
    {
        IdvError("Unknown exception thrown when deleting CSpeedTreeForest object");
    }

#if defined(USE_CUSTOM_ALLOCATOR) & defined(TRACK_ALLOC_STATS)
    g_cAllocator.Report("c:/tmp/speedtree_leak_report_opengl.txt");
#endif
    exit(0);
}

