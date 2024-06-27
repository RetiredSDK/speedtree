///////////////////////////////////////////////////////////////////////  
//  Frond Shaders
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
//  Frond-specific global variables

// vs (none)

// ps (none)


///////////////////////////////////////////////////////////////////////  
//  Texture samplers

// (none specific to the frond geometry)


///////////////////////////////////////////////////////////////////////  
//  Frond VS Ouput, PS Input

struct SFrondOutput
{
    float4 vPosition            : POSITION;
#ifdef INTERMEDIATE_FRONDBRANCH_FADING
    float3 vDiffuseTexCoords    : TEXCOORD0;    // .z = lod fade hint
#else
    float2 vDiffuseTexCoords    : TEXCOORD0;
#endif
#ifdef SELF_SHADOW_LAYER
    float2 vSelfShadowTexCoords : TEXCOORD1;
#endif
#ifdef FROND_NORMAL_MAPPING
#ifdef USE_FOG
    float4 vNormal              : TEXCOORD2;
#else
    float3 vNormal              : TEXCOORD2;
#endif
#else
    float4 vColor               : COLOR0;
#ifdef USE_FOG
    float fFog                  : TEXCOORD2;    // using FOG here causes a ps_2_0 compilation failure
#endif
#endif
};


///////////////////////////////////////////////////////////////////////  
//  FrondVS
//
//  Frond geometry vertex shader

SFrondOutput FrondVS(float4     vPosition   : POSITION,
                     float4     vNormal     : TEXCOORD0,
                     float4     vTexCoords1 : TEXCOORD1
#if defined(INTERMEDIATE_FRONDBRANCH_FADING) || defined(FROND_NORMAL_MAPPING)
                     , float4   vMisc       : TEXCOORD2
#endif
                     )
{
    // this will be fed to the frond pixel shader
    SFrondOutput sOutput;

    // define attribute aliases for readability
    float2 vSelfShadowTexCoords = float2(vPosition.w, vNormal.w);
    float2 vDiffuseTexCoords = float2(vTexCoords1.xy);
    float2 vWindParams = float2(vTexCoords1.zw);
    
    // scale the geometry (each tree instance can be uniquely scaled)
    vPosition.xyz *= g_fTreeScale;
    
    // rotate the whole tree (each tree instance can be uniquely rotated) - use optimized z-axis rotation
    // algorithm where float(sin(a), cos(a), -sin(a), 0.0f) is uploaded instead of angle a
#ifdef UPVECTOR_POS_Y
    vPosition.xz = float2(dot(g_vTreeRotationTrig.ywz, vPosition.xyz), dot(g_vTreeRotationTrig.xwy, vPosition.xyz));
    vNormal.xz = float2(dot(g_vTreeRotationTrig.ywz, vNormal.xyz), dot(g_vTreeRotationTrig.xwy, vNormal.xyz));
#else
    vPosition.xy = float2(dot(g_vTreeRotationTrig.yxw, vPosition.xyz), dot(g_vTreeRotationTrig.zyw, vPosition.xyz));
    vNormal.xy = float2(dot(g_vTreeRotationTrig.yxw, vNormal.xyz), dot(g_vTreeRotationTrig.zyw, vNormal.xyz));
#endif
    
    // compute new position from wind effect
    vPosition.xyz = WindEffect(vPosition.xyz, vWindParams);
    
    // translate tree into position (must be done after the rotation)
    vPosition.xyz += g_vTreePos.xyz;

#ifdef FROND_NORMAL_MAPPING
    // setup per-pixel normal mapping  (assumes normalized light direction)
    float3 vTangent = vMisc.xyz;
   
    // rotate the tangent by this instance's rotation and derive the binormal from the normal & tangent 
    // in order to keep vertex attribute count lower
#ifdef UPVECTOR_POS_Y
    vTangent.xz = float2(dot(g_vTreeRotationTrig.ywz, vTangent.xyz), dot(g_vTreeRotationTrig.xwy, vTangent.xyz));
#else
    vTangent.xy = float2(dot(g_vTreeRotationTrig.yxw, vTangent.xyz), dot(g_vTreeRotationTrig.zyw, vTangent.xyz));
#endif
    float3 vBinormal = -cross(vNormal.xyz, vTangent);
    
    // 0.5f + 0.5f * expr not used since texcoords aren't clamped to [0,1] range - this approach saves
    // several instructions
    sOutput.vNormal.x = dot(g_vLightDir.xyz, vTangent.xyz);
    sOutput.vNormal.y = dot(g_vLightDir.xyz, vBinormal.xyz);
    sOutput.vNormal.z = dot(g_vLightDir.xyz, vNormal.xyz);
#else
    // compute the frond lighting (not using normal mapping, but per-vertex lighting)
    sOutput.vColor = float4(LightDiffuse(vPosition.xyz, vNormal.xyz, g_vLightDir.xyz, g_vLightDiffuse.xyz, g_vMaterialDiffuse.xyz), 1.0f);
#endif
    
#ifdef SELF_SHADOW_LAYER
    // move self-shadow texture coordinates based on wind strength - motion computed on CPU and uploaded
    sOutput.vSelfShadowTexCoords = vSelfShadowTexCoords + g_vSelfShadowMotion;
#endif
    
#ifdef INTERMEDIATE_FRONDBRANCH_FADING
    float fLodFadeHint = vMisc.w;
    sOutput.vDiffuseTexCoords.z = ComputeFadeValue(fLodFadeHint);
#endif
    
    // project position to the screen
    sOutput.vPosition = mul(g_mModelViewProj, float4(vPosition.xyz, 1.0f));
    
    // pass through other texcoords exactly as they were received
    sOutput.vDiffuseTexCoords.xy = vDiffuseTexCoords;

#ifdef USE_FOG      
    // calc fog (cheap in vertex shader, relatively expensive later in the pixel shader)
#ifdef FROND_NORMAL_MAPPING
    sOutput.vNormal.w = FogValue(sOutput.vPosition.z);
#else
    sOutput.fFog = FogValue(sOutput.vPosition.z);
#endif
#endif
    
    return sOutput;
}


///////////////////////////////////////////////////////////////////////  
//  FrondPS
//
//  Frond geometry pixel shader.  This shader can process up to two texture layers:
//      - diffuse composite leaf/frond map
//      - self-shadow map (greyscale composite map generated via SpeedTreeCAD)

float4 FrondPS(SFrondOutput In) : COLOR
{
#ifdef FROND_NORMAL_MAPPING
    // since the normal map normal values (normally ranged [-1,1]) are stored
    // as a color value (ranged [0,1]), they must be uncompressed.  a dot product 
    // is used to compute the diffuse lighting contribution.
    float4 texNormal = tex2D(samCompositeNormalLeafMap, In.vDiffuseTexCoords.xy);
    const float3 vHalves = { 0.5f, 0.5f, 0.5f };
    float fDot = saturate(dot(2.0f * (texNormal.rgb - vHalves), In.vNormal.rgb));
#endif
    
    // look up the diffuse layer
    float4 texDiffuse = tex2D(samCompositeDiffuseLeafMap, In.vDiffuseTexCoords.xy);
    
    // compute the ambient contribution (pulled from the diffuse map)
    float3 vAmbient = texDiffuse.xyz * g_vMaterialAmbient.xyz;

    // compute the full lighting equation, including diffuse and ambient values and
    // their respective scales.
#ifdef FROND_NORMAL_MAPPING
    float3 vColor3 = texDiffuse.rgb * fDot * g_fDiffuseScale * g_vLightDiffuse.rgb;
#else
    float3 vColor3 = In.vColor.rgb * texDiffuse.rgb * g_fDiffuseScale;
#endif
 
#ifdef SELF_SHADOW_LAYER
    // overlay the shadow if active
    float4 texShadow = tex2D(samSelfShadowMap, In.vSelfShadowTexCoords.xy);
    vColor3 *= texShadow.xyz;
#endif    

    // compute the full lighting value
    float4 vColor = float4(vColor3 + vAmbient, texDiffuse.a);
    
#ifdef INTERMEDIATE_FRONDBRANCH_FADING
    float fFade = In.vDiffuseTexCoords.z;
    vColor.a *= fFade;
#endif
  
#ifdef USE_FOG
    // if fog is active, interpolate between the unfogged color and the fog color
    // based on vertex shader fog value
#ifdef FROND_NORMAL_MAPPING
    float fFogValue = In.vNormal.w;
#else
    float fFogValue = In.fFog;
#endif
    vColor.xyz = lerp(g_vFogColor, vColor, fFogValue).xyz;
#endif

    return vColor;
}


///////////////////////////////////////////////////////////////////////  
//  Techniques

technique Fronds
{
    pass P0
    {          
        VS_COMPILE_COMMAND FrondVS( );
        PS_COMPILE_COMMAND FrondPS( );
    }
}


