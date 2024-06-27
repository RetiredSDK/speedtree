///////////////////////////////////////////////////////////////////////  
//
//  *** INTERACTIVE DATA VISUALIZATION (IDV) PROPRIETARY INFORMATION ***
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Interactive Data Visualization and may
//  not be copied or disclosed except in accordance with the terms of
//  that agreement.
//
//      Copyright (c) 2003-2007 IDV, Inc.
//      All Rights Reserved.
//
//      IDV, Inc.
//      Web: http://www.idvinc.com

#pragma warning(disable : 4723 4995)


///////////////////////////////////////////////////////////////////////  
//  Includes

#include "IdvDirectX9.h"
#include "IdvCommandLineParser.h"
#include "DXUT/dxstdafx.h"
#include "resource.h"
#include "IdvGlobal.h"
#include "SpeedTreeForest.h"
#include "IdvState.h"
#include "IdvMouseNavigation.h"
#include "IdvAutoCamera.h"
#include "IdvVectorMath.h"
#include "IdvFilename.h"
#include "UpVector.h"
#include "ReferenceAllocator.h"
#include "IdvRandom.h"
using namespace std;


///////////////////////////////////////////////////////////////////////  
//  Function prototypes

static  bool    CALLBACK        IsDeviceAcceptable(D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, bool bWindowed, void* pUserContext);
static  bool    CALLBACK        ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings, const D3DCAPS9* pCaps, void* pUserContext);
static  HRESULT CALLBACK        OnCreateDevice(IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext);
static  HRESULT CALLBACK        OnResetDevice(IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext);
static  void    CALLBACK        OnFrameMove(IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext);
static  void    CALLBACK        OnFrameRender(IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext);
static  LRESULT CALLBACK        MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext);
static  void    CALLBACK        KeyboardProc(UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext);
static  void    CALLBACK        MouseProc(bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown, bool bSideButton1Down, bool bSideButton2Down, INT nMouseWheelDelta, INT xPos, INT yPos, void* pUserContext);
static  void    CALLBACK        OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext);
static  void    CALLBACK        OnLostDevice(void* pUserContext);
static  void    CALLBACK        OnDestroyDevice(void* pUserContext);
static  PCHAR*                  CommandLineToArgvA(PCHAR CmdLine, int* _argc);


///////////////////////////////////////////////////////////////////////  
//  Global variables

static  CSpeedTreeForest*       g_pForest = NULL;
static  st_vector_forestp*      g_pForestBank = NULL;
static  int                     g_nNumForests = 0;
static  unsigned long           g_ulRenderBitVector = CSpeedTreeForest::ALL;
static  bool                    g_bDrawOverlay = false;
static  CMouseNavigation        g_cMouseNavigation;
static  CAutoCamera             g_cAutoCamera;
static  CNavigationBase*        g_pCurrentNavigation = &g_cMouseNavigation;
static  idv::Region             g_cWorldExtents;
static  float                   g_fNearClipPlane = 5.0f;
static  float                   g_fFarClipPlane = 1000.0f;
static  float                   g_fFieldOfView = 40.0f;
static  float                   g_fAspectRatio = 4.0f / 3.0f;
static  float                   g_fAccumTime = 0.0f;
static  float                   g_fElapsedTime = 0.0f;
static  float                   g_afBgColor[4] = { 209.0f / 255.0f, 235.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f };
static  CIdvCommandLineParser*  g_pCmdLine = NULL;
#ifdef USE_CUSTOM_ALLOCATOR
static  CReferenceAllocator     g_cReferenceAllocator;
#endif


///////////////////////////////////////////////////////////////////////  
//  WinMain
//
// Entry point to the program. Initializes everything, and goes into a
// message-processing loop. Idle time is used to render the scene.

INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE hIgnored, LPSTR lpCmdLine, INT)
{
#ifdef USE_CUSTOM_ALLOCATOR
    CSpeedTreeRT::SetAllocator(&g_cReferenceAllocator);
#endif

    // create console
    AllocConsole( );                 // Allocate console window
    freopen("CONOUT$", "a", stderr); // Redirect stderr to console
    freopen("CONOUT$", "a", stdout); // Redirect stdout also
    freopen("CONIN$", "r", stdin);
    SetConsoleTitleA("Interactive Data Visualization Console Window");

    // parse the command line
    int argc = 0;
    char** argv = CommandLineToArgvA(GetCommandLineA( ), &argc);
    g_pCmdLine = new ("WinMain, g_pCmdLine") CIdvCommandLineParser;
    if (!g_pCmdLine->Parse(argc, argv))
        printf("Command-line option malformed or not set - using default values to fill in the gaps\n");
    g_fFarClipPlane = g_pCmdLine->m_fVisibility;

    // setup the forest(s)
    g_nNumForests = int(g_pCmdLine->m_vForestFiles.size( ));
    g_pForestBank = new ("WinMain, g_pForestBank") st_vector_forestp(g_nNumForests);
    for (int nForest = 0; nForest < g_nNumForests; ++nForest)
        g_pForestBank->at(nForest) = new ("WinMain, g_pForestBank->at(nForest)") CSpeedTreeForest;
    g_pForest = g_pForestBank->front( );

    // Set the callback functions. These functions allow the sample framework to notify
    // the application about device changes, user input, and windows messages.  The 
    // callbacks are optional so you need only set callbacks for events you're interested 
    // in. However, if you don't handle the device reset/lost callbacks then the sample 
    // framework won't be able to reset your device since the application must first 
    // release all device resources before resetting.  Likewise, if you don't handle the 
    // device created/destroyed callbacks then the sample framework won't be able to 
    // recreate your device resources.

    DXUTSetCallbackDeviceCreated(OnCreateDevice);
    DXUTSetCallbackDeviceReset(OnResetDevice);
    DXUTSetCallbackDeviceLost(OnLostDevice);
    DXUTSetCallbackDeviceDestroyed(OnDestroyDevice);
    DXUTSetCallbackMsgProc(MsgProc);
    DXUTSetCallbackKeyboard(KeyboardProc);
    DXUTSetCallbackMouse(MouseProc);
    DXUTSetCallbackFrameRender(OnFrameRender);
    DXUTSetCallbackFrameMove(OnFrameMove);

    // Show the cursor and clip it when in full screen
    DXUTSetCursorSettings(true, true);

    // Initialize the sample framework and create the desired Win32 window and Direct3D 
    // device for the application. Calling each of these functions is optional, but they
    // allow you to set several options which control the behavior of the framework.
    DXUTInit(false, true, true); // Parse the command line, handle the default hotkeys, and show msgboxes
    HICON hIcon = LoadIcon(NULL, MAKEINTRESOURCE(IDC_WINDOW));
#ifdef UPVECTOR_POS_Y
    DXUTCreateWindow(L"SpeedTreeRT v4.0 Sample App (DirectX 9.0c), +Y Up Axis", NULL, hIcon);
#else
    DXUTCreateWindow(L"SpeedTreeRT v4.0 Sample App (DirectX 9.0c), +Z Up Axis", NULL, hIcon);
#endif

    UINT uiAdapterOrdinal = D3DADAPTER_DEFAULT;
#ifdef NVPERF
    uiAdapterOrdinal = DXUTGetD3DObject( )->GetAdapterCount( ) - 1;
#endif

    int nSuggestedWidth = ((!g_pCmdLine->m_bForcedResolution && g_pCmdLine->m_bFullscreen) ? 0 : g_pCmdLine->m_nWindowWidth);
    int nSuggestedHeight = ((!g_pCmdLine->m_bForcedResolution && g_pCmdLine->m_bFullscreen) ? 0 : g_pCmdLine->m_nWindowHeight);
    DXUTCreateDevice(uiAdapterOrdinal, !g_pCmdLine->m_bFullscreen, nSuggestedWidth, nSuggestedHeight, IsDeviceAcceptable, ModifyDeviceSettings);

#ifdef NVPERF
    //DXUTToggleREF( );
#endif

    // Pass control to the sample framework for handling the message pump and 
    // dispatching render calls. The sample framework will call your FrameMove 
    // and FrameRender callback when there is idle time between handling window messages.
    DXUTMainLoop( );

    // Perform any application-level cleanup here. Direct3D device resources are released within the
    // appropriate callback functions and therefore don't require any cleanup code here.

    return DXUTGetExitCode( );
}


///////////////////////////////////////////////////////////////////////  
//  IsDeviceAcceptable
//
// Called during device initialization, this code checks the device for some 
// minimum set of capabilities, and rejects those that don't pass by returning E_FAIL.

bool CALLBACK IsDeviceAcceptable(D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, bool bWindowed, void* pUserContext)
{
    // no fallback defined by this app, so reject any device that 
    // doesn't support at least ps1.1
    if (pCaps->PixelShaderVersion < D3DPS_VERSION(1, 1))
        return false;

    // skip backbuffer formats that don't support alpha blending
    IDirect3D9* pD3D = DXUTGetD3DObject( ); 
    if (FAILED(pD3D->CheckDeviceFormat( pCaps->AdapterOrdinal, pCaps->DeviceType,
        AdapterFormat, D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING, 
        D3DRTYPE_TEXTURE, BackBufferFormat)))
        return false;

    return true;
}


///////////////////////////////////////////////////////////////////////  
//  ModifyDeviceSettings
//
// This callback function is called immediately before a device is created to allow the 
// application to modify the device settings. The supplied pDeviceSettings parameter 
// contains the settings that the framework has selected for the new device, and the 
// application can make any desired changes directly to this structure.  Note however that 
// DXUT will not correct invalid device settings so care must be taken 
// to return valid device settings, otherwise IDirect3D9::CreateDevice() will fail.  

bool CALLBACK ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings, const D3DCAPS9* pCaps, void* pUserContext)
{
    // If device doesn't support HW T&L or doesn't support 1.1 vertex shaders in HW 
    // then switch to SWVP.
    if( (pCaps->DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) == 0 ||
        pCaps->VertexShaderVersion < D3DVS_VERSION(1,1) )
    {
        pDeviceSettings->BehaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    }

    // Debugging vertex shaders requires either REF or software vertex processing 
    // and debugging pixel shaders requires REF.  
#ifdef DEBUG_VS
    if( pDeviceSettings->DeviceType != D3DDEVTYPE_REF )
    {
        pDeviceSettings->BehaviorFlags &= ~D3DCREATE_HARDWARE_VERTEXPROCESSING;
        pDeviceSettings->BehaviorFlags &= ~D3DCREATE_PUREDEVICE;                            
        pDeviceSettings->BehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    }
#endif
#ifdef DEBUG_PS
    pDeviceSettings->DeviceType = D3DDEVTYPE_REF;
#endif
    // For the first device created if its a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;
        if( pDeviceSettings->DeviceType == D3DDEVTYPE_REF )
            DXUTDisplaySwitchingToREFWarning();
    }

    return true;
}


///////////////////////////////////////////////////////////////////////  
//  OnCreateDevice
//
// This callback function will be called immediately after the Direct3D device has been 
// created, which will happen during application initialization and windowed/full screen 
// toggles. This is the best location to create D3DPOOL_MANAGED resources since these 
// resources need to be reloaded whenever the device is destroyed. Resources created  
// here should be released in the OnDestroyDevice callback. 

HRESULT CALLBACK OnCreateDevice(IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext)
{
    HRESULT hResult = !S_OK;

    // set the rendering context
    CIdvGeometryBuffer::SetDevice(pd3dDevice);
    CIdvTexture::SetDevice(pd3dDevice);
    CIdvState::SetDevice(pd3dDevice);
    CIdvEffect::SetDevice(pd3dDevice);

    // set textures
    CSpeedTreeRT::SetTextureFlip(true);

    // initialize all forests
    for (int nForest = 0; nForest < g_nNumForests; ++nForest)
    {
        const st_string& strForestDir = g_pCmdLine->m_vForestFiles[nForest];
        printf("\n** Initializing forest in folder [%s] (%d of %d) \n", strForestDir.c_str( ), nForest + 1, g_nNumForests);

        g_pForest = g_pForestBank->at(nForest);

        // fog
        g_pForest->SetFogParams(g_pCmdLine->m_fFogStart, g_pCmdLine->m_fFogEnd, g_afBgColor);

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
        bool bSuccess = false;
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

            // reasonable navigation speed based on scene size
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
            MessageBoxA(NULL, "FATAL ERROR - Unable to continue", "SpeedTree", MB_ICONERROR | MB_OK);
            exit(-1);
        }
    }
    g_pForest = g_pForestBank->front( );

    // update auto-camera extents
    g_cAutoCamera.SetExtents(g_cWorldExtents);

    return hResult;
}


///////////////////////////////////////////////////////////////////////  
//  OnResetDevice
//
// This callback function will be called immediately after the Direct3D device has been 
// reset, which will happen after a lost device scenario. This is the best location to 
// create D3DPOOL_DEFAULT resources since these resources need to be reloaded whenever 
// the device is lost. Resources created here should be released in the OnLostDevice 
// callback. 

HRESULT CALLBACK OnResetDevice(IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext)
{
    return S_OK;
}


///////////////////////////////////////////////////////////////////////  
//  OnFrameMove
//
// This callback function will be called once at the beginning of every frame. This is the
// best location for your application to handle updates to the scene, but is not 
// intended to contain actual rendering calls, which should instead be placed in the 
// OnFrameRender callback.  

void CALLBACK OnFrameMove(IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext)
{
    // compute time
    g_fAccumTime += fElapsedTime;
    g_fElapsedTime = fElapsedTime;

    // advance navigation
    bool bSwitchCandidate = g_pCurrentNavigation->Advance(fElapsedTime, g_pForest);

    // advance wind
    if (g_pCmdLine->m_bOverlappingForests)
    {
        for (int nForest = 0; nForest < g_nNumForests; ++nForest)
            g_pForestBank->at(nForest)->SetTime(g_fAccumTime);
    }
    else
        g_pForest->SetTime(g_fAccumTime);

    // report on even time intervals
    const float c_fReportInterval = 0.75f;
    static float fLastReportTime = g_fAccumTime;
    static int nFrameCount = 0;

    ++nFrameCount;
    float fTimeSinceLastReport = g_fAccumTime - fLastReportTime;

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

        fLastReportTime = g_fAccumTime;
        nFrameCount = 0;
    }

    // switch forest if necessary and applicable
    if (bSwitchCandidate && g_pForestBank && g_pForestBank->size( ) > 0)
    {
        int nIndex = IdvRandom(0, 10000) % (int) (g_pForestBank->size( ));
        g_pForest = g_pForestBank->at(nIndex);
    }
}


///////////////////////////////////////////////////////////////////////  
//  OnFrameRender
//
// This callback function will be called at the end of every frame to perform all the 
// rendering calls for the scene, and it will also be called if the window needs to be 
// repainted. After this function has returned, DXUT will call 
// IDirect3DDevice9::Present to display the contents of the next buffer in the swap chain

void CALLBACK OnFrameRender(IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext)
{
    // clear the viewport
    pd3dDevice->Clear(0L, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_COLORVALUE(g_afBgColor[0], g_afBgColor[1], g_afBgColor[2], g_afBgColor[3]), 1.0f, 0L);

    // begin the scene
    if (SUCCEEDED(pd3dDevice->BeginScene( )))
    {
        // set up view
        static D3DXMATRIX matProjection;
        D3DXMatrixPerspectiveFovRH(&matProjection, D3DXToRadian(g_fFieldOfView), FLOAT(g_pCmdLine->m_nWindowWidth) / FLOAT(g_pCmdLine->m_nWindowHeight), g_fNearClipPlane, g_fFarClipPlane);
        pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProjection);
        const D3DXMATRIX* pModelview = (D3DXMATRIX*) g_pCurrentNavigation->GetCameraTransform( );
        pd3dDevice->SetTransform(D3DTS_VIEW, pModelview);
        
        // setup composite matrix for shader
        D3DXMATRIX matBlendShader;
        D3DXMatrixMultiply(&matBlendShader, pModelview, &matProjection);

        // draw forest(s)
        if (g_pCmdLine->m_bOverlappingForests)
        {
            for (int nForest = 0; nForest < g_nNumForests; ++nForest)
            {
                if (g_pForestBank->at(nForest)->IsVisible( ))
                {
                    g_pForestBank->at(nForest)->SetWindowBounds(g_pCmdLine->m_nWindowWidth, g_pCmdLine->m_nWindowHeight);

                    // update the forest camera
                    g_pForestBank->at(nForest)->UpdateCamera(g_pCurrentNavigation->GetCameraPos( ), (const float*) &matProjection, (const float*) pModelview);

                    // render the forest
                    g_pForestBank->at(nForest)->Draw(g_ulRenderBitVector);
                }
            }
        }
        else
        {
            g_pForest->SetWindowBounds(g_pCmdLine->m_nWindowWidth, g_pCmdLine->m_nWindowHeight);

            // update the forest camera
            g_pForest->UpdateCamera(g_pCurrentNavigation->GetCameraPos( ), (const float*) &matProjection, (const float*) pModelview);

            // render the forest
            g_pForest->Draw(g_ulRenderBitVector);
        }

        // draw ground
        g_pForest->DrawGround( );

        // draw overlay
        if (g_bDrawOverlay)
            g_pForest->DrawOverlay( );

        // end the scene
        pd3dDevice->EndScene( );
    }
}


///////////////////////////////////////////////////////////////////////  
//  MsgProc
//
// Before handling window messages, DXUT passes incoming windows 
// messages to the application through this callback function. If the application sets 
// *pbNoFurtherProcessing to TRUE, then DXUT will not process this message.

LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext)
{
    unsigned int nX = 0, nY = 0;
    CMouseNavigation::EMouseButton eNavButton = CMouseNavigation::BUTTON_NONE;

    // handle keyboard and mouse events
    nX = int(LOWORD(lParam));
    nY = int(HIWORD(lParam));

    switch (uMsg)
    {
    case WM_LBUTTONDOWN:
        g_pCurrentNavigation->SetShiftKeyState((wParam & MK_SHIFT) != 0);
        g_pCurrentNavigation->SetControlKeyState((wParam & MK_CONTROL) != 0);
        eNavButton = CMouseNavigation::BUTTON_LEFT;
        g_pCurrentNavigation->MouseButton(eNavButton, true, nX, nY);
        SetCapture(hWnd);
        break;
    case WM_LBUTTONUP:
        eNavButton = CMouseNavigation::BUTTON_LEFT;
        g_pCurrentNavigation->MouseButton(eNavButton, false, nX, nY);
        ReleaseCapture( );
        break;
    case WM_RBUTTONDOWN:
        g_pCurrentNavigation->SetShiftKeyState((wParam & MK_SHIFT) != 0);
        g_pCurrentNavigation->SetControlKeyState((wParam & MK_CONTROL) != 0);
        eNavButton = CMouseNavigation::BUTTON_RIGHT;
        g_pCurrentNavigation->MouseButton(eNavButton, true, nX, nY);
        SetCapture(hWnd);
        break;
    case WM_RBUTTONUP:
        eNavButton = CMouseNavigation::BUTTON_RIGHT;
        g_pCurrentNavigation->MouseButton(eNavButton, false, nX, nY);
        ReleaseCapture( );
        return true; // override the context menu
        break;
    case WM_MBUTTONDOWN:
        g_pCurrentNavigation->SetShiftKeyState((wParam & MK_SHIFT) != 0);
        g_pCurrentNavigation->SetControlKeyState((wParam & MK_CONTROL) != 0);
        eNavButton = CMouseNavigation::BUTTON_MIDDLE;
        g_pCurrentNavigation->MouseButton(eNavButton, true, nX, nY);
        SetCapture(hWnd);
        break;
    case WM_MBUTTONUP:
        eNavButton = CMouseNavigation::BUTTON_MIDDLE;
        g_pCurrentNavigation->MouseButton(eNavButton, false, nX, nY);
        ReleaseCapture( );
        break;
    case WM_MOUSEMOVE:
        if (nX > 3000)
            nX = nX - 65535;
        if (nY > 3000)
            nY = nY - 65535;
        g_pCurrentNavigation->MouseMotion(nX, nY);
        break;
    case WM_KEYDOWN:
        if (wParam >= '1' && wParam <= '9')
        {
            int nIndex = int(wParam - '1');
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
            switch (wParam)
            {
            case 'L': // toggle leaf visibility
                g_ulRenderBitVector ^= CSpeedTreeForest::LEAVES;
                break;
            case 'F': // toggle frond visibility
                g_ulRenderBitVector ^= CSpeedTreeForest::FRONDS;
                break;
            case 'B': // toggle branch visibility       
                g_ulRenderBitVector ^= CSpeedTreeForest::BRANCHES;
                break;
            case 'Z': // toggle billboard visibility
                g_ulRenderBitVector ^= CSpeedTreeForest::BILLBOARDS;
                break;
            case 'S': // rotate the trees one way
                {
                    for (int nForest = 0; nForest < g_nNumForests; ++nForest)
                    {
                        g_pForestBank->at(nForest)->DebugSpin(-0.05f);
                        g_pForest->ForceCameraUpdate( );
                    }
                }
                break;
            case 'D': // rotate the trees the other way
                {
                    for (int nForest = 0; nForest < g_nNumForests; ++nForest)
                    {
                        g_pForestBank->at(nForest)->DebugSpin(0.05f);
                        g_pForest->ForceCameraUpdate( );
                    }
                }
                break;
            case 'X': // rotate the light
                {
                    float afLightDir[3];
                    Assign3d(afLightDir, cosf(2.0f * g_fAccumTime), sinf(2.0f * g_fAccumTime), 1.0f);
                    idv::Vec3 cLightDir(afLightDir);
                    cLightDir.Normalize( );
                    float afLightColor[3] = { 1.0f, 1.0f, 1.0f };
                    for (int nForest = 0; nForest < g_nNumForests; ++nForest)
                        g_pForestBank->at(nForest)->SetLighting(cLightDir, afLightColor, g_pCmdLine->m_fLightScale);
                }
                break;
            case 'A':
                {
                    for (int nForest = 0; nForest < g_nNumForests; ++nForest)
                        g_pForestBank->at(nForest)->ToggleAlphaTesting( );
                }
                break;
            case 'W':
                {
                    for (int nForest = 0; nForest < g_nNumForests; ++nForest)
                        g_pForestBank->at(nForest)->ToggleWireframe( );
                }
                break;
            case 'N':
                {
                    for (int nForest = 0; nForest < g_nNumForests; ++nForest)
                        g_pForestBank->at(nForest)->ToggleNormalMapping( );
                }
                break;
            case 'G':
                {
                    for (int nForest = 0; nForest < g_nNumForests; ++nForest)
                        g_pForestBank->at(nForest)->ToggleWindStrength( );
                }
                break;
            case 'O':
                g_bDrawOverlay = !g_bDrawOverlay;
                break;
            case 188: // zoom into overlay
                if (g_bDrawOverlay)
                    g_pForest->OverlayZoom(-1.05f);
                break;
            case 190: // zoom into overlay
                if (g_bDrawOverlay)
                    g_pForest->OverlayZoom(1.05f);
                break;
            case 'C': // toggle demo camera
                if (g_pCurrentNavigation == &g_cMouseNavigation)
                    g_pCurrentNavigation = &g_cAutoCamera;
                else
                    g_pCurrentNavigation = &g_cMouseNavigation;
                break;
            }
        }
        break;
    case WM_PAINT:
        break;
    }

    return S_OK;
}


///////////////////////////////////////////////////////////////////////  
//  KeyboardProc
//
// As a convenience, DXUT inspects the incoming windows messages for
// keystroke messages and decodes the message parameters to pass relevant keyboard
// messages to the application.  The framework does not remove the underlying keystroke 
// messages, which are still passed to the application's MsgProc callback.

void CALLBACK KeyboardProc(UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext)
{
}


///////////////////////////////////////////////////////////////////////  
//  MouseProc

void CALLBACK MouseProc(bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown, bool bSideButton1Down, bool bSideButton2Down, INT nMouseWheelDelta, INT xPos, INT yPos, void* pUserContext)
{
}


///////////////////////////////////////////////////////////////////////  
//  OnLostDevice
//
// This callback function will be called immediately after the Direct3D device has 
// entered a lost state and before IDirect3DDevice9::Reset is called. Resources created
// in the OnResetDevice callback should be released here, which generally includes all 
// D3DPOOL_DEFAULT resources. See the "Lost Devices" section of the documentation for 
// information about lost devices.

void CALLBACK OnLostDevice(void* pUserContext)
{
}


///////////////////////////////////////////////////////////////////////  
//  OnDestroyDevice
//
// This callback function will be called immediately after the Direct3D device has 
// been destroyed, which generally happens as a result of application termination or 
// windowed/full screen toggles. Resources created in the OnCreateDevice callback 
// should be released here, which generally includes all D3DPOOL_MANAGED resources. 

void CALLBACK OnDestroyDevice(void* pUserContext)
{
    for (int nForest = 0; nForest < g_nNumForests; ++nForest)
    {
        st_delete<CSpeedTreeForest>(g_pForestBank->at(nForest), "OnDestroyDevice, g_pForest->at(nForest)");
    }
    st_delete<st_vector_forestp>(g_pForestBank, "OnDestroyDevice, g_pForestBank");
    st_delete<CIdvCommandLineParser>(g_pCmdLine, "OnDestroyDevice, g_pCmdLine");
}


///////////////////////////////////////////////////////////////////////  
//  CommandLineToArgvA
//
// Taken from: http://alter.org.ua/docs/win/args/index.php?lang=en - not
// formatted to IDV/SpeedTree coding standard.

PCHAR* CommandLineToArgvA(PCHAR CmdLine, int* _argc)
{
    PCHAR* argv;
    PCHAR  _argv;
    ULONG   len;
    ULONG   argc;
    CHAR   a;
    ULONG   i, j;

    BOOLEAN  in_QM;
    BOOLEAN  in_TEXT;
    BOOLEAN  in_SPACE;

    len = (ULONG) strlen(CmdLine);
    i = ((len+2)/2)*sizeof(PVOID) + sizeof(PVOID);

    argv = (PCHAR*)GlobalAlloc(GMEM_FIXED,
        i + (len+2)*sizeof(CHAR));

    _argv = (PCHAR)(((PUCHAR)argv)+i);

    argc = 0;
    argv[argc] = _argv;
    in_QM = FALSE;
    in_TEXT = FALSE;
    in_SPACE = TRUE;
    i = 0;
    j = 0;

    a = CmdLine[i];
    while(a) {
        if(in_QM) {
            if(a == '\"') {
                in_QM = FALSE;
            } else {
                _argv[j] = a;
                j++;
            }
        } else {
            switch(a) {
                case '\"':
                    in_QM = TRUE;
                    in_TEXT = TRUE;
                    if(in_SPACE) {
                        argv[argc] = _argv+j;
                        argc++;
                    }
                    in_SPACE = FALSE;
                    break;
                case ' ':
                case '\t':
                case '\n':
                case '\r':
                    if(in_TEXT) {
                        _argv[j] = '\0';
                        j++;
                    }
                    in_TEXT = FALSE;
                    in_SPACE = TRUE;
                    break;
                default:
                    in_TEXT = TRUE;
                    if(in_SPACE) {
                        argv[argc] = _argv+j;
                        argc++;
                    }
                    _argv[j] = a;
                    j++;
                    in_SPACE = FALSE;
                    break;
            }
        }
        i++;
        a = CmdLine[i];
    }
    _argv[j] = '\0';
    argv[argc] = NULL;

    (*_argc) = argc;
    return argv;
}


