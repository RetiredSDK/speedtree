@echo off
::
::  SpeedTree Shader Compilation Utility
::
::  *** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
::
::  This software is supplied under the terms of a license agreement or
::  nondisclosure agreement with Interactive Data Visualization, Inc. and
::  may not be copied, disclosed, or exploited except in accordance with 
::  the terms of that agreement.
::
::      Copyright (c) 2003-2007 IDV, Inc.
::      All rights reserved in all media.
::
::      IDV, Inc.
::      Web: http://www.idvinc.com
::
::  *** Release Version 4.1 ***

setlocal


if "%1" == "" goto usage

:: Set up #defines to configure shaders
set DEFINES=-DNUM_WIND_MATRICES=7
set DEFINES=%DEFINES% -DMAX_NUM_LEAF_ANGLES=20
set DEFINES=%DEFINES% -DNUM_360_IMAGES=63
set DEFINES=%DEFINES% -DBRANCH_DETAIL_LAYER
set DEFINES=%DEFINES% -DBRANCH_NORMAL_MAPPING
set DEFINES=%DEFINES% -DFROND_NORMAL_MAPPING
set DEFINES=%DEFINES% -DLEAF_NORMAL_MAPPING
set DEFINES=%DEFINES% -DUSE_FOG
set DEFINES=%DEFINES% -DSELF_SHADOW_LAYER
set DEFINES=%DEFINES% -DSUPPORT_360_BILLBOARDS
set DEFINES=%DEFINES% -DBILLBOARD_NORMAL_MAPPING
set DEFINES=%DEFINES% -DHORZ_BILLBOARDS
set DEFINES=%DEFINES% -DINTERMEDIATE_FRONDBRANCH_FADING


if "%1" == "PS3" goto Ps3Cg
if "%1" == "FX" goto FX
if "%1" == "Cg" goto WinCg
goto usage

::
:: ----------- PS3 Cg Compiler -----------------------------------------------------------------------------------------------
::

:Ps3Cg
set VP_PROFILE=sce_vp_rsx
set FP_PROFILE=sce_fp_rsx
set COMPILER=sce-cgc.exe
set DEFINES=%DEFINES% -DIDV_OPENGL
set DEFINES=%DEFINES% -DPS3

::Vertex Programs
echo. 
echo *** Compiling BranchVS ***
%COMPILER% %DEFINES% -fastmath -fastprecision -entry BranchVS -o PS3/Branch.vpo -v -profile %VP_PROFILE% SpeedTree.fx
echo. 
echo *** Compiling FrondVS ***
%COMPILER% %DEFINES% -fastmath -fastprecision -entry FrondVS -o PS3/Frond.vpo -v -profile %VP_PROFILE% SpeedTree.fx
echo. 
echo *** Compiling LeafCardVS ***
%COMPILER% %DEFINES% -fastmath -fastprecision -entry LeafCardVS -o PS3/LeafCard.vpo -v -profile %VP_PROFILE% SpeedTree.fx
echo. 
echo *** Compiling LeafMeshVS ***
%COMPILER% %DEFINES% -fastmath -fastprecision -entry LeafMeshVS -o PS3/LeafMesh.vpo -v -profile %VP_PROFILE% SpeedTree.fx
echo. 
echo *** Compiling Billboard1VS ***
%COMPILER% %DEFINES% -fastmath -fastprecision -entry Billboard1VS -o PS3/Billboard1.vpo -v -profile %VP_PROFILE% SpeedTree.fx
echo. 
echo *** Compiling Billboard2VS ***
%COMPILER% %DEFINES% -fastmath -fastprecision -entry Billboard2VS -o PS3/Billboard2.vpo -v -profile %VP_PROFILE% SpeedTree.fx
echo. 
echo *** Compiling HorzBillboardVS ***
%COMPILER% %DEFINES% -fastmath -fastprecision -entry HorzBillboardVS -o PS3/HorzBillboard.vpo -v -profile %VP_PROFILE% SpeedTree.fx
echo. 
echo *** Compiling OverlayVS ***
%COMPILER% %DEFINES% -fastmath -fastprecision -entry OverlayVS -o PS3/Overlay.vpo -v -profile %VP_PROFILE% SpeedTree.fx
echo. 
echo *** Compiling GroundVS ***
%COMPILER% %DEFINES% -fastmath -fastprecision -entry GroundVS -o PS3/Ground.vpo -v -profile %VP_PROFILE% Ground.fx

::Fragment Programs
echo. 
echo *** Compiling BranchPS ***
%COMPILER% %DEFINES% -fastmath -fastprecision -entry BranchPS -o PS3/Branch.fpo -v -profile %FP_PROFILE% SpeedTree.fx
echo. 
echo *** Compiling FrondPS ***
%COMPILER% %DEFINES% -fastmath -fastprecision -entry FrondPS -o PS3/Frond.fpo -v -profile %FP_PROFILE% SpeedTree.fx
echo. 
echo *** Compiling LeafCardPS ***
%COMPILER% %DEFINES% -fastmath -fastprecision -entry LeafPS -o PS3/LeafCard.fpo -v -profile %FP_PROFILE% SpeedTree.fx
echo. 
echo *** Compiling LeafMeshPS ***
%COMPILER% %DEFINES% -fastmath -fastprecision -entry LeafPS -o PS3/LeafMesh.fpo -v -profile %FP_PROFILE% SpeedTree.fx
echo. 
echo *** Compiling BillboardPS ***
%COMPILER% %DEFINES% -fastmath -fastprecision -entry BillboardPS -o PS3/Billboard.fpo -v -profile %FP_PROFILE% SpeedTree.fx
echo. 
echo *** Compiling OverlayPS ***
%COMPILER% %DEFINES% -fastmath -fastprecision -entry OverlayPS -o PS3/Overlay.fpo -v -profile %FP_PROFILE% SpeedTree.fx
echo. 
echo *** Compiling GroundPS ***
%COMPILER% %DEFINES% -fastmath -fastprecision -entry GroundPS -o PS3/Ground.fpo -v -profile %FP_PROFILE% Ground.fx

goto complete


::
:: ----------- Windows Cg Compiler --------------------------------------------------------------------------------------------
::

:WinCg
set VP_PROFILE=arbvp1
set FP_PROFILE=arbfp1
set COMPILER=cgc
set DEFINES=%DEFINES% -DIDV_OPENGL

::Vertex Programs
echo. 
echo *** Compiling BranchVS ***
%COMPILER% %DEFINES% -fastmath -fastprecision -entry BranchVS -v -profile %VP_PROFILE% SpeedTree.fx
echo. 
echo *** Compiling FrondVS ***
%COMPILER% %DEFINES% -fastmath -fastprecision -entry FrondVS -v -profile %VP_PROFILE% SpeedTree.fx
echo. 
echo *** Compiling LeafCardVS ***
%COMPILER% %DEFINES% -fastmath -fastprecision -entry LeafCardVS -v -profile %VP_PROFILE% SpeedTree.fx
echo.
echo *** Compiling LeafMeshVS ***
%COMPILER% %DEFINES% -fastmath -fastprecision -entry LeafMeshVS -v -profile %VP_PROFILE% SpeedTree.fx
echo. 
echo *** Compiling Billboard1VS ***
%COMPILER% %DEFINES% -fastmath -fastprecision -entry Billboard1VS -v -profile %VP_PROFILE% SpeedTree.fx
echo. 
echo *** Compiling Billboard2VS ***
%COMPILER% %DEFINES% -fastmath -fastprecision -entry Billboard2VS -v -profile %VP_PROFILE% SpeedTree.fx
echo. 
echo *** Compiling HorzBillboardVS ***
%COMPILER% %DEFINES% -fastmath -fastprecision -entry HorzBillboardVS -v -profile %VP_PROFILE% SpeedTree.fx
echo. 
echo *** Compiling OverlayVS ***
%COMPILER% %DEFINES% -fastmath -fastprecision -entry OverlayVS -v -profile %VP_PROFILE% SpeedTree.fx
echo. 
echo *** Compiling GroundVS ***
%COMPILER% %DEFINES% -fastmath -fastprecision -entry GroundVS -v -profile %VP_PROFILE% Ground.fx

::Fragment Programs
echo. 
echo *** Compiling BranchPS ***
%COMPILER% %DEFINES% -fastmath -fastprecision -entry BranchPS -v -profile %FP_PROFILE% SpeedTree.fx
echo. 
echo *** Compiling FrondPS ***
%COMPILER% %DEFINES% -fastmath -fastprecision -entry FrondPS -v -profile %FP_PROFILE% SpeedTree.fx
echo. 
echo *** Compiling LeafPS ***
%COMPILER% %DEFINES% -fastmath -fastprecision -entry LeafPS -v -profile %FP_PROFILE% SpeedTree.fx
echo. 
echo *** Compiling BillboardPS ***
%COMPILER% %DEFINES% -fastmath -fastprecision -entry BillboardPS -v -profile %FP_PROFILE% SpeedTree.fx
echo. 
echo *** Compiling OverlayPS ***
%COMPILER% %DEFINES% -fastmath -fastprecision -entry OverlayPS -v -profile %FP_PROFILE% SpeedTree.fx
echo. 
echo *** Compiling GroundPS ***
%COMPILER% %DEFINES% -fastmath -fastprecision -entry GroundPS -v -profile %FP_PROFILE% Ground.fx

goto complete


::
:: ----------- DirectX FX Compiler (Windows/Xenon version) --------------------------------------------------------------
::

:FX
set VP_PROFILE=vs_2_0
:: there are a lot of problems with pixel targets below 2.0 in the Dec 2006 DK SDK
set FP_PROFILE=ps_2_0
set COMPILER=fxc
set DEFINES=%DEFINES% -DIDV_DIRECTX9
:: if vs_1_1/ps_1_1, make sure LOW_COMPILE_TARGET is #defined
:: set DEFINES=%DEFINES% -DLOW_COMPILE_TARGET

::Vertex Programs
echo. 
echo *** Compiling BranchVS ***
%COMPILER% %DEFINES% /T%VP_PROFILE% /EBranchVS SpeedTree.fx
echo. 
echo *** Compiling FrondVS ***
%COMPILER% %DEFINES% /T%VP_PROFILE% /EFrondVS SpeedTree.fx
echo. 
echo *** Compiling LeafCardVS ***
%COMPILER% %DEFINES% /T%VP_PROFILE% /ELeafCardVS SpeedTree.fx
echo. 
echo *** Compiling LeafMeshVS ***
%COMPILER% %DEFINES% /T%VP_PROFILE% /ELeafMeshVS SpeedTree.fx
echo. 
echo *** Compiling Billboard1VS ***
%COMPILER% %DEFINES% /T%VP_PROFILE% /EBillboard1VS SpeedTree.fx
echo. 
echo *** Compiling Billboard2VS ***
%COMPILER% %DEFINES% /T%VP_PROFILE% /EBillboard2VS SpeedTree.fx
echo. 
echo *** Compiling HorzBillboardVS ***
%COMPILER% %DEFINES% /T%VP_PROFILE% /EHorzBillboardVS SpeedTree.fx
echo. 
echo *** Compiling OverlayVS ***
%COMPILER% %DEFINES% /T%VP_PROFILE% /EOverlayVS SpeedTree.fx
echo. 
echo *** Compiling GroundVS ***
%COMPILER% %DEFINES% /T%VP_PROFILE% /EGroundVS Ground.fx


::Fragment Programs
echo. 
echo *** Compiling BranchPS ***
%COMPILER% %DEFINES% /T%FP_PROFILE% /EBranchPS SpeedTree.fx
echo. 
echo *** Compiling FrondPS ***
%COMPILER% %DEFINES% /T%FP_PROFILE% /EFrondPS SpeedTree.fx
echo. 
echo *** Compiling LeafPS ***
%COMPILER% %DEFINES% /T%FP_PROFILE% /ELeafPS SpeedTree.fx
echo. 
echo *** Compiling BillboardPS ***
%COMPILER% %DEFINES% /T%FP_PROFILE% /EBillboardPS SpeedTree.fx
echo. 
echo *** Compiling OverlayPS ***
%COMPILER% %DEFINES% /T%FP_PROFILE% /EOverlayPS SpeedTree.fx
echo. 
echo *** Compiling GroundPS ***
%COMPILER% %DEFINES% /T%FP_PROFILE% /EGroundPS Ground.fx


goto Complete


:usage

echo SpeedTree 4.0 Shader Compilation Utility (c) 2006
echo Usage:
echo     Compile_Shaders.bat [PS3 or FX or Cg]
echo        PS3 = sce-cgc.exe with targets sce_vp_rsx and sce_fp_rsx
echo        FX = fxc.exe with targets vs_3_0 and ps_3_0
echo        Cg = cgc.exe with targets fp40 and fp40


:Complete
pause
