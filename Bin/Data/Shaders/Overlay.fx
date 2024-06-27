///////////////////////////////////////////////////////////////////////  
//  Overlay Shaders
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
//
//  *** Release Version 4.1 ***


///////////////////////////////////////////////////////////////////////  
//  Overlay-specific global variables

float4      g_vOverlayColor;


///////////////////////////////////////////////////////////////////////  
//  Overlay VS Ouput, PS Input

struct SOverlayOutput
{
    float4 vPosition    : POSITION;
};


///////////////////////////////////////////////////////////////////////  
//  OverlayVS
//
//  Overlay vertex shader

SOverlayOutput OverlayVS(float4 vPosition : POSITION)
{
    // this will be fed to the overlay pixel shader
    SOverlayOutput sOutput;
    
    // project to screen
    sOutput.vPosition = mul(g_mModelViewProj, vPosition);

    return sOutput;
}


///////////////////////////////////////////////////////////////////////  
//  OverlayPS

float4 OverlayPS(SOverlayOutput In) : COLOR
{
    return g_vOverlayColor;
}


///////////////////////////////////////////////////////////////////////  
//  Techniques

technique Overlay
{
    pass P0
    {          
        VS_COMPILE_COMMAND OverlayVS( );
        PS_COMPILE_COMMAND OverlayPS( );
    }
}


