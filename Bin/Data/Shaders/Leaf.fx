///////////////////////////////////////////////////////////////////////  
//  Leaf Shaders
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
//  Leaf-specific global variables

// vs
float4      g_avLeafAngles[MAX_NUM_LEAF_ANGLES]; // each element: .x = rock angle, .y = rustle angle
                                                 // each element is a float4, even though only a float2 is needed, to facilitate
                                                 // fast uploads on all platforms (one call to upload whole array)
float4      g_vLeafAngleScalars;                 // each tree model has unique scalar values: .x = rock scalar, .y = rustle scalar
float4x4    g_mLeafUnitSquare =                  // unit leaf card that's turned towards the camera and wind-rocked/rustled by the
            {                                    // vertex shader.  card is aligned on YZ plane and centered at (0.0f, 0.0f, 0.0f)
                float4(0.0f, 0.5f, 0.5f, 0.0f), 
                float4(0.0f, -0.5f, 0.5f, 0.0f), 
                float4(0.0f, -0.5f, -0.5f, 0.0f), 
                float4(0.0f, 0.5f, -0.5f, 0.0f)
            };

// ps (none)


///////////////////////////////////////////////////////////////////////  
//  Leaf VS Ouput, PS Input

struct SLeafOutput
{
    float4 vPosition      : POSITION;
    float2 vBaseTexCoords : TEXCOORD0;
#ifdef LEAF_NORMAL_MAPPING
#ifdef USE_FOG
    float4 vNormal        : TEXCOORD1;
#else
    float3 vNormal        : TEXCOORD1;
#endif
#else
    float4 vColor         : COLOR0;
#ifdef USE_FOG
    float fFog            : TEXCOORD1;  // using FOG here causes a ps_2_0 compilation failure
#endif
#endif
};


///////////////////////////////////////////////////////////////////////  
//  LeafCardVS
//
//  Leaf card geometry vertex shader

SLeafOutput LeafCardVS(float4   vPosition  : POSITION,  // xyz = position, w = corner index
                       float4   vTexCoord0 : TEXCOORD0, // xy = diffuse texcoords, zw = compressed wind parameters
                       float4   vTexCoord1 : TEXCOORD1, // .x = width, .y = height, .z = pivot x, .w = pivot.y
                       float4   vTexCoord2 : TEXCOORD2, // .x = angle.x, .y = angle.y, .z = wind angle index, .w = dimming
#ifdef LEAF_NORMAL_MAPPING
                       float3   vTangent   : TEXCOORD3,
#endif
                       float3   vNormal    : NORMAL)
{
    // this will be fed to the leaf pixel shader
    SLeafOutput sOutput;
    
    // define attribute aliases for readability
    float fAzimuth = g_vCameraAngles.x;      // camera azimuth for billboarding
    float fPitch = g_vCameraAngles.y;        // camera pitch for billboarding
    float2 vSize = vTexCoord1.xy;            // leaf card width & height
    int nCorner = vPosition.w;               // which card corner this vertex represents [0,3]
    float fRotAngleX = vTexCoord2.x;         // angle offset for leaf rocking (helps make it distinct)
    float fRotAngleY = vTexCoord2.y;         // angle offset for leaf rustling (helps make it distinct)
    float fWindAngleIndex = vTexCoord2.z;    // which wind matrix this leaf card will follow
    float2 vPivotPoint = vTexCoord1.zw;      // point about which card will rock and rustle
    float fDimming = vTexCoord2.w;           // interior leaves are darker (range = [0.0,1.0])
    float2 vWindParams = vTexCoord0.zw;      // compressed wind parameters

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
    
    // compute rock and rustle values (all trees share the g_avLeafAngles table, but each can be scaled uniquely)
    float2 vLeafRockAndRustle = g_vLeafAngleScalars.xy * g_avLeafAngles[fWindAngleIndex].xy;
        
    // access g_mLeafUnitSquare matrix with corner index and apply scales
#ifdef UPVECTOR_POS_Y
    float3 vPivotedPoint = g_mLeafUnitSquare[nCorner].xzy;
#else
    float3 vPivotedPoint = g_mLeafUnitSquare[nCorner].xyz;
#endif

    // adjust by pivot point so rotation occurs around the correct point
    vPivotedPoint.yz += vPivotPoint;
#ifdef UPVECTOR_POS_Y
    float3 vCorner = vPivotedPoint * vSize.xyx;
#else
    float3 vCorner = vPivotedPoint * vSize.xxy;
#endif

    // rock & rustling on the card corner
    // * note: The bulk of the LeafCardVS instructions come from this section of code. Please use the
    //         code commented out just below this block if you want a quick and dirty approximation
    //         of the wind as tuned in CAD as opposed to this 100% faithful implementation.  Using the
    //         shorter code reduces the compiled shader length by ~25%.
    //         
#ifdef UPVECTOR_POS_Y
    float3x3 matRotation = RotationMatrix_yAxis(fAzimuth + fRotAngleX);
    matRotation = mul(matRotation, RotationMatrix_zAxis(fPitch + fRotAngleY));
    matRotation = mul(matRotation, RotationMatrix_yAxis(-vLeafRockAndRustle.y));
    matRotation = mul(matRotation, RotationMatrix_xAxis(vLeafRockAndRustle.x));
#else
    float3x3 matRotation = RotationMatrix_zAxis(fAzimuth + fRotAngleX);
    matRotation = mul(matRotation, RotationMatrix_yAxis(fPitch + fRotAngleY));
    matRotation = mul(matRotation, RotationMatrix_zAxis(vLeafRockAndRustle.y));
    matRotation = mul(matRotation, RotationMatrix_xAxis(vLeafRockAndRustle.x));
#endif
    /* quick & dirty rock/rustle code
#ifdef UPVECTOR_POS_Y
    float3x3 matRotation = RotationMatrix_yAxis(fAzimuth + fRotAngleX + vLeafRockAndRustle.y);
    matRotation = mul(matRotation, RotationMatrix_zAxis(fPitch + fRotAngleY + vLeafRockAndRustle.x));
#else
    float3x3 matRotation = RotationMatrix_zAxis(fAzimuth + fRotAngleX);
    matRotation = mul(matRotation, RotationMatrix_yAxis(fPitch + fRotAngleY));
    matRotation = mul(matRotation, RotationMatrix_zAxis(vLeafRockAndRustle.y));
    matRotation = mul(matRotation, RotationMatrix_xAxis(vLeafRockAndRustle.x));
#endif
    */
    vCorner = mul(matRotation, vCorner);
    
    // place and scale the leaf card
    vPosition.xyz += vCorner;
    vPosition.xyz *= g_fTreeScale;

    // perturb normal for wind effect (optional - not previewed or tunable in CAD)
    /*
#ifdef UPVECTOR_POS_Y
    vNormal.xyz += 0.15f * vLeafRockAndRustle.xxy;
#else
    vNormal.xyz += 0.15f * vLeafRockAndRustle.xyx;
#endif
    vNormal = normalize(vNormal);
    */

    // translate tree into position (must be done after the rotation)
    vPosition.xyz += g_vTreePos.xyz;

#ifdef LEAF_NORMAL_MAPPING
    // setup per-pixel normal mapping  (assumes normalized light direction)
    
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
    
    sOutput.vNormal.xyz *= fDimming;
#else    
    // compute the leaf lighting (not using normal mapping, but per-vertex lighting)
    sOutput.vColor.rgb = fDimming * LightDiffuse(vPosition.xyz, vNormal.xyz, g_vLightDir, g_vLightDiffuse.rgb, g_vMaterialDiffuse.rgb);
    sOutput.vColor.a = 1.0f;
#endif

    // project position to the screen
    sOutput.vPosition = mul(g_mModelViewProj, float4(vPosition.xyz, 1.0f));
    
    // pass through other texcoords exactly as they were received
    sOutput.vBaseTexCoords.xy = vTexCoord0.xy;

#ifdef USE_FOG      
    // calc fog (cheap in vertex shader, relatively expensive later in the pixel shader)
#ifdef LEAF_NORMAL_MAPPING
    sOutput.vNormal.w = FogValue(sOutput.vPosition.z);
#else
    sOutput.fFog = FogValue(sOutput.vPosition.z);
#endif
#endif

    return sOutput;
}


///////////////////////////////////////////////////////////////////////  
//  LeafMeshVS

SLeafOutput LeafMeshVS(float4   vPosition  : POSITION,   // xyz = position, w = compressed wind param 1
                       float4   vTexCoord0 : TEXCOORD0,  // xy = diffuse texcoords, z = wind angle index, w = dimming
                       float3   vOrientX   : TEXCOORD1,  // xyz = vector xyz
                       float3   vOrientZ   : TEXCOORD2,  // xyz = vector xyz
                       float4   vOffset    : TEXCOORD3,  // xyz = mesh placement position, w = compressed wind param 2
#ifdef LEAF_NORMAL_MAPPING
                       float3   vTangent   : TEXCOORD4,
#endif
                       float3   vNormal    : NORMAL)     // xyz = normal xyz
{
    // this will be fed to the leaf pixel shader
    SLeafOutput sOutput;

    // define attribute aliases for readability
    float fWindAngleIndex = vTexCoord0.z;       // which wind matrix this leaf card will follow
    float fDimming = vTexCoord0.w;              // interior leaves are darker (range = [0.0,1.0])
    float2 vWindParams = float2(vPosition.w, vOffset.w);
    
    // vOffset represents the location where the leaf mesh will be placed - here it is rotated into place
    // and has the wind effect motion applied to it
#ifdef UPVECTOR_POS_Y
    vOffset.xz = float2(dot(g_vTreeRotationTrig.ywz, vOffset.xyz), dot(g_vTreeRotationTrig.xwy, vOffset.xyz));
#else
    vOffset.xy = float2(dot(g_vTreeRotationTrig.yxw, vOffset.xyz), dot(g_vTreeRotationTrig.zyw, vOffset.xyz));
#endif
    vOffset.xyz = WindEffect(vOffset.xyz, vWindParams);
    
    // compute rock and rustle values (all trees share the g_avLeafAngles table), but g_vLeafAngleScalars
    // scales the angles to match wind settings specified in SpeedTreeCAD
    float2 vLeafRockAndRustle = g_vLeafAngleScalars.xy * g_avLeafAngles[fWindAngleIndex].xy;
    
    // vPosition stores the leaf mesh geometry, not yet put into place at position vOffset.
    // leaf meshes rock and rustle, which requires rotations on two axes (rustling is not
    // useful on leaf mesh geometry)
    float3x3 matRockRustle = RotationMatrix_xAxis(vLeafRockAndRustle.x); // rock
    vPosition.xyz = mul(matRockRustle, vPosition.xyz);
    
    // build mesh orientation matrix - cannot be done beforehand on CPU due to wind effect / rotation order issues.
    // it is used to orient each mesh into place at vOffset
    float3 vOrientY = cross(vOrientX, vOrientZ);
#ifdef UPVECTOR_POS_Y
    vOrientY = -vOrientY;
#endif
    float3x3 matOrientMesh =
    {
        vOrientX, vOrientY, vOrientZ
    };
    
    // apply orientation matrix to the mesh positon & normal
    vPosition.xyz = mul(matOrientMesh, vPosition.xyz);
    vNormal.xyz = mul(matOrientMesh, vNormal.xyz);
    
    // rotate the whole tree (each tree instance can be uniquely rotated) - use optimized z-axis rotation
    // algorithm where float(sin(a), cos(a), -sin(a), 0.0f) is uploaded instead of angle a
#ifdef UPVECTOR_POS_Y
    vPosition.xz = float2(dot(g_vTreeRotationTrig.ywz, vPosition.xyz), dot(g_vTreeRotationTrig.xwy, vPosition.xyz));
    vNormal.xz = float2(dot(g_vTreeRotationTrig.ywz, vNormal.xyz), dot(g_vTreeRotationTrig.xwy, vNormal.xyz));
#else
    vPosition.xy = float2(dot(g_vTreeRotationTrig.yxw, vPosition.xyz), dot(g_vTreeRotationTrig.zyw, vPosition.xyz));
    vNormal.xy = float2(dot(g_vTreeRotationTrig.yxw, vNormal.xyz), dot(g_vTreeRotationTrig.zyw, vNormal.xyz));
#endif

    // put oriented mesh into place at rotated and wind-affected vOffset
    vPosition.xyz += vOffset.xyz;

    // scale the geometry (each tree instance can be uniquely scaled)
    vPosition.xyz *= g_fTreeScale;
    
    // translate tree into position (must be done after the rotation)
    vPosition.xyz += g_vTreePos.xyz;
    
    // compute the leaf lighting (not using normal mapping, but per-vertex lighting)
#ifdef LEAF_NORMAL_MAPPING
    // setup per-pixel normal mapping  (assumes normalized light direction)
    
    // rotate the tangent by this instance's rotation and derive the binormal from the normal & tangent 
    // in order to keep vertex attribute count lower
#ifdef UPVECTOR_POS_Y
    vTangent.xz = float2(dot(g_vTreeRotationTrig.ywz, vTangent.xyz), dot(g_vTreeRotationTrig.xwy, vTangent.xyz));
#else
    vTangent.xy = float2(dot(g_vTreeRotationTrig.yxw, vTangent.xyz), dot(g_vTreeRotationTrig.zyw, vTangent.xyz));
#endif
    float3 vBinormal = cross(vNormal.xyz, vTangent);
    
    // 0.5f + 0.5f * expr not used since texcoords aren't clamped to [0,1] range - this approach saves
    // several instructions
    sOutput.vNormal.x = dot(g_vLightDir.xyz, vTangent.xyz);
    sOutput.vNormal.y = dot(g_vLightDir.xyz, vBinormal.xyz);
    sOutput.vNormal.z = dot(g_vLightDir.xyz, vNormal.xyz);
    
    sOutput.vNormal.xyz *= fDimming;
#else
    sOutput.vColor.rgb = fDimming * LightDiffuse(vPosition.xyz, vNormal.xyz, g_vLightDir, g_vLightDiffuse.rgb, g_vMaterialDiffuse.rgb);
    sOutput.vColor.a = 1.0f;
#endif

    // project position to the screen
    sOutput.vPosition = mul(g_mModelViewProj, float4(vPosition.xyz, 1.0f));
    
    // pass through other texcoords exactly as they were received
    sOutput.vBaseTexCoords.xy = vTexCoord0.xy;

#ifdef USE_FOG      
    // calc fog (cheap in vertex shader, relatively expensive later in the pixel shader)
#ifdef LEAF_NORMAL_MAPPING
    sOutput.vNormal.w = FogValue(sOutput.vPosition.z);
#else
    sOutput.fFog = FogValue(sOutput.vPosition.z);
#endif
#endif

    return sOutput;
}


///////////////////////////////////////////////////////////////////////  
//  LeafPS
//
//  Leaf geometry pixel shader, shared by both leaf cards and mshes.  This 
//  shader processes only one layer:
//      - diffuse composite leaf/frond map
//
//  There is no self-shadow layer used in the leaf geometry.

float4 LeafPS(SLeafOutput In) : COLOR
{
#ifdef LEAF_NORMAL_MAPPING
    // since the normal map normal values (normally ranged [-1,1]) are stored
    // as a color value (ranged [0,1]), they must be uncompressed.  a dot product 
    // is used to compute the diffuse lighting contribution.
    float4 texNormal = tex2D(samCompositeNormalLeafMap, In.vBaseTexCoords.xy);
    const float3 vHalves = { 0.5f, 0.5f, 0.5f };
    float fDot = saturate(dot(2.0f * (texNormal.rgb - vHalves), In.vNormal.rgb));
#endif

    // look up the diffuse layer
    float4 texDiffuse = tex2D(samCompositeDiffuseLeafMap, In.vBaseTexCoords.xy);
    
    // compute the ambient contribution (pulled from the diffuse map)
    float3 vAmbient = texDiffuse.xyz * g_vMaterialAmbient.xyz;

    // compute the full lighting equation, including diffuse and ambient values and
    // their respective scales.
#ifdef LEAF_NORMAL_MAPPING
    float4 vColor = float4(texDiffuse.rgb * fDot * g_fDiffuseScale * g_vLightDiffuse.rgb + vAmbient, texDiffuse.a);
#else
    float4 vColor = float4(saturate(In.vColor.rgb * texDiffuse.rgb * g_fDiffuseScale + vAmbient), In.vColor.a * texDiffuse.a);
#endif
    
#ifdef USE_FOG
    // if fog is active, interpolate between the unfogged color and the fog color
    // based on vertex shader fog value
#ifdef LEAF_NORMAL_MAPPING
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

technique LeafCards
{
    pass P0
    {          
        VS_COMPILE_COMMAND LeafCardVS( );
        PS_COMPILE_COMMAND LeafPS( );
    }
}

technique LeafMeshes
{
    pass P0
    {          
        VS_COMPILE_COMMAND LeafMeshVS( );
        PS_COMPILE_COMMAND LeafPS( );
    }
}

