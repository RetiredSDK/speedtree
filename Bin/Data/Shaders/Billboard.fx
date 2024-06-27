///////////////////////////////////////////////////////////////////////  
//  Billboard Shaders
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
//  Billboard-specific global variables

// vs
float4      g_v360TexCoords[NUM_360_IMAGES] REG(22); // each element defines the texcoords for a single billboard image - each element has:
                                                     //     x = s-coord (rightmost s-coord of billboard)
                                                     //     y = t-coord (topmost t-coord of billboard) 
                                                     //     z = s-axis width (leftmost s-coord = x - z)
                                                     //     w = t-axis height (bottommost t-coord = y - w)
float       g_fSpecialAzimuth;                       // camera azimuth, adjusted to speed billboard vs computations
float4x4    g_amBBNormals;                           // all billboards share the same normals since they all face the camera
float4x4    g_amBBBinormals;                         // all billboards share the same binormals since they all face the camera  
float4x4    g_amBBTangents;                          // all billboards share the same tangents since they all face the camera
float4x4    g_mBBUnitSquare =                        // unit billboard card that's turned towards the camera.  card is aligned on 
            {                                        // YZ plane and centered at (0.0f, 0.0f, 0.5f)
                float4(0.0f, 0.5f, 1.0f, 0.0f), 
                float4(0.0f, -0.5f, 1.0f, 0.0f), 
                float4(0.0f, -0.5f, 0.0f, 0.0f), 
                float4(0.0f, 0.5f, 0.0f, 0.0f) 
            };
float4x4    g_afTexCoordScales =                     // used to compress & optimize the g_v360TexCoord lookups (x = s scale, y = t scale)
            { 
                float4(0.0f, 0.0f, 0.0f, 0.0f), 
                float4(1.0f, 0.0f, 0.0f, 0.0f), 
                float4(1.0f, 1.0f, 0.0f, 0.0f), 
                float4(0.0f, 1.0f, 0.0f, 0.0f) 
            };

// ps
texture     g_tBillboardDiffuseMap;                  // composite billboard diffuse map
texture     g_tBillboardNormalMap;                   // composite billboard normal map
float       g_fBillboardLightAdjust REG(18);         // used to assist in accuracy of normal map lit bb's matching the 3D tree
#ifdef HORZ_BILLBOARDS
float       g_fBillboardFadeValue;                   // [0,1], 0 = transparent, 1 = opaque
#endif


///////////////////////////////////////////////////////////////////////  
//  Billboard texture samplers

sampler2D samBillboardDiffuseMap : register(s0) = sampler_state
{
    Texture = <g_tBillboardDiffuseMap>;
#ifdef IDV_DIRECTX9
    // Trade-off choice here - no filter on the billboard textures makes the match the 3D alpha-tested leaf geometry better during 
    // LOD transition, but a linear/anisotropic filter makes for better billboards when standing alone (no transition context). 
    // Choose between better transitions or better looking billboards.
    //MagFilter = SPEEDTREE_FILTER;
    //MipFilter = SPEEDTREE_FILTER;
    //MinFilter = SPEEDTREE_FILTER;
#endif
};

#ifdef BILLBOARD_NORMAL_MAPPING
sampler2D samBillboardNormalMap : register(s1) = sampler_state
{
    Texture = <g_tBillboardNormalMap>;
#ifdef IDV_DIRECTX9
    // Trade-off choice here - no filter on the billboard textures makes the match the 3D alpha-tested leaf geometry better during 
    // LOD transition, but a linear/anisotropic filter makes for better billboards when standing alone (no transition context). 
    // Choose between better transitions or better looking billboards.
    //MagFilter = SPEEDTREE_FILTER;
    //MipFilter = SPEEDTREE_FILTER;
    //MinFilter = SPEEDTREE_FILTER;
#endif
};
#endif


///////////////////////////////////////////////////////////////////////  
//  SBillboardOutput structure

struct SBillboardOutput
{
    float4  vPosition         : POSITION;
    float2  vDiffuseTexCoords : TEXCOORD0;
    float2  vNormalTexCoords  : TEXCOORD1;
#ifdef BILLBOARD_NORMAL_MAPPING
    float4  vNormal           : TEXCOORD2;
#else
    float   fNormal           : TEXCOORD2;
#endif

#ifdef USE_FOG
    float4  vLightAdjusts     : TEXCOORD3;
#else
    float3  vLightAdjusts     : TEXCOORD3; 
#endif
};


///////////////////////////////////////////////////////////////////////  
//  Billboard1VS
//
//  In order to ensure smooth LOD transitions, two billboards are rendered
//  per tree instance.  Each billboard represents a partially faded rendering
//  of the two closest billboard images for the current camera azimuth and
//  current tree instance rotation.
//
//  Separate shaders are necessary because since different equations are used
//  to pick the billboard index and fade values for the two bb's.

SBillboardOutput Billboard1VS(float4 vPosition      : POSITION,     // xyz = position, w = corner index
                              float4 vGeom          : TEXCOORD0,    // x = width, y = height, z = tree azimuth, w = lod fade
                              float4 vMiscParams    : TEXCOORD1,    // x = scale, y = texcoord offset, z = num images, w = 360 transition %
                              float3 vLightAdjusts  : TEXCOORD2)    // x = bright side adjustment, y = dark side adjustment, z = ambient scale
{
    //global float4 g_v360TexCoords[NUM_360_IMAGES];
    
    // this will be fed to the billboard pixel shader
    SBillboardOutput sOutput;
    
    // define attribute aliases for readability
    float fAzimuth = g_vCameraAngles.x;         // current camera azimuth
    float fPitch = g_vCameraAngles.y;           // current camera pitch
    int nCorner = vPosition.w;                  // which card corner this vertex represents [0,3]
    int nNumImages = vMiscParams.z;             // # of 360-degree images
    float c_fSliceDiameter = c_fTwoPi / float(nNumImages); // diameter = 360 / g_nNum360Images
    float c_fLodFade = vGeom.w;                 // computed on CPU - the amount the billboard as a whole is faded from 3D geometry
    float c_fTreeScale = vMiscParams.x;         // uniform scale of tree instance
    float c_fTransitionPercentage = vMiscParams.w;  // controls how thick or thin the 360-degree image transition is 
    int nTexCoordTableOffset = vMiscParams.y;   // offset into g_v360TexCoords where this instance's texcoords begin
    
    // there are two azimuth values to consider:
    //    1) fAzimuth: the azimuth of the camera position
    //    2) fAdjustedAzimuth: the azimuth of the camera plus the orientation of the tree the billboard 
    //                         represents (used to determine which bb image to use and its alpha value)
    
    // modify the adjusted azimuth to appear in range [0,2*pi]
    float fAdjustedAzimuth = g_fSpecialAzimuth + vGeom.z;
    if (fAdjustedAzimuth < 0.0f)
        fAdjustedAzimuth += c_fTwoPi;
    else if (fAdjustedAzimuth > c_fTwoPi)
        fAdjustedAzimuth -= c_fTwoPi;

    // pick the billboard image index and access the extract texcoords from the table
    int nIndex0 = int(fAdjustedAzimuth / c_fSliceDiameter + 1);
    if (nIndex0 > nNumImages - 1)
        nIndex0 = 0;

    // determine texcoords based on corner position - while not a straighforward method for determining the texcoords
    // for a specific corner, this one provided a good compromise of speed and space
    float4 vTexCoords = g_v360TexCoords[nIndex0 + nTexCoordTableOffset];
    sOutput.vDiffuseTexCoords.x = vTexCoords.x - vTexCoords.z * g_afTexCoordScales[nCorner].x;
    sOutput.vDiffuseTexCoords.y = vTexCoords.y - vTexCoords.w * g_afTexCoordScales[nCorner].y;
    sOutput.vNormalTexCoords = sOutput.vDiffuseTexCoords;

    // compute the alpha fade value
#ifdef SUPPORT_360_BILLBOARDS
    float fAlpha0 = 1.0f - Modulate_Float(fAdjustedAzimuth, c_fSliceDiameter) / c_fSliceDiameter;
#else
    float fAlpha0 = 0.0f;
#endif
    float fFadePoint = lerp(c_fClearAlpha, c_fOpaqueAlpha, c_fLodFade);

    // 4.0
    //fAlpha0 = lerp(fFadePoint, c_fClearAlpha, pow(fAlpha0, 1.7));
    
    // 4.1 (helps reduce a too-faded look)
    fAlpha0 = max(fAlpha0, c_fTransitionPercentage);
    fAlpha0 = lerp(fFadePoint, c_fClearAlpha, pow( (fAlpha0 - c_fTransitionPercentage) / (1.0f - c_fTransitionPercentage), 1.7f));
    
    // each billboard may be faded at a distinct value, but it isn't efficient to change
    // the alpha test value per billboard.  instead we adjust the alpha value of the 
    // billboards's outgoing color to achieve the same effect against a static alpha test 
    // value (c_gOpaqueAlpha).
    fAlpha0 = 1.0f - (fAlpha0 - c_fOpaqueAlpha) / c_fAlphaSpread;

    // multiply by the correct corner
#ifdef UPVECTOR_POS_Y
    float3 vecCorner = g_mBBUnitSquare[nCorner].xzy * vGeom.xyx * c_fTreeScale;
#else
    float3 vecCorner = g_mBBUnitSquare[nCorner].xyz * vGeom.xxy * c_fTreeScale;
#endif

    // apply rotation to scaled corner
#ifdef UPVECTOR_POS_Y
    vecCorner.xz = float2(dot(g_vCameraAzimuthTrig.ywz, vecCorner.xyz), dot(g_vCameraAzimuthTrig.xwy, vecCorner.xyz));
#else
    vecCorner.xy = float2(dot(g_vCameraAzimuthTrig.yxw, vecCorner.xyz), dot(g_vCameraAzimuthTrig.zyw, vecCorner.xyz));
#endif
    vPosition.xyz += vecCorner;
    vPosition.w = 1.0f;
    
#ifdef BILLBOARD_NORMAL_MAPPING
    // setup per-pixel normal mapping  (assumes normalized light direction)

    // 0.5f + 0.5f * expr not used since texcoords aren't clamped to [0,1] range - this approach saves
    // several instructions
    sOutput.vNormal.x = dot(g_vLightDir.xyz, g_amBBTangents[nCorner].xyz);
    sOutput.vNormal.y = dot(g_vLightDir.xyz, g_amBBBinormals[nCorner].xyz);
    sOutput.vNormal.z = dot(g_vLightDir.xyz, g_amBBNormals[nCorner].xyz);
    sOutput.vNormal.w = fAlpha0;
#else
    sOutput.fNormal = fAlpha0;
#endif
    
    // project to the screen
    sOutput.vPosition = mul(g_mModelViewProj, vPosition);

    // pass through light adjustments
    sOutput.vLightAdjusts.xyz = vLightAdjusts.xyz;
    
#ifdef USE_FOG      
    // calc fog (cheap in vertex shader, relatively expensive later in the pixel shader)
    sOutput.vLightAdjusts.w = FogValue(sOutput.vPosition.z);
#endif
    
    return sOutput;
}


///////////////////////////////////////////////////////////////////////  
//  Billboard2VS
//
//  In order to ensure smooth LOD transitions, two billboards are rendered
//  per tree instance.  Each billboard represents a partially faded rendering
//  of the two closest billboard images for the current camera azimuth and
//  current tree instance rotation.
//
//  Separate shaders are necessary because since different equations are used
//  to pick the billboard index and fade values for the two bb's.

SBillboardOutput Billboard2VS(float4 vPosition      : POSITION,     // xyz = position, w = corner index
                              float4 vGeom          : TEXCOORD0,    // x = width, y = height, z = tree azimuth, w = lod fade
                              float4 vMiscParams    : TEXCOORD1,    // x = scale, y = texcoord offset, z = num images, w = 360 transition %
                              float3 vLightAdjusts  : TEXCOORD2)    // x = bright side adjustment, y = dark side adjustment, z = ambient scale
{
    // this will be fed to the billboard pixel shader
    SBillboardOutput sOutput;
    
    // define attribute aliases for readability
    float fAzimuth = g_vCameraAngles.x;         // current camera azimuth
    float fPitch = g_vCameraAngles.y;           // current camera pitch
    int nCorner = vPosition.w;                  // which card corner this vertex represents [0,3]
    int nNumImages = vMiscParams.z;             // # of 360-degree images
    float c_fSliceDiameter = c_fTwoPi / float(nNumImages); // diameter = 360 / g_nNum360Images
    float c_fLodFade = vGeom.w;                 // computed on CPU - the amount the billboard as a whole is faded from 3D geometry
    float c_fTreeScale = vMiscParams.x;         // uniform scale of tree instance
    float c_fTransitionPercentage = vMiscParams.w;  // controls how thick or thin the 360-degree image transition is 
    int nTexCoordTableOffset = vMiscParams.y;   // offset into g_v360TexCoords where this instance's texcoords begin
    
    // there are two azimuth values to consider:
    //    1) fAzimuth: the azimuth of the camera position
    //    2) fAdjustedAzimuth: the azimuth of the camera plus the orientation of the tree the billboard 
    //                         represents (used to determine which bb image to use and its alpha value)

    // modify the adjusted azimuth to appear in range [0,2*pi]
    float fAdjustedAzimuth = g_fSpecialAzimuth + vGeom.z;
    if (fAdjustedAzimuth < 0.0f)
        fAdjustedAzimuth += c_fTwoPi;
    else if (fAdjustedAzimuth > c_fTwoPi)
        fAdjustedAzimuth -= c_fTwoPi;
    
    // pick the index and access the texcoords
    int nIndex1 = int(fAdjustedAzimuth / c_fSliceDiameter);
    if (nIndex1 > nNumImages - 1)
        nIndex1 = 0;
        
    // determine texcoords based on corner position - while not a straighforward method for determining the texcoords
    // for a specific corner, this one provided a good compromise of speed and space
    float4 vTexCoords = g_v360TexCoords[nIndex1 + nTexCoordTableOffset];
    sOutput.vDiffuseTexCoords.x = vTexCoords.x - vTexCoords.z * g_afTexCoordScales[nCorner].x;
    sOutput.vDiffuseTexCoords.y = vTexCoords.y - vTexCoords.w * g_afTexCoordScales[nCorner].y;
    sOutput.vNormalTexCoords = sOutput.vDiffuseTexCoords;
  
    // compute the alpha fade value
#ifdef SUPPORT_360_BILLBOARDS
    float fAlpha1 = (fAdjustedAzimuth - (nIndex1 * c_fSliceDiameter)) / c_fSliceDiameter;
#else
    float fAlpha1 = 0.0f;
#endif
    float fFadePoint = lerp(c_fClearAlpha, c_fOpaqueAlpha, c_fLodFade);

    // 4.0
    //fAlpha1 = lerp(fFadePoint, c_fClearAlpha, pow(fAlpha1, 1.7));
    
    // 4.1 (helps reduce a too-faded look)
    fAlpha1 = max(fAlpha1, c_fTransitionPercentage);
    fAlpha1 = lerp(fFadePoint, c_fClearAlpha, pow( (fAlpha1 - c_fTransitionPercentage) / (1.0f - c_fTransitionPercentage), 1.7f));
    
    // each billboard may be faded at a distinct value, but it isn't efficient to change
    // the alpha test value per billboard.  instead we adjust the alpha value of the 
    // billboards's outgoing color to achieve the same effect against a static alpha test 
    // value (c_gOpaqueAlpha).
    fAlpha1 = 1.0f - (fAlpha1 - c_fOpaqueAlpha) / c_fAlphaSpread;

    // multiply by the correct corner
#ifdef UPVECTOR_POS_Y
    float3 vecCorner = g_mBBUnitSquare[nCorner].xzy * vGeom.xyx * c_fTreeScale;
#else
    float3 vecCorner = g_mBBUnitSquare[nCorner].xyz * vGeom.xxy * c_fTreeScale;
#endif

    // apply rotation to scaled corner
#ifdef UPVECTOR_POS_Y
    vecCorner.xz = float2(dot(g_vCameraAzimuthTrig.ywz, vecCorner.xyz), dot(g_vCameraAzimuthTrig.xwy, vecCorner.xyz));
#else
    vecCorner.xy = float2(dot(g_vCameraAzimuthTrig.yxw, vecCorner.xyz), dot(g_vCameraAzimuthTrig.zyw, vecCorner.xyz));
#endif
    vPosition.xyz += vecCorner;
    vPosition.w = 1.0f;
    
#ifdef BILLBOARD_NORMAL_MAPPING
    // setup per-pixel normal mapping  (assumes normalized light direction)

    // 0.5f + 0.5f * expr not used since texcoords aren't clamped to [0,1] range - this approach saves
    // several instructions
    sOutput.vNormal.x = dot(g_vLightDir.xyz, g_amBBTangents[nCorner].xyz);
    sOutput.vNormal.y = dot(g_vLightDir.xyz, g_amBBBinormals[nCorner].xyz);
    sOutput.vNormal.z = dot(g_vLightDir.xyz, g_amBBNormals[nCorner].xyz);
    sOutput.vNormal.w = fAlpha1;
#else
    sOutput.fNormal = fAlpha1;
#endif

    // project to the screen
    sOutput.vPosition = mul(g_mModelViewProj, vPosition);

    // pass through light adjustments
    sOutput.vLightAdjusts.xyz = vLightAdjusts.xyz;

#ifdef USE_FOG      
    // calc fog (cheap in vertex shader, relatively expensive later in the pixel shader)
    sOutput.vLightAdjusts.w = FogValue(sOutput.vPosition.z);
#endif

    return sOutput;
}


#ifdef HORZ_BILLBOARDS
///////////////////////////////////////////////////////////////////////  
//  HorzBillboardVS

SBillboardOutput HorzBillboardVS(float4 vPosition      : POSITION,     // xyz = position, w = ground offset
                                 float4 vGeom          : TEXCOORD0,    // x = corner-x, y = corner-y, z = tree azimuth, w = lod fade
                                 float3 vMiscParams    : TEXCOORD1,    // x = scale, y = s texcoord, z = t texcoord
                                 float3 vLightAdjusts  : TEXCOORD2)    // x = bright side adjustment, y = dark side adjustment, z = ambient scale
{
    // this will be fed to the billboard pixel shader
    SBillboardOutput sOutput;
    
    // define attribute aliases for readability
    float c_fLodFade = vGeom.w;                 // computed on CPU - the amount the billboard as a whole is faded from 3D geometry
    float c_fTreeScale = vMiscParams.x;
    float c_fTreeAzimuth = vGeom.z;
#ifdef UPVECTOR_POS_Y
    float3 c_vNormal = float3(0.0f, 1.0f, 0.0f);
    float3 c_vTangent = float3(-1.0f, 0.0f, 0.0f);
    float3 c_vBinormal = float3(0.0f, 0.0f, 1.0f);
#else
    float3 c_vNormal = float3(0.0f, 0.0f, 1.0f);
    float3 c_vTangent = float3(1.0f, 0.0f, 0.0f);
    float3 c_vBinormal = float3(0.0f, 1.0f, 0.0f);
#endif

    // compute alpha fade/fizzle value
    float fFadePoint = lerp(c_fClearAlpha, c_fOpaqueAlpha, c_fLodFade);
    float fAlpha = lerp(fFadePoint, c_fClearAlpha, g_fBillboardFadeValue);
    
    // each billboard may be faded at a distinct value, but it isn't efficient to change
    // the alpha test value per billboard.  instead we adjust the alpha value of the 
    // billboards's outgoing color to achieve the same effect against a static alpha test 
    // value (c_gOpaqueAlpha).
    fAlpha = 1.0f - (fAlpha - c_fOpaqueAlpha) / c_fAlphaSpread;
    
#ifdef BILLBOARD_NORMAL_MAPPING
    // setup per-pixel normal mapping  (assumes normalized light direction)
    sOutput.vNormal.x = dot(g_vLightDir.xyz, c_vTangent);
    sOutput.vNormal.y = dot(g_vLightDir.xyz, c_vNormal);
    sOutput.vNormal.z = dot(g_vLightDir.xyz, c_vBinormal);
    sOutput.vNormal.w = fAlpha;
#else
    sOutput.fNormal = fAlpha;
#endif

    // pass on texcoords
    sOutput.vDiffuseTexCoords = vMiscParams.yz;
    sOutput.vNormalTexCoords = vMiscParams.yz;
    
    // scale and rotate the corner
#ifdef UPVECTOR_POS_Y
    float3 vCorner = float3(vGeom.x, vPosition.w, vGeom.y) * c_fTreeScale;
    vCorner.xz = Rotate_yAxis(c_fTreeAzimuth, vCorner);
#else
    float3 vCorner = float3(vGeom.xy, vPosition.w) * c_fTreeScale;
    vCorner.xy = Rotate_zAxis(c_fTreeAzimuth, vCorner);
#endif
    
    // translate corner piece to base tree position
    vPosition.xyz += vCorner;
    
    // project to the screen
    vPosition.w = 1.0f;
    sOutput.vPosition = mul(g_mModelViewProj, vPosition);

    // pass through light adjustments
    sOutput.vLightAdjusts.xyz = vLightAdjusts.xyz;

#ifdef USE_FOG      
    // calc fog (cheap in vertex shader, relatively expensive later in the pixel shader)
    sOutput.vLightAdjusts.w = FogValue(sOutput.vPosition.z);
#endif
   
    return sOutput;
}
#endif


///////////////////////////////////////////////////////////////////////  
//  BillboardPS
//
//  Billboard geometry pixel shader
//
//  Not having BILLBOARD_NORMAL_MAPPING #define'd makes for a much shorter
//  pixel shader - one that should be used on lower compile targets

float4 BillboardPS(SBillboardOutput In) : COLOR
{
#ifndef BILLBOARD_NORMAL_MAPPING
    float4 texDiffuse = tex2D(samBillboardDiffuseMap, In.vDiffuseTexCoords.xy);
    texDiffuse.a *= In.fNormal;

#ifdef USE_FOG
    // calc fog (cheap in vertex shader, relatively expensive later in the pixel shader)
    texDiffuse.xyz = lerp(g_vFogColor.xyz, texDiffuse.xyz, In.fFog);
#endif
    
    return texDiffuse;
    
#else // BILLBOARD_NORMAL_MAPPING

    // look up the diffuse and normal-map layers
    float4 texDiffuse = tex2D(samBillboardDiffuseMap, In.vDiffuseTexCoords.xy);
    float4 texNormal = tex2D(samBillboardNormalMap, In.vNormalTexCoords.xy);

    // compute the ambient value
    const float4 vecNoiseOffset = { -0.32f, -0.32f, -0.32f, 0.0f };
    float4 vecAmbient = (texNormal.aaaa + vecNoiseOffset) * In.vLightAdjusts.z;
    vecAmbient = vecAmbient * texDiffuse;
    
    // since the normal map normal values (normally ranged [-1,1]) are store
    // as a color value (ranged [0,1]), they must be uncompressed.  a dot product 
    // is used to compute the diffuse lighting contribution.
    const float3 vecHalves = { 0.5f, 0.5f, 0.5f };
    float fDot = saturate(dot(texNormal.rgb - vecHalves, In.vNormal.rgb)) * 2.0f;

    // lighting adjustment to use SpeedTreeCAD parameters to help make the bb
    // match the 3D geometry
    float fLightAdjust = lerp(In.vLightAdjusts.x, In.vLightAdjusts.y, g_fBillboardLightAdjust);
    float4 vColor = (fDot * texDiffuse * fLightAdjust) + vecAmbient;

    // use an alpha value that is the diffuse map's noisy alpha channel multiplied
    // against the faded alpha color from the vertex shader
    vColor.a = texDiffuse.a * In.vNormal.w;

#ifdef USE_FOG
    // calc fog (cheap in vertex shader, relatively expensive later in the pixel shader)
    vColor.xyz = lerp(g_vFogColor.xyz, vColor.xyz, In.vLightAdjusts.w);
#endif

    return vColor;
#endif // BILLBOARD_NORMAL_MAPPING
}


///////////////////////////////////////////////////////////////////////  
//  Techniques

technique Billboards1
{
    pass P0
    {          
        VS_COMPILE_COMMAND Billboard1VS( );
        PS_COMPILE_COMMAND BillboardPS( );
    }
}

technique Billboards2
{
    pass P0
    {          
        VS_COMPILE_COMMAND Billboard2VS( );
        PS_COMPILE_COMMAND BillboardPS( );
    }
}

#ifdef HORZ_BILLBOARDS
technique HorzBillboards
{
    pass P0
    {          
        VS_COMPILE_COMMAND HorzBillboardVS( );
        PS_COMPILE_COMMAND BillboardPS( );
    }
}
#endif

