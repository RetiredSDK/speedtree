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
//  Preprocessor

#include "CompileCommands.fx"


///////////////////////////////////////////////////////////////////////  
//  Ground-specific global variables

float4x4    g_mModelViewProj;
float4      g_vGroundColor;
texture     g_tGroundMap;
float4      g_vFogColor;
float3      g_vFogParams;       // used for fog distance calcs: .x = fog start, .y = fog end, .z = end - start


sampler2D samGroundMap : register(s0) = sampler_state
{
    Texture = <g_tGroundMap>;
#ifdef IDV_DIRECTX9
    MagFilter = Linear;
    MipFilter = Linear;
    MinFilter = Linear;
#endif
};


///////////////////////////////////////////////////////////////////////  
//  Ground VS Ouput, PS Input

struct SGroundOutput
{
    float4 vPosition        : POSITION;
#ifdef USE_FOG
    float3 vTexCoords       : TEXCOORD0;  // .xy = diffuse texcoords, .z = fog
#else
    float2 vTexCoords       : TEXCOORD0;
#endif
};


///////////////////////////////////////////////////////////////////////  
//  FogValue
//
//  Simple LINEAR fog computation.  If an exponential equation is desired,
//  it can be placed here - all of the shaders call this one function.

#ifdef USE_FOG
float FogValue(float fPoint)
{
    float fFogEnd = g_vFogParams.y;
    float fFogDist = g_vFogParams.z;
    
    return saturate((fFogEnd - fPoint) / fFogDist);
}
#endif


///////////////////////////////////////////////////////////////////////  
//  GroundVS
//
//  Ground vertex shader

SGroundOutput GroundVS(float4 vPosition  : POSITION,
                       float2 vTexCoords : TEXCOORD0)
{
    // this will be fed to the overlay pixel shader
    SGroundOutput sOutput;
    
    // project to screen
    sOutput.vPosition = mul(g_mModelViewProj, vPosition);
    sOutput.vTexCoords.xy = vTexCoords;
    
#ifdef USE_FOG
    sOutput.vTexCoords.z = FogValue(sOutput.vPosition.z);
#endif

    return sOutput;
}


///////////////////////////////////////////////////////////////////////  
//  GroundPS

float4 GroundPS(SGroundOutput In) : COLOR
{
    // simple texture lookup
    float4 texDiffuse = tex2D(samGroundMap, In.vTexCoords.xy);
    float4 vOutput = texDiffuse * g_vGroundColor;
    
#ifdef USE_FOG
    // if fog is active, interpolate between the unfogged color and the fog color
    // based on vertex shader fog value
    float fFogValue = In.vTexCoords.z;
    vOutput.xyz = lerp(g_vFogColor.xyz, vOutput.xyz, fFogValue);
#endif

    return vOutput;
}


///////////////////////////////////////////////////////////////////////  
//  Techniques

technique Ground
{
    pass P0
    {          
        VS_COMPILE_COMMAND GroundVS( );
        PS_COMPILE_COMMAND GroundPS( );
    }
}



